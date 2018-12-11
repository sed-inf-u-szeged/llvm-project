#pragma once

#include "ASTPrePostVisitor.h"
#include <clang/AST/RecursiveASTVisitor.h>

namespace clang
{

  template <typename Derived>
  class RecursiveASTPrePostVisitor : public clang::ASTPrePostVisitor, public clang::RecursiveASTVisitor<Derived>
  {
    public:
      bool visitDecl(Decl* decl);
      void visitEndDecl(Decl* decl);
      bool visitStmt(Stmt* stmt);
      void visitEndStmt(Stmt* stmt);
  };

  #define TRY_TO(CALL_EXPR)                                                    \
  do {                                                                         \
    if (!RecursiveASTVisitor<Derived>::getDerived().CALL_EXPR)                 \
      return false;                                                            \
  } while (false)

  template <typename Derived>
  bool RecursiveASTPrePostVisitor<Derived>::visitDecl(Decl* decl)
  {
    switch (decl->getKind()) {
      #define ABSTRACT_DECL(DECL)
      #define DECL(CLASS, BASE)                                                       \
      case Decl::CLASS:                                                               \
        TRY_TO(WalkUpFrom##CLASS##Decl(static_cast<CLASS##Decl *>(decl)));            \
      break;
      #include "clang/AST/DeclNodes.inc"
      #undef ABSTRACT_DECL
      #undef DECL
    }
    return true;
  }

  template <typename Derived>
  void RecursiveASTPrePostVisitor<Derived>::visitEndDecl(Decl* decl)
  {
    static_cast<Derived*>(this)->VisitEndDecl(decl);
  }

  template <typename Derived>
  bool RecursiveASTPrePostVisitor<Derived>::visitStmt(Stmt* stmt)
  {
    switch (stmt->getStmtClass()) {
      case Stmt::NoStmtClass:
        break;
      #define ABSTRACT_STMT(STMT)
      #define STMT(CLASS, PARENT)                                                    \
      case Stmt::CLASS##Class:                                                       \
        TRY_TO(WalkUpFrom##CLASS(static_cast<CLASS *>(stmt))); break;
      #define INITLISTEXPR(CLASS, PARENT)                                            \
      case Stmt::CLASS##Class:                                                       \
      {                                                                              \
        auto ILE = static_cast<CLASS *>(stmt);                                       \
        if (auto Sem = ILE->isSemanticForm() ? ILE : ILE->getSemanticForm())         \
          TRY_TO(WalkUpFrom##CLASS(Sem));                                            \
        if (auto Syn = ILE->isSemanticForm() ? ILE->getSyntacticForm() : ILE)        \
          TRY_TO(WalkUpFrom##CLASS(Syn));                                            \
        break;                                                                       \
      }
      #include "clang/AST/StmtNodes.inc"
      #undef ABSTRACT_STMT
      #undef STMT
      #undef INITLISTEXPR
    }
    return true;
  }

  template <typename Derived>
  void RecursiveASTPrePostVisitor<Derived>::visitEndStmt(Stmt* stmt)
  {
    static_cast<Derived*>(this)->VisitEndStmt(stmt);
  }
  #undef TRY_TO
}
