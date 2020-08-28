#ifndef _ASTPREPOSTVISITOR_H_
#define _ASTPREPOSTVISITOR_H_

#include <vector>
#include <clang/Metrics/Output.h>

namespace clang
{
  class ASTContext;
  class Decl;
  class Stmt;
  class Type;

  namespace metrics
  {
    namespace detail
    {
      class GlobalMergeData_ThreadSafe;
    }
  }

class ASTPrePostVisitor
{
  public:
    virtual bool visitDecl(clang::Decl* decl) { return true; };
    virtual void visitEndDecl(clang::Decl* decl) {};

    virtual bool visitStmt(clang::Stmt* stmt) { return true; };
    virtual void visitEndStmt(clang::Stmt* stmt) {};

    virtual bool visitType(clang::Type* type) { return true; };
    virtual void visitEndType(clang::Type* type) {};
};

class ASTPrePostTraverser
{
  public:
    enum class NodeType
    {
      Statement,
      Declaration,
      Type
    };

    struct NodeInfo
    {
      void*    nodePtr;
      NodeType nodeType;
      bool     end;
    };

    using NodeList = std::vector<NodeInfo>;

  public:
    ASTPrePostTraverser(const clang::ASTContext& astContext, ASTPrePostVisitor& visitor, clang::metrics::detail::GlobalMergeData_ThreadSafe* gmd = nullptr, bool visitTemplateInstantiations = false, bool visitImplicitCode = false);
    ASTPrePostTraverser(const clang::ASTContext& astContext, clang::Decl* decl, ASTPrePostVisitor& visitor, bool visitTemplateInstantiations = false, bool visitImplicitCode = false);
    ASTPrePostTraverser(const clang::ASTContext& astContext, clang::Stmt* stmt, ASTPrePostVisitor& visitor, bool visitTemplateInstantiations = false, bool visitImplicitCode = false);
    void run();

  private:
    const clang::ASTContext& astContext;
    ASTPrePostVisitor& visitor;
    NodeList merged;
};




} // clang namespace

#endif

