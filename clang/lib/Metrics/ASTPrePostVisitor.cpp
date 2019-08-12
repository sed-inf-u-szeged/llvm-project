#include <clang/Metrics/ASTPrePostVisitor.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
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
      ASTMergeVisitor(const ASTContext &context, const bool post, NodeList& nodes, const bool visitTemplateInstantiations, const bool visitImplicitCode)
        : context (context)
        , post (post)
        , visitTemplateInstantiations (visitTemplateInstantiations)
        , visitImplicitCode (visitImplicitCode)
        , nodes (nodes)
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

    private:
      const ASTContext &context;
      const bool post;
      const bool visitTemplateInstantiations;
      const bool visitImplicitCode;
      NodeList& nodes;
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

}

namespace clang
{

ASTPrePostTraverser::ASTPrePostTraverser(const clang::ASTContext& astContext, ASTPrePostVisitor& visitor, const bool visitTemplateInstantiations, const bool visitImplicitCode)
  : astContext (astContext)
  , visitor (visitor)
{
  if (astContext.getTranslationUnitDecl() != nullptr)
  {
    NodeList pre, post;
    ASTMergeVisitor(astContext, false, pre, visitTemplateInstantiations, visitImplicitCode).TraverseDecl(astContext.getTranslationUnitDecl());
    ASTMergeVisitor(astContext, true, post, visitTemplateInstantiations, visitImplicitCode).TraverseDecl(astContext.getTranslationUnitDecl());
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

