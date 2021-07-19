#pragma once

#include "ClangMetrics.h"
#include <clang/Metrics/RecursiveASTPrePostVisitor.h>
#include <clang/AST/ParentMapContext.h>

#include <unordered_set>

// Inner class implementing Clang's RecursiveASTVisitor pattern. Defines callbacks to the AST.
// See the Clang documentation for more info.
class clang::metrics::detail::ClangMetrics::NodeVisitor final : public clang::RecursiveASTPrePostVisitor<NodeVisitor>
{
public:
  NodeVisitor(ClangMetrics& action) : rMyMetrics(action)
  {}

  clang::FileID locToFileID(const clang::SourceLocation & loc);

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
  bool VisitEnumConstantDecl(const clang::EnumConstantDecl * decl);
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
  void VisitEndDecl(const Decl* decl);
  bool VisitStmt(const clang::Stmt* stmt);
  void VisitEndStmt(const clang::Stmt* stmt);

  bool VisitDeclStmt(const clang::DeclStmt* stmt);
  bool TraverseLambdaExpr(const clang::LambdaExpr * stmt);
  bool VisitLambdaExpr(const clang::LambdaExpr* stmt);
  bool VisitIfStmt(const clang::IfStmt* stmt);
  bool VisitForStmt(const clang::ForStmt* stmt);
  bool VisitCXXForRangeStmt(const clang::CXXForRangeStmt* stmt);
  bool VisitCompoundStmt(const clang::CompoundStmt * stmt);
  bool VisitWhileStmt(const clang::WhileStmt* stmt);
  bool VisitConditionalOperator(const clang::ConditionalOperator * op);
  bool VisitParenExpr(const clang::ParenExpr * expr);
  bool VisitDoStmt(const clang::DoStmt* stmt);
  bool VisitSwitchStmt(const clang::SwitchStmt* stmt);
  bool VisitCaseStmt(const clang::CaseStmt* stmt);
  bool VisitDefaultStmt(const clang::DefaultStmt* stmt);
  bool VisitBreakStmt(const clang::BreakStmt* stmt);
  bool VisitContinueStmt(const clang::ContinueStmt* stmt);
  bool VisitLabelStmt(const clang::LabelStmt* stmt);
  bool VisitGotoStmt(const clang::GotoStmt* stmt);
  bool VisitCXXTryStmt(const clang::CXXTryStmt* stmt);
  bool VisitArraySubscriptExpr(const clang::ArraySubscriptExpr * expr);
  bool VisitInitListExpr(const clang::InitListExpr * expr);
  bool VisitCXXCatchStmt(const clang::CXXCatchStmt* stmt);
  bool VisitCXXThrowExpr(const clang::CXXThrowExpr* stmt);
  bool VisitReturnStmt(const clang::ReturnStmt* stmt);
  bool VisitUnaryExprOrTypeTraitExpr(const clang::UnaryExprOrTypeTraitExpr* stmt);
  bool VisitDeclRefExpr(const clang::DeclRefExpr* stmt);
  bool VisitCallExpr(const clang::CallExpr* stmt);
  bool VisitCXXConstructorDecl(const clang::CXXConstructorDecl * decl);
  bool VisitCXXConstructExpr(const clang::CXXConstructExpr* stmt);
  bool VisitCXXUnresolvedConstructExpr(const clang::CXXUnresolvedConstructExpr * expr);
  bool VisitMemberExpr(const clang::MemberExpr* stmt);
  bool VisitCXXThisExpr(const clang::CXXThisExpr* stmt);
  bool VisitCXXNewExpr(const clang::CXXNewExpr* stmt);
  bool VisitCXXDeleteExpr(const clang::CXXDeleteExpr* stmt);
  bool VisitExplicitCastExpr(const clang::ExplicitCastExpr* stmt);
  bool VisitObjCBridgedCastExpr(const clang::ObjCBridgedCastExpr* stmt);
  bool VisitObjCBoxedExpr(const clang::ObjCBoxedExpr* stmt);
  bool VisitObjCAtSynchronizedStmt(const clang::ObjCAtSynchronizedStmt* stmt);
  bool VisitObjCAtFinallyStmt(const clang::ObjCAtFinallyStmt* stmt);
  bool VisitObjCAtTryStmt(const clang::ObjCAtTryStmt* stmt);
  bool VisitObjCAtCatchStmt(const clang::ObjCAtCatchStmt* stmt);
  bool VisitObjCAtThrowStmt(const clang::ObjCAtThrowStmt* stmt);
  bool VisitObjCEncodeExpr(const clang::ObjCEncodeExpr* stmt);

