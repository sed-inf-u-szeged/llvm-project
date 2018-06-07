#pragma once

#include "ASTPrePostVisitor.h"
#include <clang/AST/RecursiveASTVisitor.h>

namespace clang
{
  template <typename Derived> 
  class RecursiveASTPrePostVisitor : public columbus::ASTPrePostVisitor, public clang::RecursiveASTVisitor<Derived>
  {
  public:
	bool visitDecl(Decl* decl);

	void visitEndDecl(Decl* decl);

	bool visitStmt(Stmt* stmt);

  };

  template <typename Derived>
  bool RecursiveASTPrePostVisitor<Derived>::visitDecl(Decl* decl)
  {
    switch (decl->getKind()) {
    #define DECL(CLASS, BASE)                                                      \
    case Decl::CLASS:                                                              \
    TRY_TO(WalkUpFrom##CLASS(static_cast<CLASS##Decl *>(D)))                       \                                                                  
    break;
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
	case Stmt::NoStmtClass: break;
    #define STMT(CLASS, PARENT)                                                \
    case Stmt::CLASS##Class:                                                   \
    TRY_TO(WalkUpFrom##CLASS(static_cast<CLASS *>(S))); break;
    #define INITLISTEXPR(CLASS, PARENT)                                        \
    case Stmt::CLASS##Class:                                                   \
    {                                                                          \
      auto ILE = static_cast<CLASS *>(S);                                      \
      if (auto Sem = ILE->isSemanticForm() ? ILE : ILE->getSemanticForm())     \
        TRY_TO(WalkUpFrom##CLASS(Sem));                                        \
      if (auto Syn = ILE->isSemanticForm() ? ILE->getSyntacticForm() : ILE)    \
        TRY_TO(WalkUpFrom##CLASS(Syn));                                        \
      break;                                                                   \
    }

    }
	return true;
  }

}