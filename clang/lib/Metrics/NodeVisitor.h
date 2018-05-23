#pragma once

#include "ClangMetrics.h"
#include <clang/AST/RecursiveASTVisitor.h>


// Inner class implementing Clang's RecursiveASTVisitor pattern. Defines callbacks to the AST.
// See the Clang documentation for more info.
class clang::metrics::detail::ClangMetrics::NodeVisitor final : public clang::RecursiveASTVisitor<NodeVisitor>
{
public:
  NodeVisitor(ClangMetrics& action) : rMyMetrics(action), pCurrentFunctionDecl(nullptr)
  {}

  // Callbacks triggered when visiting a specific AST node.
  bool VisitCXXRecordDecl(const clang::CXXRecordDecl* decl);
  bool VisitFunctionDecl(const clang::FunctionDecl* decl);
  bool VisitFunctionTemplateDecl(const clang::FunctionTemplateDecl* decl);
  bool VisitTemplateTypeParmDecl(const clang::TemplateTypeParmDecl* decl);
  bool VisitNonTypeTemplateParmDecl(const clang::NonTypeTemplateParmDecl* decl);
  bool VisitTemplateTemplateParmDecl(const clang::TemplateTemplateParmDecl* decl);
  bool VisitCXXMethodDecl(const clang::CXXMethodDecl* decl);
  bool VisitEnumDecl(const clang::EnumDecl* decl);
  bool VisitValueDecl(const clang::ValueDecl* decl);
  bool VisitVarDecl(const clang::VarDecl* decl);
  bool VisitFieldDecl(const clang::FieldDecl* decl);
  bool VisitAccessSpecDecl(const clang::AccessSpecDecl* decl);
  bool VisitUsingDecl(const clang::UsingDecl* decl);
  bool VisitUsingDirectiveDecl(const clang::UsingDirectiveDecl* decl);
  bool VisitNamespaceAliasDecl(const clang::NamespaceAliasDecl* decl);
  bool VisitTypeAliasDecl(const clang::TypeAliasDecl* decl);
  bool VisitTypedefDecl(const clang::TypedefDecl* decl);
  bool VisitFriendDecl(const clang::FriendDecl* decl);
  bool VisitObjCMethodDecl(const clang::ObjCMethodDecl* decl);

  bool VisitDecl(const clang::Decl* decl);
  bool VisitStmt(const clang::Stmt* stmt);

  bool VisitDeclStmt(const clang::DeclStmt* stmt);
  bool VisitIfStmt(const clang::IfStmt* stmt);
  bool VisitForStmt(const clang::ForStmt* stmt);
  bool VisitCXXForRangeStmt(const clang::CXXForRangeStmt* stmt);
  bool VisitWhileStmt(const clang::WhileStmt* stmt);
  bool VisitDoStmt(const clang::DoStmt* stmt);
  bool VisitSwitchStmt(const clang::SwitchStmt* stmt);
  bool VisitCaseStmt(const clang::CaseStmt* stmt);
  bool VisitDefaultStmt(const clang::DefaultStmt* stmt);
  bool VisitBreakStmt(const clang::BreakStmt* stmt);
  bool VisitContinueStmt(const clang::ContinueStmt* stmt);
  bool VisitLabelStmt(const clang::LabelStmt* stmt);
  bool VisitGotoStmt(const clang::GotoStmt* stmt);
  bool VisitObjCBridgedCastExpr(const clang::ObjCBridgedCastExpr* stmt);
  bool VisitObjCBoxedExpr(const clang::ObjCBoxedExpr* stmt);
  bool VisitCXXTryStmt(const clang::CXXTryStmt* stmt);
  bool VisitObjCAtSynchronizedStmt(const clang::ObjCAtSynchronizedStmt* stmt);
  bool VisitObjCAtFinallyStmt(const clang::ObjCAtFinallyStmt* stmt);
  bool VisitObjCAtTryStmt(const clang::ObjCAtTryStmt* stmt);
  bool VisitCXXCatchStmt(const clang::CXXCatchStmt* stmt);
  bool VisitObjCAtCatchStmt(const clang::ObjCAtCatchStmt* stmt);
  bool VisitCXXThrowExpr(const clang::CXXThrowExpr* stmt);
  bool VisitObjCAtThrowStmt(const clang::ObjCAtThrowStmt* stmt);
  bool VisitReturnStmt(const clang::ReturnStmt* stmt);
  bool VisitUnaryExprOrTypeTraitExpr(const clang::UnaryExprOrTypeTraitExpr* stmt);
  bool VisitDeclRefExpr(const clang::DeclRefExpr* stmt);
  bool VisitCallExpr(const clang::CallExpr* stmt);
  bool VisitCXXConstructExpr(const clang::CXXConstructExpr* stmt);
  bool VisitMemberExpr(const clang::MemberExpr* stmt);
  bool VisitCXXThisExpr(const clang::CXXThisExpr* stmt);
  bool VisitCXXNewExpr(const clang::CXXNewExpr* stmt);
  bool VisitCXXDeleteExpr(const clang::CXXDeleteExpr* stmt); 
  bool VisitObjCEncodeExpr(const clang::ObjCEncodeExpr* stmt);
  bool VisitExplicitCastExpr(const clang::ExplicitCastExpr* stmt);

