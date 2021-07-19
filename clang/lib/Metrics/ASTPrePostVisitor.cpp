#include <clang/Metrics/ASTPrePostVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include "ClangMetrics.h"
#include <stack>
#include<iostream>

using namespace clang;
using namespace std;

namespace
{

  using NodeType = clang::ASTPrePostTraverser::NodeType;
  using NodeInfo = clang::ASTPrePostTraverser::NodeInfo;
  using NodeList = clang::ASTPrePostTraverser::NodeList;
  
  class ASTMergeVisitor : public RecursiveASTVisitor<ASTMergeVisitor>
  {
    public:
    ASTMergeVisitor(
        const ASTContext &context, const bool post, NodeList &nodes,
        const bool visitTemplateInstantiations, const bool visitImplicitCode,
        clang::metrics::detail::GlobalMergeData_ThreadSafe *gmd = nullptr,
        std::set<llvm::sys::fs::UniqueID> *filesToTraverse = nullptr)
        : context (context)
        , post (post)
        , visitTemplateInstantiations (visitTemplateInstantiations)
        , visitImplicitCode (visitImplicitCode)
        , nodes (nodes)
        , gmd(gmd)
        , filesToTraverse(filesToTraverse)
      {
      }

      bool VisitStmt(Stmt *s)
      {
        nodes.push_back({s, NodeType::Statement, post});
        return true;
      }

      bool VisitDecl(Decl *d)
      {
        nodes.push_back({d, NodeType::Declaration, post});
        return true;
      }

      bool VisitType(Type *t)
      {
        nodes.push_back({t, NodeType::Type, post});
        return true;
      }

      bool shouldTraversePostOrder() const
      {
        return post;
      }

      bool shouldVisitImplicitCode() const
      {
        return visitImplicitCode;
      }

      bool shouldVisitTemplateInstantiations() const
      {
        return visitTemplateInstantiations;
      }

      bool TraverseDecl(clang::Decl *decl);

    private:
      const ASTContext &context;
      const bool post;
      const bool visitTemplateInstantiations;
      const bool visitImplicitCode;
      NodeList& nodes;
      clang::metrics::detail::GlobalMergeData_ThreadSafe* gmd;
      std::set<llvm::sys::fs::UniqueID> *filesToTraverse;
  };

  void dumpNodeInfo(const NodeInfo& nodeInfo, ASTContext *context)
  {
    switch (nodeInfo.nodeType)
    {
      case NodeType::Statement:
      {
        Stmt* stmt = static_cast<Stmt*>(nodeInfo.nodePtr);
        stmt->dump();
        break;

      }
      case NodeType::Declaration:
      {
        Decl* decl = static_cast<Decl*>(nodeInfo.nodePtr);
        decl->dump();
        break;
      }
      case NodeType::Type:
      {
        Type* type = static_cast<Type*>(nodeInfo.nodePtr);
        type->dump();
        break;
      }
    }
  }

  NodeList merge(const NodeList& pre, const NodeList& post)
  {
    NodeList result;
    stack<void*> nodeStack;
    auto preItemIterator = pre.begin();
    auto postItemIterator = post.begin();

    if (pre.size() != post.size())
    {
      llvm::errs() << "Size of the preorder and postorder vector is different!\n";
      for (auto& preNode : pre)
      {
        auto found = find_if(post.begin(), post.end(), [&preNode](auto& postNode){return preNode.nodePtr == postNode.nodePtr; });
        if (found == post.end())
        {
          dumpNodeInfo(preNode, nullptr);
        }
      }
      return result;
    }

    if (pre.empty())
      return result;

    // Insert the first element into the stack
    nodeStack.push(preItemIterator->nodePtr);
    result.push_back(*preItemIterator);
    ++preItemIterator;

    while (postItemIterator != post.end())
    {
      if (!nodeStack.empty() && (nodeStack.top() == postItemIterator->nodePtr))
      {
        result.push_back(*postItemIterator);
        ++postItemIterator;
        nodeStack.pop();
      }
      else
      {
      if (preItemIterator == pre.end())
      return result;

        result.push_back(*preItemIterator);
        nodeStack.push(preItemIterator->nodePtr);
        ++preItemIterator;
      }
    }

    return result;
  }