  bool VisitIntegerLiteral(const clang::IntegerLiteral* stmt);
  bool VisitFloatingLiteral(const clang::FloatingLiteral* stmt);
  bool VisitCharacterLiteral(const clang::CharacterLiteral* stmt);
  bool VisitStringLiteral(const clang::StringLiteral* stmt); 
  bool VisitCXXBoolLiteralExpr(const clang::CXXBoolLiteralExpr* stmt);
  bool VisitCXXNullPtrLiteralExpr(const clang::CXXNullPtrLiteralExpr* stmt);
  bool VisitObjCBoolLiteralExpr(const ObjCBoolLiteralExpr* stmt);
  bool VisitObjCMessageExpr(const clang::ObjCMessageExpr* stmt);
  //bool VisitCompoundLiteralExpr(const clang::CompoundLiteralExpr* stmt); <-- TODO: find out what it is

  //bool VisitConditionalOperator(const clang::ConditionalOperator* op) {  return true; }
  bool VisitBinaryOperator(const clang::BinaryOperator* op);
  bool VisitUnaryOperator(const clang::UnaryOperator* op);
  bool VisitClassScopeFunctionSpecializationDecl(const clang::ClassScopeFunctionSpecializationDecl* decl);

private:
  // Helper function.
  void increaseMcCCStmt(const clang::Stmt* stmt);

  // Helper function. Returns the declaration context of a statement or nullptr if there's no such context.
  const clang::Decl* getDeclFromStmt(const clang::Stmt& stmt);

  const clang::FunctionDecl * getLambdaAncestor(const clang::Stmt & stmt);

  bool isStmtInALambdaCapture(const clang::Stmt & stmt);

  // Helper function. Returns the function in which a statement is used, or nullptr if the statement is not within a function.
  const clang::DeclContext* getFunctionContextFromStmt(const clang::Stmt& stmt);

  const clang::DeclContext * getFunctionContext();

  const clang::DeclContext * getFunctionContext(const clang::Decl * decl);

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

  bool handleTemplateArgument(HalsteadStorage & hs, const clang::TemplateArgument &arg);

  // Helper function for converting Clang's overloaded operator type to UnifiedCXXOperator.
  static UnifiedCXXOperator convertOverloadedOperator(const clang::CXXOperatorCallExpr& stmt);

  // Handle semicolons. Returns true if it added one (as halstead operator)
  bool handleSemicolon(const clang::SourceManager& sm, const clang::DeclContext* f, clang::SourceLocation semiloc, bool isMacro = false);

  // Needed because local classes are not visited correctly (ValueDecl issue).
  void handleFunctionRelatedHalsteadStuff(HalsteadStorage& hs, const clang::FunctionDecl* decl);
  void handleMethodRelatedHalsteadStuff(HalsteadStorage& hs, const clang::CXXMethodDecl* decl);

  // Handle countings for NL and NLE metrics
  void handleNLMetrics(const clang::Stmt* stmt, bool increase);

  // Helper function. Returns the first parent (recursively searched upwards) of the node or null if there is no
  // parent of type T for the node.
  template<class T, class N> const T* searchForParent(N node)
  {
    using namespace clang;

    ASTContext* con = rMyMetrics.getASTContext();
    DynTypedNodeList parents = con->getParents(*node);

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

public:
  ClangMetrics& rMyMetrics;

private:
  std::stack<const clang::Decl*> pCurrentFunctionDecl;
  std::unordered_set<void *> alreadyVisitedNodes;
  clang::SourceLocation lastFieldBeginLoc;
};