  bool VisitIntegerLiteral(const clang::IntegerLiteral* stmt);
  bool VisitFloatingLiteral(const clang::FloatingLiteral* stmt);
  bool VisitCharacterLiteral(const clang::CharacterLiteral* stmt);
  bool VisitStringLiteral(const clang::StringLiteral* stmt); 
  bool VisitCXXBoolLiteralExpr(const clang::CXXBoolLiteralExpr* stmt);
  bool VisitObjCBoolLiteralExpr(const ObjCBoolLiteralExpr* stmt);
  bool VisitCXXNullPtrLiteralExpr(const clang::CXXNullPtrLiteralExpr* stmt);
  //bool VisitCompoundLiteralExpr(const clang::CompoundLiteralExpr* stmt); <-- TODO: find out what it is

  bool VisitObjCMessageExpr(const clang::ObjCMessageExpr* stmt);

  bool VisitConditionalOperator(const clang::ConditionalOperator* op) { increaseMcCCStmt(op); return true; }
  bool VisitBinaryOperator(const clang::BinaryOperator* op);
  bool VisitUnaryOperator(const clang::UnaryOperator* op);

private:
  // Helper function.
  void increaseMcCCStmt(const clang::Stmt* stmt);

  // Helper function. Returns the declaration context of a statement or nullptr if there's no such context.
  const clang::Decl* getDeclFromStmt(const clang::Stmt& stmt);

  // Helper function. Returns the function in which a statement is used, or nullptr if the statement is not within a function.
  const clang::DeclContext* getFunctionContextFromStmt(const clang::Stmt& stmt);

  // Helper function. Creates Halstead operators/operands based on a qualified name, and appends them to
  // the HalsteadStorage of func.
  void handleNestedName(const clang::DeclContext* f, const clang::NestedNameSpecifier* nns);

  // Helper function. Adds Halstead operators/operands for a given type.
  // Desugars the type to handle complex types, like pointer to a const.
  // Adds type qualifiers (eg.: const).
  // Param 'isOperator' decides whether the type will be added as an operator or an operand.
  void handleQualType(HalsteadStorage& hs, const clang::QualType& qtype, bool isOperator);

  // Helper function. Use it to handle function call arguments.
  void handleCallArgs(HalsteadStorage& hs, const clang::Stmt* arg);

  // Helper function for converting Clang's overloaded operator type to UnifiedCXXOperator.
  static UnifiedCXXOperator convertOverloadedOperator(const clang::CXXOperatorCallExpr& stmt);

  // Handle semicolons.
  void handleSemicolon(const clang::SourceManager& sm, const clang::DeclContext* f, clang::SourceLocation semiloc);

  // Needed because local classes are not visited correctly (ValueDecl issue).
  void handleFunctionRelatedHalsteadStuff(HalsteadStorage& hs, const clang::FunctionDecl* decl);
  void handleMethodRelatedHalsteadStuff(HalsteadStorage& hs, const clang::CXXMethodDecl* decl);

  // Helper function. Returns the first parent (recursively searched upwards) of the node or null if there is no
  // parent of type T for the node.
  template<class T, class N> const T* searchForParent(N node)
  {
    using namespace clang;

    ASTContext* con = rMyMetrics.getASTContext();
    ASTContext::DynTypedNodeList parents = con->getParents(*node);

    auto it = parents.begin();
    if (it == parents.end())
      return nullptr;

    const T* parent = it->get<T>();
    if (parent)
      return parent;

    const Stmt* parentStmt = it->get<Stmt>();
    if (parentStmt)
      return searchForParent<T>(parentStmt);

    return nullptr;
  }

private:
  ClangMetrics& rMyMetrics;
  const clang::Decl* pCurrentFunctionDecl;
};