  bool ASTMergeVisitor::TraverseDecl(clang::Decl * decl)
  {
    if(!gmd || !filesToTraverse)
      return RecursiveASTVisitor<ASTMergeVisitor>::TraverseDecl(decl);

    //std::cout << "clangmetrics traverseDecl (merge)" << std::endl;
    if(!decl)
      return true;
    
    const SourceManager &sm = context.getSourceManager();

    SourceLocation loc = decl->getLocation();
    FileID fileid;
    if(loc.isMacroID())
      fileid = sm.getFileID(sm.getExpansionLoc(loc)); // we need this, as for code in macros, the spellingloc has no file attached to it
    else
      fileid = sm.getFileID(loc);

    const FileEntry *fileEntry = sm.getFileEntryForID(fileid);
    

    if(fileEntry)
    {
      auto fileID = fileEntry->getUniqueID();
      bool shouldTraverse = false;
      if (filesToTraverse->count(fileID) != 0)
        shouldTraverse = true;
      else
      {
        gmd->call([&](metrics::detail::GlobalMergeData& mergeData) {
          //only visit if this file was not yet visited by another thread
          if (mergeData.files.count(fileID) == 0)
          {
            // files is accessable by every thread and is used to make sure a file isn't traversed by multiple threads
            mergeData.files.insert(make_pair(fileID, fileEntry));
            // filesToTraverse is used to store which files this thread will traverse, so the post traverser knows which ones the pre traverser traversed
            filesToTraverse->insert(fileID);
            shouldTraverse = true;
            //std::cout << "clangmetrics traversed stuff" << std::endl;
          }
          else
          {
            //std::cout << "clangmetrics skipped" << std::endl;
          }
        });
      }

      if(shouldTraverse)
        RecursiveASTVisitor<ASTMergeVisitor>::TraverseDecl(decl);
    }
    else 
    {
      clang::RecursiveASTVisitor<ASTMergeVisitor>::TraverseDecl(decl);
      //std::cout << "clangmetrics traversed stuff" << std::endl;
    }
    //cout << "clangmetrics traversed decl (mergevisitor)" << endl;

    return true;
  }

}

namespace clang
{

ASTPrePostTraverser::ASTPrePostTraverser(const clang::ASTContext& astContext, ASTPrePostVisitor& visitor, clang::metrics::detail::GlobalMergeData_ThreadSafe *gmd, const bool visitTemplateInstantiations, const bool visitImplicitCode)
  : astContext (astContext)
  , visitor (visitor)
{
  if (astContext.getTranslationUnitDecl() != nullptr)
  {
    // Used to store which files the ASTMergeVisitor traverses, so that the pre and post will traverse the same files. This is required due to multithreading
    std::set<llvm::sys::fs::UniqueID> filesToTraverse;
    NodeList pre, post;
    ASTMergeVisitor(astContext, false, pre, visitTemplateInstantiations, visitImplicitCode, gmd, &filesToTraverse).TraverseDecl(astContext.getTranslationUnitDecl()); //ezt kell overrideolni
    ASTMergeVisitor(astContext, true, post, visitTemplateInstantiations, visitImplicitCode, gmd, &filesToTraverse).TraverseDecl(astContext.getTranslationUnitDecl());
    merged = merge(pre, post);
  }
}


ASTPrePostTraverser::ASTPrePostTraverser(const clang::ASTContext& astContext, clang::Decl* decl, ASTPrePostVisitor& visitor, const bool visitTemplateInstantiations, const bool visitImplicitCode)
  : astContext (astContext)
  , visitor (visitor)
{
  if (decl != nullptr)
  {
    NodeList pre, post;
    ASTMergeVisitor(astContext, false, pre, visitTemplateInstantiations, visitImplicitCode).TraverseDecl(decl);
    ASTMergeVisitor(astContext, true, post, visitTemplateInstantiations, visitImplicitCode).TraverseDecl(decl);
    merged = merge(pre, post);
  }
}


ASTPrePostTraverser::ASTPrePostTraverser(const clang::ASTContext& astContext, clang::Stmt* stmt, ASTPrePostVisitor& visitor, const bool visitTemplateInstantiations, const bool visitImplicitCode)
  : astContext (astContext)
  , visitor (visitor)
{
  if (stmt != nullptr)
  {
    NodeList pre, post;
    ASTMergeVisitor(astContext, false, pre, visitTemplateInstantiations, visitImplicitCode).TraverseStmt(stmt);
    ASTMergeVisitor(astContext, true, post, visitTemplateInstantiations, visitImplicitCode).TraverseStmt(stmt);
    merged = merge(pre, post);
  }
}


void ASTPrePostTraverser::run()
{
  for (const auto& node : merged)
  {
    switch (node.nodeType)
    {
      case NodeType::Declaration:
        if (node.end)
          visitor.visitEndDecl(static_cast<Decl*>(node.nodePtr));
        else
          visitor.visitDecl(static_cast<Decl*>(node.nodePtr));
        break;
      case NodeType::Statement:
        if (node.end)
          visitor.visitEndStmt(static_cast<Stmt*>(node.nodePtr));
        else
          visitor.visitStmt(static_cast<Stmt*>(node.nodePtr));
        break;
      case NodeType::Type:
        if (node.end)
          visitor.visitEndType(static_cast<Type*>(node.nodePtr));
        else
          visitor.visitType(static_cast<Type*>(node.nodePtr));
        break;
    }
  }

}


} // namespace clang

