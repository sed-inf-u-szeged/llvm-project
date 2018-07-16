#include "NodeVisitor.h"

#include <clang/Lex/Preprocessor.h>
// Use clang namespace for simplicity
using namespace clang;
using namespace metrics::detail;

namespace
{
  // Helper function for finding the semicolon at the end of statements.
  SourceLocation findSemiAfterLocation(SourceLocation loc, ASTContext* con, bool isDecl);
}

bool ClangMetrics::NodeVisitor::VisitCXXRecordDecl(const CXXRecordDecl* decl)
{
  // Calculating Halstead for functions only.
  if (const DeclContext* f = decl->getParentFunctionOrMethod())
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // Add the declarator (class, struct or union) as operator.
    if (decl->isClass())
      hs.add<Halstead::ClassOperator>();
    else if (decl->isStruct())
      hs.add<Halstead::StructOperator>();
    else if (decl->isUnion())
      hs.add<Halstead::UnionOperator>();

    // Add name if there's one (operand).
    if (!decl->getDeclName().isEmpty())
      hs.add<Halstead::ValueDeclOperand>(decl);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitFunctionDecl(const FunctionDecl* decl)
{
  HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[decl].hsStorage;
  handleFunctionRelatedHalsteadStuff(hs, decl);

  return true;
}

bool ClangMetrics::NodeVisitor::VisitFunctionTemplateDecl(const clang::FunctionTemplateDecl* decl)
{
  if (FunctionDecl* templated = decl->getTemplatedDecl())
  {
    // Add template keyword (operator).
    rMyMetrics.myFunctionMetrics[templated].hsStorage.add<Halstead::TemplateOperator>();
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitTemplateTypeParmDecl(const clang::TemplateTypeParmDecl* decl)
{
  if (const DeclContext* f = decl->getParentFunctionOrMethod())
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // Add typename or class keyword (operator).
    if (decl->wasDeclaredWithTypename())
      hs.add<Halstead::TypenameOperator>();
    else
      hs.add<Halstead::ClassOperator>();

    // Add name of the parameter if there's any (operand).
    if (!decl->getDeclName().isEmpty())
      hs.add<Halstead::ValueDeclOperand>(decl);

    // Handle default arguments if there's any.
    if (decl->hasDefaultArgument())
    {
      // Add equal sign (operator).
      hs.add<Halstead::OperatorOperator>(BinaryOperatorKind::BO_Assign);

      // Add type argument (operator).
      handleQualType(hs, decl->getDefaultArgument(), true);
    }

    // Handle parameter packs.
    if (decl->isParameterPack())
      hs.add<Halstead::PackDeclarationOperator>();
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitNonTypeTemplateParmDecl(const clang::NonTypeTemplateParmDecl* decl)
{
  // Only handle default arguments here. Everything else is done in VisitValueDecl().
  if (const DeclContext* f = decl->getParentFunctionOrMethod())
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // If the decl has a default argument, we add the equal sign (operator).
    // The type and init value are already handled by VisitValueDecl().
    if (decl->hasDefaultArgument())
      hs.add<Halstead::OperatorOperator>(BinaryOperatorKind::BO_Assign);

    // Handle parameter packs.
    if (decl->isParameterPack())
      hs.add<Halstead::PackDeclarationOperator>();

    if (decl->isPackExpansion())
      hs.add<Halstead::PackExpansionOperator>();
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitTemplateTemplateParmDecl(const clang::TemplateTemplateParmDecl* decl)
{
  if (const DeclContext* f = decl->getParentFunctionOrMethod())
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // Add the inner 'template' keyword of the template template parameter.
    hs.add<Halstead::TemplateOperator>();

    // TODO: Right now Clang has no API to check whether the template template parameter was
    // declared with the 'class' or the 'typename' keyword, as the latter is only allowed in C++ 17.
    // Add as 'class' keyword for now.
    hs.add<Halstead::ClassOperator>();

    // Add name of the parameter if there's any (operand).
    if (!decl->getDeclName().isEmpty())
      hs.add<Halstead::ValueDeclOperand>(decl);

    // Add the default argument if there's any.
    if (decl->hasDefaultArgument())
    {
      // Add equal sign (operator).
      hs.add<Halstead::OperatorOperator>(BinaryOperatorKind::BO_Assign);

      // Add template argument (operator).
      TemplateDecl* tmp = decl->getDefaultArgument().getArgument().getAsTemplate().getAsTemplateDecl();
      if (tmp)
        hs.add<Halstead::TemplateNameOperator>(tmp);
    }
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitCXXMethodDecl(const CXXMethodDecl* decl)
{
  // No need to add the current node - it will be done by VisitFunctionDecl() anyway.

  // Forward to the method handler.
  HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[decl].hsStorage;
  handleMethodRelatedHalsteadStuff(hs, decl);

  return true;
}

bool ClangMetrics::NodeVisitor::VisitEnumDecl(const EnumDecl* decl)
{
  // Halstead stuff:
  if (const DeclContext* f = decl->getParentFunctionOrMethod())
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // Add 'enum' keyword (operator).
    hs.add<Halstead::EnumOperator>();

    // If this is a C++ 11 strongly typed enum, also add 'class' or 'struct' keyword.
    if (decl->isScoped())
    {
      if (decl->isScopedUsingClassTag())
        hs.add<Halstead::ClassOperator>();
      else
        hs.add<Halstead::StructOperator>();
    }

    // Add explicit underlying type if there's one.
    if (TypeSourceInfo* type = decl->getIntegerTypeSourceInfo())
      handleQualType(hs, type->getType(), true);

    // Add name if there's one (operand).
    if (!decl->getDeclName().isEmpty())
      hs.add<Halstead::ValueDeclOperand>(decl);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitValueDecl(const ValueDecl* decl)
{
  // Get function in which this decl is declared in.
  if (const DeclContext* f = decl->getParentFunctionOrMethod())
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // Handle CXXMethodDecl bug (?). For local classes, methods will be identified as ValueDecls at first.
    if (CXXMethodDecl::classof(decl))
    {
      handleMethodRelatedHalsteadStuff(hs, cast<CXXMethodDecl>(decl));
      handleFunctionRelatedHalsteadStuff(hs, cast<CXXMethodDecl>(decl));
    }
    else
    {
      // A value declaration always has an operator (its type).
      // However, we only handle it here if this declaration is not part of
      // a DeclStmt. DeclStmts are handled elsewhere. This is due to multi-decl-single-stmt, like 'int x, y, z;'.
      // We also don't add a type for enum constant declarations.
      ASTContext* con = rMyMetrics.getASTContext();
      assert(con && "ASTContext should always be available.");

      // There should be exactly one parent.
      auto parents = con->getParents(*decl);
      if (parents.begin() != parents.end())
      {
        const Stmt* ds = parents.begin()->get<Stmt>();
        if (!ds || !DeclStmt::classof(ds))
        {
          if (!EnumConstantDecl::classof(decl))
            handleQualType(hs, decl->getType(), true);
        }
      }

      // A value declaration can have an operand too (its name).
      if (!decl->getDeclName().isEmpty())
        hs.add<Halstead::ValueDeclOperand>(decl);
    }
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitVarDecl(const clang::VarDecl* decl)
{
  // Only handle init syntax and 'static' here. Everything else is done in VisitValueDecl().
  if (decl->hasInit())
  {
    // Get function in which this decl is declared in.
    if (const DeclContext* f = decl->getParentFunctionOrMethod())
    {
      // Handle initialization.
      switch (decl->getInitStyle())
      {
      case VarDecl::InitializationStyle::CInit:
        rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::OperatorOperator>(BinaryOperatorKind::BO_Assign);
        break;

      case VarDecl::InitializationStyle::CallInit:
        // Unfortunately implicit constructor calls are also considered "CallInit" even if there are no parentheses.
        // To avoid counting these, first check the init expression and branch on its "constructorness".
        if (const Expr* init = decl->getInit())
        {
          if (CXXConstructExpr::classof(init))
          {
            // Get the parentheses range in the source code. Only add the Halstead operator if it's not invalid.
            SourceRange range = cast<CXXConstructExpr>(init)->getParenOrBraceRange();
            if (range.isValid())
              rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::ParenthesesInitSyntaxOperator>();
          }
          else
          {
            // If it's not a constructor there's no such problem - the parentheses must be there.
            rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::ParenthesesInitSyntaxOperator>();
          }
        }
        break;

      case VarDecl::InitializationStyle::ListInit:
        rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::BracesInitSyntaxOperator>();
        break;
      }
    }
  }

  // Only need to handle 'static' for local variables.
  // FieldDecls (class members) cannot be static for local classes.
  if (decl->isStaticLocal())
  {
    if (const DeclContext* f = decl->getParentFunctionOrMethod())
      rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::StaticOperator>();
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitFieldDecl(const clang::FieldDecl* decl)
{
  // Only handle init and 'mutable' syntax here. Everything else is done in VisitValueDecl().
  if (decl->hasInClassInitializer())
  {
    // Get function in which this decl is declared in.
    if (const DeclContext* f = decl->getParentFunctionOrMethod())
    {
      // Handle initialization.
      switch (decl->getInClassInitStyle())
      {
      case InClassInitStyle::ICIS_CopyInit:
        rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::OperatorOperator>(BinaryOperatorKind::BO_Assign);
        break;

      case InClassInitStyle::ICIS_ListInit:
        rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::BracesInitSyntaxOperator>();
        break;
      }
    }
  }

  if (decl->isMutable())
  {
    if (const DeclContext* f = decl->getParentFunctionOrMethod())
      rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::MutableOperator>();
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitAccessSpecDecl(const clang::AccessSpecDecl* decl)
{
  // Get function in which this decl is declared in.
  if (const DeclContext* f = decl->getParentFunctionOrMethod())
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::AccessSpecDeclOperator>(decl);

  return true;
}

bool ClangMetrics::NodeVisitor::VisitUsingDecl(const clang::UsingDecl* decl)
{
  // Get function in which this decl is declared in.
  if (const DeclContext* f = decl->getParentFunctionOrMethod())
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // Add using keyword (operator).
    hs.add<Halstead::UsingOperator>();

    // The using keyword also has an operand.
    // TODO: Multiple names? foo::bar? How much does that count?
    hs.add<Halstead::UsingOperand>(decl);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitUsingDirectiveDecl(const clang::UsingDirectiveDecl* decl)
{
  // Get function in which this decl is declared in.
  if (const DeclContext* f = decl->getParentFunctionOrMethod())
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // Add using keyword (operator).
    hs.add<Halstead::UsingOperator>();

    // Add namespace keyword (operator).
    hs.add<Halstead::NamespaceOperator>();

    // Add name of the namespace (operand).
    // TODO: Multiple names? foo::bar? How much does that count?
    if (const NamespaceDecl* nn = decl->getNominatedNamespace())
      hs.add<Halstead::NamespaceOperand>(nn);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitNamespaceAliasDecl(const clang::NamespaceAliasDecl* decl)
{
  if (const DeclContext* f = decl->getParentFunctionOrMethod())
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // Add namespace keyword (operator).
    hs.add<Halstead::NamespaceOperator>();

    // Add alias name (operand).
    hs.add<Halstead::NamespaceOperand>(decl);

    // Add target name (operand).
    if (const NamedDecl* an = decl->getAliasedNamespace())
      hs.add<Halstead::NamespaceOperand>(an);

    // Add equal sign (operator) which is part of the alias declaration.
    hs.add<Halstead::OperatorOperator>(BinaryOperatorKind::BO_Assign);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitTypeAliasDecl(const clang::TypeAliasDecl* decl)
{
  ASTContext* con = rMyMetrics.getASTContext();
  assert(con && "ASTContext should always be available.");

  // Get function in which this decl is declared in
  if (const DeclContext* f = decl->getParentFunctionOrMethod())
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // Add using keyword (operator).
    hs.add<Halstead::UsingOperator>();

    // Add alias type (operand).
    if (const Type* type = con->getTypeDeclType(decl).getTypePtr())
      hs.add<Halstead::TypeOperand>(type);

    // Add aliased type (operator).
    handleQualType(hs, decl->getUnderlyingType(), true);

    // Add equal sign (operator).
    hs.add<Halstead::OperatorOperator>(BinaryOperatorKind::BO_Assign);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitTypedefDecl(const clang::TypedefDecl* decl)
{
  ASTContext* con = rMyMetrics.getASTContext();
  assert(con && "ASTContext should always be available.");

  // Get function in which this decl is declared in
  if (const DeclContext* f = decl->getParentFunctionOrMethod())
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // Add typedef keyword (operator).
    hs.add<Halstead::TypedefOperator>();

    // Add new type (operand).
    if (const Type* type = con->getTypeDeclType(decl).getTypePtr())
      hs.add<Halstead::TypeOperand>(type);

    // Add original type (operator).
    handleQualType(hs, decl->getUnderlyingType(), true);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitFriendDecl(const clang::FriendDecl* decl)
{
  if (const DeclContext* f = decl->getParentFunctionOrMethod())
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // Add 'friend' keyword (operator).
    hs.add<Halstead::FriendOperator>();

    // Add type (operand).
    // Note: The implementation does not catch the possible 'class' keyword after 'friend'.
    if (const TypeSourceInfo* t = decl->getFriendType())
    {
      if (const Type* type = t->getType().getTypePtr())
        hs.add<Halstead::TypeOperand>(type);
    }
    else if (const NamedDecl* fd = decl->getFriendDecl())
    {
      if (CXXMethodDecl::classof(fd))
      {
        handleMethodRelatedHalsteadStuff(hs, cast<CXXMethodDecl>(fd));
        handleFunctionRelatedHalsteadStuff(hs, cast<FunctionDecl>(fd));
      }
      else if (FunctionDecl::classof(fd))
        handleFunctionRelatedHalsteadStuff(hs, cast<FunctionDecl>(fd));
    }

    // Note: Local classes cannot contain template friends, thus there is no need to handle that.
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitObjCMethodDecl(const clang::ObjCMethodDecl* decl)
{
  // Add the current node.
  HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[decl].hsStorage;

  // Handle the return type operator.
  handleQualType(hs, decl->getReturnType(), true);

  // Add the method name as a operand.
  hs.add<Halstead::ValueDeclOperand>(decl);

  // Add the + or - method operators.
  if (decl->isClassMethod())
    hs.add<Halstead::ObjCClassMethodOperator>();
  else
    hs.add<Halstead::ObjCInstanceMethodOperator>();
    
  return true;
}

bool ClangMetrics::NodeVisitor::VisitDecl(const Decl* decl)
{
  if (decl->isFunctionOrFunctionTemplate())
  {
    this->pCurrentFunctionDecl.push(decl);
  }

  // Add it to the GMD.
  rMyMetrics.rMyGMD.addDecl(decl);

  // Add the places where there is sure to be code.
  rMyMetrics.rMyGMD.addCodeLine(decl->getLocStart());
  rMyMetrics.rMyGMD.addCodeLine(decl->getLocation());
  rMyMetrics.rMyGMD.addCodeLine(decl->getLocEnd());

  // Handle semicolons.
  const SourceManager& sm = rMyMetrics.getASTContext()->getSourceManager();
  SourceLocation semiloc = findSemiAfterLocation(decl->getLocEnd(), rMyMetrics.getASTContext(), false);

  if (dyn_cast_or_null<FunctionDecl>(decl->getParentFunctionOrMethod()) || dyn_cast_or_null<ObjCMethodDecl>(decl->getParentFunctionOrMethod()) )
  {
    handleSemicolon(sm, decl->getParentFunctionOrMethod(), semiloc);
  }

  return true;
}

void ClangMetrics::NodeVisitor::VisitEndDecl(const Decl* decl)
{
  if (decl->isFunctionOrFunctionTemplate())
  {
    if(!this->pCurrentFunctionDecl.empty())
      this->pCurrentFunctionDecl.pop();
  }
}

bool ClangMetrics::NodeVisitor::VisitStmt(const Stmt* stmt)
{
  // Add places where there is sure to be code.
  rMyMetrics.rMyGMD.addCodeLine(stmt->getLocStart());
  rMyMetrics.rMyGMD.addCodeLine(stmt->getLocEnd());

  // Increase NOS in the Range containing this statement.
  // We are only interested in 'true' statements, not subexpressions.
  if (!Expr::classof(stmt))
  {
    if (const GlobalMergeData::Range* range = rMyMetrics.rMyGMD.getParentRange(stmt->getLocStart()))
      ++range->numberOfStatements;
  }

  // Handle semicolons.
  const SourceManager& sm = rMyMetrics.getASTContext()->getSourceManager();
  SourceLocation semiloc = findSemiAfterLocation(stmt->getLocEnd(), rMyMetrics.getASTContext(), false);
  handleSemicolon(sm, getFunctionContextFromStmt(*stmt), semiloc);
  return true;
}

bool ClangMetrics::NodeVisitor::VisitDeclStmt(const clang::DeclStmt* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    if (stmt->isSingleDecl())
    {
      // Handle the qualified type of single decl.
      if (const Decl* sd = stmt->getSingleDecl())
      {
        if (ValueDecl::classof(sd))
          handleQualType(hs, cast<ValueDecl>(sd)->getType(), true);
      }

      return true;
    }

    const DeclGroup& group = stmt->getDeclGroup().getDeclGroup();
    if (group.size() == 0)
      return true;

    if (!group[0] || !ValueDecl::classof(group[0]))
      return true;

    // Handle the qualified type of the first decl.
    handleQualType(hs, cast<ValueDecl>(group[0])->getType(), true);

    // Only need to check for pointer and reference decl syntax.
    for (unsigned i = 1; i < group.size(); ++i)
    {
      const ValueDecl* cv = dyn_cast_or_null<const ValueDecl>(group[i]);
      if (!cv)
        continue;

      QualType type = cv->getType();

      if (type->isPointerType())
      {
        hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::POINTER);
      }
      else if (type->isReferenceType())
      {
        if (type->isLValueReferenceType())
          hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::LV_REF);
        else if (type->isRValueReferenceType())
          hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::RV_REF);
      }

      // Add commas.
      hs.add<Halstead::DeclSeparatorCommaOperator>();
    }

    // Declaration names are already declared in their respective function.
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitLambdaExpr(const clang::LambdaExpr* stmt)
{
  CXXRecordDecl* rd = stmt->getLambdaClass();
  CXXMethodDecl* md = stmt->getCallOperator();
  if (rd && md)
  {
    TraverseCXXRecordDecl(rd);
    TraverseCXXMethodDecl(md);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitIfStmt(const clang::IfStmt* stmt)
{
  increaseMcCCStmt(stmt);

  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;
    hs.add<Halstead::IfOperator>();

    if (stmt->getElse())
      hs.add<Halstead::ElseOperator>();
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitForStmt(const clang::ForStmt* stmt)
{
  increaseMcCCStmt(stmt);

  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::ForOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitCXXForRangeStmt(const clang::CXXForRangeStmt* stmt)
{
  increaseMcCCStmt(stmt);

  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::ForOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitWhileStmt(const clang::WhileStmt* stmt)
{
  increaseMcCCStmt(stmt);

  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::WhileOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitDoStmt(const clang::DoStmt* stmt)
{
  increaseMcCCStmt(stmt);

  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;
    hs.add<Halstead::DoOperator>();
    hs.add<Halstead::WhileOperator>();
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitSwitchStmt(const clang::SwitchStmt* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::SwitchOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitCaseStmt(const clang::CaseStmt* stmt)
{
  increaseMcCCStmt(stmt);

  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::CaseOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitDefaultStmt(const clang::DefaultStmt* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::DefaultCaseOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitBreakStmt(const clang::BreakStmt* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::BreakOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitContinueStmt(const clang::ContinueStmt* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::ContinueOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitLabelStmt(const clang::LabelStmt* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    if (const LabelDecl* d = stmt->getDecl())
      rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::LabelDeclOperand>(d);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitGotoStmt(const clang::GotoStmt* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // Add goto keyword (operator).
    hs.add<Halstead::GotoOperator>();

    // Add its label (operand).
    if (const LabelDecl* ld = stmt->getLabel())
      hs.add<Halstead::LabelDeclOperand>(ld);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitCXXTryStmt(const clang::CXXTryStmt* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::TryOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitObjCBridgedCastExpr(const clang::ObjCBridgedCastExpr* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;
    hs.add<Halstead::BridgedCastOperator>();
    handleQualType(hs, stmt->getTypeAsWritten(), true);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitObjCBoxedExpr(const clang::ObjCBoxedExpr* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;
    hs.add<Halstead::ObjCBoxedOperator>();
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitObjCAtTryStmt(const clang::ObjCAtTryStmt* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::TryOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitObjCAtFinallyStmt(const clang::ObjCAtFinallyStmt* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::FinallyOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitCXXCatchStmt(const clang::CXXCatchStmt* stmt)
{
  increaseMcCCStmt(stmt);

  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::CatchOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitObjCAtCatchStmt(const clang::ObjCAtCatchStmt* stmt)
{
  increaseMcCCStmt(stmt);

  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::CatchOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitCXXThrowExpr(const clang::CXXThrowExpr* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::ThrowOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitObjCAtThrowStmt(const clang::ObjCAtThrowStmt* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::ThrowOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitReturnStmt(const clang::ReturnStmt* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::ReturnOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitUnaryExprOrTypeTraitExpr(const UnaryExprOrTypeTraitExpr* stmt)
{
  // Only increase Halstead operator count in case of 'alignof' and 'sizeof'.
  if (stmt->getKind() == UnaryExprOrTypeTrait::UETT_AlignOf || stmt->getKind() == UnaryExprOrTypeTrait::UETT_SizeOf)
  {
    // Find parent function
    const Decl* parent = getDeclFromStmt(*stmt);
    if(!parent)
      return true;

    if (const DeclContext* f = parent->getParentFunctionOrMethod())
    {
      // Increase operator count of said function.
      if (stmt->getKind() == UnaryExprOrTypeTrait::UETT_AlignOf)
        rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::AlignofOperator>();
      else
        rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::SizeofOperator>();
    }
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitDeclRefExpr(const DeclRefExpr* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    // Functions are handled by VisitCallExpr(), because after declaration a function can also become
    // an operand if used as an argument to another function.
    // Here we only handle ValueDecls that are always operands.
    if (const ValueDecl* decl = stmt->getDecl())
    {
      if (!FunctionDecl::classof(decl))
        rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::ValueDeclOperand>(decl);
    }

    // Add Halstead operators/operands if this is a nested name.
    handleNestedName(f, stmt->getQualifier());
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitObjCAtSynchronizedStmt(const ObjCAtSynchronizedStmt* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;
    hs.add<Halstead::ObjCSynchronizeOperator>();
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitCallExpr(const clang::CallExpr* stmt)
{
  // UDLs are handled at the different "literal" callbacks, so we must skip those here.
  // TODO: Template UDLs are still not handled (or at least not tested).
  if (UserDefinedLiteral::classof(stmt))
    return true;

  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // If this is a call to an overloaded operator, we must hande it "syntactically" as if it was a built-in operator.
    // Clang only classifies "operator call expression" as such if the "traditional" infix syntax is used.
    // For example for 'a == b', the '==' is an CXXOperatorCallExpr whereas in the case of 'a.operator==(b)' it is a method call.
    if (CXXOperatorCallExpr::classof(stmt))
    {
      UnifiedCXXOperator op = convertOverloadedOperator(*cast<CXXOperatorCallExpr>(stmt));
      if (op.getType() == UnifiedCXXOperator::UNKNOWN)
      {
        // Handle as if this wasn't an operator call.
        if (const FunctionDecl* callee = stmt->getDirectCallee())
          hs.add<Halstead::FunctionOperator>(callee);
      }
      else
      {
        hs.add<Halstead::OperatorOperator>(op);
      }
    }
    else /* not an operator call */
    {
      // Add the callee (operator).
      if (const FunctionDecl* callee = stmt->getDirectCallee())
        hs.add<Halstead::FunctionOperator>(callee);
    }

    // Iterate over the arguments.
    for (const Expr* arg : stmt->arguments())
    {
      // As a top AST node, 'arg' cannot be a DeclRefExpr to a FunctionDecl, as there's always at least
      // an implicit conversion (function to pointer decay) before. This means we can call the handler as it is
      // without checking the type of 'arg'.
      handleCallArgs(hs, arg);
    }

    // Add the required commas (argument count - 1).
    for (unsigned i = 1; i < stmt->getNumArgs(); ++i)
      hs.add<Halstead::DeclSeparatorCommaOperator>();
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitCXXConstructExpr(const clang::CXXConstructExpr* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // Iterate over the arguments.
    for (const Expr* arg : stmt->arguments())
    {
      // As a top AST node, 'arg' cannot be a DeclRefExpr to a FunctionDecl, as there's always at least
      // an implicit conversion (function to pointer decay) before. This means we can call the lambda as it is
      // without checking the type of 'arg'.
      handleCallArgs(hs, arg);
    }

    // Add the required commas (argument count - 1).
    for (unsigned i = 1; i < stmt->getNumArgs(); ++i)
      hs.add<Halstead::DeclSeparatorCommaOperator>();
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitMemberExpr(const clang::MemberExpr* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // Functions are handled by VisitCallExpr(), because after declaration a function can also become
    // an operand if used as an argument to another function.
    // Here we only handle ValueDecls that are always operands.
    if (const ValueDecl* decl = stmt->getMemberDecl())
    {
      if (!FunctionDecl::classof(decl))
        hs.add<Halstead::ValueDeclOperand>(decl);
    }

    // Handle arrow/dot.
    if (stmt->isArrow())
      hs.add<Halstead::OperatorOperator>(UnifiedCXXOperator::ARROW);
    else
      hs.add<Halstead::OperatorOperator>(UnifiedCXXOperator::DOT);

    // Add Halstead operators/operands if this is a nested name.
    handleNestedName(f, stmt->getQualifier());
    
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitCXXThisExpr(const clang::CXXThisExpr* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    // Add this keyword.
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::ThisExprOperator>();
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitCXXNewExpr(const clang::CXXNewExpr* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

    // Add new keyword (operator).
    hs.add<Halstead::NewExprOperator>();

    // Add the type allocated by the new keyword.
    hs.add<Halstead::TypeOperator>(stmt->getAllocatedType().getTypePtr());

    // Note: placement params are handled automatically by the visitor.
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitCXXDeleteExpr(const clang::CXXDeleteExpr* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::DeleteExprOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitObjCEncodeExpr(const clang::ObjCEncodeExpr* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::EncodeExprOperator>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitExplicitCastExpr(const clang::ExplicitCastExpr* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;
    
    // Add the cast operator itself based on its kind.
    if (CStyleCastExpr::classof(stmt))
      hs.add<Halstead::CStyleCastOperator>();
    else if (CXXStaticCastExpr::classof(stmt))
      hs.add<Halstead::StaticCastOperator>();
    else if (CXXConstCastExpr::classof(stmt))
      hs.add<Halstead::ConstCastOperator>();
    else if (CXXReinterpretCastExpr::classof(stmt))
      hs.add<Halstead::ReinterpretCastOperator>();
    else if (CXXDynamicCastExpr::classof(stmt))
      hs.add<Halstead::DynamicCastOperator>();
    else if (CXXFunctionalCastExpr::classof(stmt))
      hs.add<Halstead::FunctionalCastOperator>();

    // Add type.
    handleQualType(hs, stmt->getTypeAsWritten(), true);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitIntegerLiteral(const IntegerLiteral* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    if (auto udl = searchForParent<UserDefinedLiteral>(stmt))
      rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::UserDefinedLiteralOperand>(udl, stmt);
    else
      rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::IntegerLiteralOperand>(stmt);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitFloatingLiteral(const FloatingLiteral* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    if (auto udl = searchForParent<UserDefinedLiteral>(stmt))
      rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::UserDefinedLiteralOperand>(udl, stmt);
    else
      rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::FloatingLiteralOperand>(stmt);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitCharacterLiteral(const CharacterLiteral* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    if (auto udl = searchForParent<UserDefinedLiteral>(stmt))
      rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::UserDefinedLiteralOperand>(udl, stmt);
    else
      rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::CharacterLiteralOperand>(stmt);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitStringLiteral(const StringLiteral* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    if (auto udl = searchForParent<UserDefinedLiteral>(stmt))
      rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::UserDefinedLiteralOperand>(udl, stmt);
    else
      rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::StringLiteralOperand>(stmt);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitObjCBoolLiteralExpr(const ObjCBoolLiteralExpr* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::ObjCBoolLiteralOperand>(stmt);

  return true;
}

bool ClangMetrics::NodeVisitor::VisitCXXBoolLiteralExpr(const CXXBoolLiteralExpr* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::BoolLiteralOperand>(stmt);
  }
    

  return true;
}

bool ClangMetrics::NodeVisitor::VisitCXXNullPtrLiteralExpr(const CXXNullPtrLiteralExpr* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::NullptrLiteralOperand>();

  return true;
}

bool ClangMetrics::NodeVisitor::VisitObjCMessageExpr(const clang::ObjCMessageExpr* stmt)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*stmt))
  {
    HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;
    hs.add<Halstead::ObjCMessageOperator>();
    hs.add<Halstead::MessageSelectorOperand>(stmt);
        
    const QualType qt = stmt->getReceiverType();     
    handleQualType(hs, qt, false);
        
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitBinaryOperator(const BinaryOperator* op)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*op))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::OperatorOperator>(op->getOpcode());

  // Increase McCC only if this is a logical and/or operator.
  if (op->getOpcode() == BinaryOperatorKind::BO_LAnd || op->getOpcode() == BinaryOperatorKind::BO_LOr)
  {
    increaseMcCCStmt(op);
  }

  return true;
}

bool ClangMetrics::NodeVisitor::VisitUnaryOperator(const clang::UnaryOperator* op)
{
  if (const DeclContext* f = getFunctionContextFromStmt(*op))
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::OperatorOperator>(op->getOpcode());

  return true;
}





// ----------------------------------------------- HELPER FUNCTIONS -----------------------------------------------

void ClangMetrics::NodeVisitor::increaseMcCCStmt(const Stmt* stmt)
{
  // Get the SourceManager.
  const SourceManager& sm = rMyMetrics.getASTContext()->getSourceManager();

  // Increase per file McCC.
  ++rMyMetrics.myMcCCByFiles[sm.getFileID(stmt->getLocStart())];

  const Decl* parentDecl = getDeclFromStmt(*stmt);

  // Check whether we've got a parent. Return if not.
  if (!parentDecl)
    return;

  // Check whether it's a function or not. Increase the McCC by one if it is.
  if (clang::ObjCMethodDecl::classof(parentDecl))
    ++rMyMetrics.myFunctionMetrics[cast<ObjCMethodDecl>(parentDecl)].McCC;
  else if(clang::FunctionDecl::classof(parentDecl))
    ++rMyMetrics.myFunctionMetrics[cast<FunctionDecl>(parentDecl)].McCC;

}

const Decl* ClangMetrics::NodeVisitor::getDeclFromStmt(const Stmt& stmt)
{
  ASTContext* con = rMyMetrics.getASTContext();
  assert(con && "ASTContext should always be available.");

  auto parents = con->getParents(stmt);

  auto it = parents.begin();
  if (it == parents.end())
    return nullptr;

  const Decl* parentDecl = it->get<Decl>();
  if (parentDecl)
    return parentDecl;

  const Stmt* parentStmt = it->get<Stmt>();
  if (parentStmt)
    return getDeclFromStmt(*parentStmt);

  return nullptr;
}

const clang::DeclContext* ClangMetrics::NodeVisitor::getFunctionContextFromStmt(const clang::Stmt& stmt)
{
  if (!pCurrentFunctionDecl.empty())
  {
    const Decl* dp = pCurrentFunctionDecl.top();
    {
      if (DeclContext::classof(dp))
      {
        return cast<DeclContext>(dp);
      }
    }
  }

  // If no function was found, nullptr will be returned.
  return nullptr;
}

void ClangMetrics::NodeVisitor::handleNestedName(const clang::DeclContext* f, const clang::NestedNameSpecifier* nns)
{
  HalsteadStorage& hs = rMyMetrics.myFunctionMetrics[f].hsStorage;

  while (nns)
  {
    // Add operand based on kind.
    switch (nns->getKind())
    {
    case NestedNameSpecifier::Identifier:
      // TODO: write this
      break;

    case NestedNameSpecifier::Namespace:
      hs.add<Halstead::NamespaceNameOperator>(nns->getAsNamespace());
      break;

    case NestedNameSpecifier::NamespaceAlias:
      hs.add<Halstead::NamespaceNameOperator>(nns->getAsNamespaceAlias());
      break;

    case NestedNameSpecifier::TypeSpec:
      hs.add<Halstead::TypeOperator>(nns->getAsType());
      /*[[fallthrough]];*/

    case NestedNameSpecifier::TypeSpecWithTemplate:
      // TODO: Additional processing based on template arguments.
      break;
    }

    // Add the scope resolution operator.
    hs.add<Halstead::ScopeResolutionOperator>();

    // Continue with prefix.
    nns = nns->getPrefix();
  }
}

void ClangMetrics::NodeVisitor::handleQualType(HalsteadStorage& hs, const clang::QualType& qtype, bool isOperator)
{
  // Desugar the type of any "internal" qualifiers (eg.: reference to const) where the qualifier
  // is hidden as part of the type itself.
  if (const Type* type = qtype.getTypePtrOrNull())
  {
    // Short-circuit: If it's a decltype, the expression inside is already handled, we only
    // add a decltype keyword without expanding the type itself.
    if (DecltypeType::classof(type))
    {
      hs.add<Halstead::DecltypeOperator>();
    }
    else
    {
      if (type->isPointerType())
      {
        hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::POINTER);
        handleQualType(hs, cast<PointerType>(type->getCanonicalTypeInternal())->getPointeeType(), isOperator);
      }
      else if (type->isReferenceType())
      {
        if (type->isLValueReferenceType())
          hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::LV_REF);
        else if (type->isRValueReferenceType())
          hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::RV_REF);

        handleQualType(hs, cast<ReferenceType>(type->getCanonicalTypeInternal())->getPointeeTypeAsWritten(), isOperator);
      }
      else if (type->isMemberPointerType())
      {
        hs.add<Halstead::ScopeResolutionOperator>();
        hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::POINTER);

        handleQualType(hs, cast<MemberPointerType>(type->getCanonicalTypeInternal())->getPointeeType(), isOperator);
      }
      else if (AutoType::classof(type))
      {
        AutoTypeKeyword word = cast<AutoType>(type)->getKeyword();
        switch (word)
        {
        case clang::AutoTypeKeyword::Auto:
          hs.add<Halstead::AutoOperator>();
          break;

        case clang::AutoTypeKeyword::DecltypeAuto:
          hs.add<Halstead::DecltypeOperator>();
          hs.add<Halstead::AutoOperator>();
          break;
        }
      }
      else
      {
        if (isOperator)
          hs.add<Halstead::TypeOperator>(type);
        else
          hs.add<Halstead::TypeOperand>(type);
      }
    } // decltype else
  }

  // Handle const
  if (qtype.isConstQualified())
    hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::CONST);

  // Handle volatile
  if (qtype.isVolatileQualified())
    hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::VOLATILE);
}

void ClangMetrics::NodeVisitor::handleCallArgs(HalsteadStorage& hs, const Stmt* arg)
{
  // The value of arg can be null, handle this.
  if (!arg)
    return;

  // Iterate over the children of the statement in the AST.
  for (const Stmt* sub : arg->children())
  {
    // It is possible that sub here is null.
    if (!sub)
      continue;

    // We're only interested in DeclRefExprs.
    if (DeclRefExpr::classof(sub))
    {
      if (const ValueDecl* argDecl = cast<DeclRefExpr>(sub)->getDecl())
      {
        // Every ValueDecl is already added by VisitDeclRefExpr() except FunctionDecls, because in this case
        // they are considered operands - we add them here manually.
        if (FunctionDecl::classof(argDecl))
          hs.add<Halstead::ValueDeclOperand>(argDecl);
      }
    }

    // Recursion into the children of the child.
    if (sub->child_begin() != sub->child_end())
      handleCallArgs(hs, sub);
  }
}

UnifiedCXXOperator ClangMetrics::NodeVisitor::convertOverloadedOperator(const CXXOperatorCallExpr& stmt)
{
  // Get the operator's kind
  OverloadedOperatorKind op = stmt.getOperator();

  // Add as if it was a built-in operator.
  UnifiedCXXOperator ot = UnifiedCXXOperator::UNKNOWN;
  switch (op)
  {
    // "Primitive" cases:
  case clang::OO_Tilde:               ot = UnifiedCXXOperator::BITWISE_NEGATION;        break;
  case clang::OO_Exclaim:             ot = UnifiedCXXOperator::LOGICAL_NEGATION;        break;
  case clang::OO_Pipe:                ot = UnifiedCXXOperator::BITWISE_OR;              break;
  case clang::OO_Equal:               ot = UnifiedCXXOperator::ASSIGNMENT;              break;
  case clang::OO_Less:                ot = UnifiedCXXOperator::LESS;                    break;
  case clang::OO_Greater:             ot = UnifiedCXXOperator::GREATER;                 break;
  case clang::OO_PlusEqual:           ot = UnifiedCXXOperator::COMPOUND_ADDITION;       break;
  case clang::OO_MinusEqual:          ot = UnifiedCXXOperator::COMPOUND_SUBTRACTION;    break;
  case clang::OO_StarEqual:           ot = UnifiedCXXOperator::COMPOUND_MULTIPLICATION; break;
  case clang::OO_SlashEqual:          ot = UnifiedCXXOperator::COMPOUND_DIVISION;       break;
  case clang::OO_PercentEqual:        ot = UnifiedCXXOperator::COMPOUND_MODULO;         break;
  case clang::OO_CaretEqual:          ot = UnifiedCXXOperator::COMPOUND_BITWISE_XOR;    break;
  case clang::OO_AmpEqual:            ot = UnifiedCXXOperator::COMPOUND_BITWISE_AND;    break;
  case clang::OO_PipeEqual:           ot = UnifiedCXXOperator::COMPOUND_BITWISE_OR;     break;
  case clang::OO_LessLess:            ot = UnifiedCXXOperator::LEFT_SHIFT;              break;
  case clang::OO_GreaterGreater:      ot = UnifiedCXXOperator::RIGHT_SHIFT;             break;
  case clang::OO_LessLessEqual:       ot = UnifiedCXXOperator::COMPOUND_LEFT_SHIFT;     break;
  case clang::OO_GreaterGreaterEqual: ot = UnifiedCXXOperator::COMPOUND_RIGHT_SHIFT;    break;
  case clang::OO_EqualEqual:          ot = UnifiedCXXOperator::EQUAL;                   break;
  case clang::OO_ExclaimEqual:        ot = UnifiedCXXOperator::NOT_EQUAL;               break;
  case clang::OO_LessEqual:           ot = UnifiedCXXOperator::LESS_EQUAL;              break;
  case clang::OO_GreaterEqual:        ot = UnifiedCXXOperator::GREATER_EQUAL;           break;
  case clang::OO_AmpAmp:              ot = UnifiedCXXOperator::LOGICAL_AND;             break;
  case clang::OO_PipePipe:            ot = UnifiedCXXOperator::LOGICAL_OR;              break;
  case clang::OO_Slash:               ot = UnifiedCXXOperator::DIVISION;                break;
  case clang::OO_Percent:             ot = UnifiedCXXOperator::MODULO;                  break;
  case clang::OO_Caret:               ot = UnifiedCXXOperator::BITWISE_XOR;             break;
  case clang::OO_Comma:               ot = UnifiedCXXOperator::COMMA;                   break;
  case clang::OO_ArrowStar:           ot = UnifiedCXXOperator::POINTER_TO_MEMBER_ARROW; break;
  case clang::OO_Arrow:               ot = UnifiedCXXOperator::ARROW;                   break;
  case clang::OO_Subscript:           ot = UnifiedCXXOperator::SUBSCRIPT;               break;

    // Special cases where additional handling is needed.
  case clang::OO_Plus:
    if (stmt.getNumArgs() == 2)
      ot = UnifiedCXXOperator::ADDITION;
    else
      ot = UnifiedCXXOperator::UNARY_PLUS;
    break;

  case clang::OO_Minus:
    if (stmt.getNumArgs() == 2)
      ot = UnifiedCXXOperator::SUBTRACTION;
    else
      ot = UnifiedCXXOperator::UNARY_MINUS;
    break;

  case clang::OO_Star:
    if (stmt.getNumArgs() == 2)
      ot = UnifiedCXXOperator::MULTIPLICATION;
    else
      ot = UnifiedCXXOperator::DEREFERENCE;
    break;

  case clang::OO_Amp:
    if (stmt.getNumArgs() == 2)
      ot = UnifiedCXXOperator::BITWISE_AND;
    else
      ot = UnifiedCXXOperator::ADDRESS_OF;
    break;

  case clang::OO_PlusPlus:
    if (const FunctionDecl* dc = stmt.getDirectCallee())
    {
      if (dc->getNumParams() == 1)
        ot = UnifiedCXXOperator::POSTFIX_INCREMENT;  // postfix
      else
        ot = UnifiedCXXOperator::PREFIX_INCREMENT;   // prefix
    }
    break;

  case clang::OO_MinusMinus:
    if (const FunctionDecl* dc = stmt.getDirectCallee())
    {
      if (dc->getNumParams() == 1)
        ot = UnifiedCXXOperator::POSTFIX_DECREMENT;  // postfix
      else
        ot = UnifiedCXXOperator::PREFIX_DECREMENT;   // prefix
    }
    break;
  }

  return ot;
}

void ClangMetrics::NodeVisitor::handleSemicolon(const SourceManager& sm, const DeclContext* f, SourceLocation semiloc)
{
  if (!f || semiloc.isInvalid())
    return;
  
  unsigned line   = sm.getExpansionLineNumber(semiloc);
  unsigned column = sm.getExpansionColumnNumber(semiloc);

  unsigned sl, sc;
  unsigned el, ec;

  FileID file;

  if (clang::ObjCMethodDecl::classofKind(f->getDeclKind()))
  {
    const ObjCMethodDecl* fd = cast<ObjCMethodDecl>(f);

    sl = sm.getExpansionLineNumber(fd->getLocStart());
    sc = sm.getExpansionColumnNumber(fd->getLocStart());
    el = sm.getExpansionLineNumber(fd->getLocEnd());
    ec = sm.getExpansionColumnNumber(fd->getLocEnd());

    file = sm.getFileID(fd->getLocStart());
  }
  else if (clang::FunctionDecl::classofKind(f->getDeclKind()))
  {
    const FunctionDecl* fd = cast<FunctionDecl>(f);

    sl = sm.getExpansionLineNumber(fd->getLocStart());
    sc = sm.getExpansionColumnNumber(fd->getLocStart());
    el = sm.getExpansionLineNumber(fd->getLocEnd());
    ec = sm.getExpansionColumnNumber(fd->getLocEnd());

    file = sm.getFileID(fd->getLocStart());
  }
  else
  {
    return;
  }

  // Ensure that the semicolon we found is within the range of the function.
  if (!(sl <= line && line <= el))
    return;

  if (sl == line && !(sc < column))
    return;

  if (el == line && !(column < ec))
    return;

  // If this is the first time we see this semicolon, add it as an operator
  // and register it, so that it won't be counted multiple times.
  auto it = rMyMetrics.mySemicolonLocations.find({ file, line, column });
  if (it == rMyMetrics.mySemicolonLocations.end())
  {
    rMyMetrics.myFunctionMetrics[f].hsStorage.add<Halstead::SemicolonOperator>();
    rMyMetrics.mySemicolonLocations.emplace(file, line, column);
  }
}

void ClangMetrics::NodeVisitor::handleFunctionRelatedHalsteadStuff(HalsteadStorage& hs, const clang::FunctionDecl* decl)
{
  // Ensure pointer validity.
  if (!decl)
    return;

  // A function always has a return type which can be considered as an operator (Halstead), unless it's
  // a constructor, a destructor or a cast operator (in which case the typename is considered part of the operator keyword).
  if (!CXXConstructorDecl::classof(decl) && !CXXDestructorDecl::classof(decl) && !CXXConversionDecl::classof(decl))
    handleQualType(hs, decl->getReturnType(), true);

  // A function always has a name, which can be considered as an operand (Halstead).
  hs.add<Halstead::ValueDeclOperand>(decl);

  // Check for alternative function declaration (trailing return).
  if (const FunctionProtoType* fpt = decl->getType()->getAs<FunctionProtoType>())
  {
    if (fpt->hasTrailingReturn())
    {
      hs.add<Halstead::AutoOperator>();
      hs.add<Halstead::TrailingReturnArrowOperator>();
    }
  }

  // Check for variadicness.
  if (decl->isVariadic())
    hs.add<Halstead::VariadicEllipsisOperator>();

  // TODO: Check for constexpr.

  // Handle inline.
  if (decl->isInlineSpecified())
    hs.add<Halstead::InlineOperator>();

  // Handle defaulted and deleted functions.
  if (decl->isExplicitlyDefaulted())
    hs.add<Halstead::DefaultFunctionOperator>();

  if (decl->isDeletedAsWritten())
    hs.add<Halstead::DeleteFunctionOperator>();

  // Handle storage specifiers.
  // TODO: Currently only 'static' is handled. Implement the rest!
  // Note: Static member functions are also considered by Clang to have static storage duration, thus
  // this comparision also checks for those.
  if (decl->getStorageClass() == StorageClass::SC_Static)
    hs.add<Halstead::StaticOperator>();

  // Add commas between parameters. Comma count is param count - 1.
  for (unsigned i = 1; i < decl->getNumParams(); ++i)
    hs.add<Halstead::DeclSeparatorCommaOperator>();
}

void ClangMetrics::NodeVisitor::handleMethodRelatedHalsteadStuff(HalsteadStorage& hs, const clang::CXXMethodDecl* decl)
{
  // Ensure pointer validity.
  if (!decl)
    return;

  if (decl->isConst())
    hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::CONST);

  if (decl->isVolatile())
    hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::VOLATILE);

  switch (decl->getRefQualifier())
  {
  case clang::RQ_LValue:
    hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::LV_REF);
    break;
  case clang::RQ_RValue:
    hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::RV_REF);
    break;
  }

  // Handle 'explicit' keyword for constructors and cast operators.
  if (CXXConstructorDecl::classof(decl) && cast<CXXConstructorDecl>(decl)->isExplicitSpecified())
    hs.add<Halstead::ExplicitOperator>();

  if (CXXConversionDecl::classof(decl) && cast<CXXConversionDecl>(decl)->isExplicitSpecified())
    hs.add<Halstead::ExplicitOperator>();

  // Handle virtualness.
  if (decl->isVirtualAsWritten())
    hs.add<Halstead::VirtualOperator>();

  if (decl->isPure())
    hs.add<Halstead::PureVirtualDeclarationOperator>();
}


namespace
{
  // COPIED FROM http://clang.llvm.org/doxygen/Transforms_8cpp_source.html

  /// \brief \arg Loc is the end of a statement range. This returns the location
  /// of the semicolon following the statement.
  /// If no semicolon is found or the location is inside a macro, the returned
  /// source location will be invalid.
  SourceLocation findSemiAfterLocation(SourceLocation loc,
                        ASTContext *Ctx,
                        bool IsDecl) {
  const SourceManager &SM = Ctx->getSourceManager();
  if (loc.isMacroID()) {
    if (!Lexer::isAtEndOfMacroExpansion(loc, SM, Ctx->getLangOpts(), &loc))
    return SourceLocation();
  }
  loc = Lexer::getLocForEndOfToken(loc, /*Offset=*/0, SM, Ctx->getLangOpts());
 
  // Break down the source location.
  std::pair<FileID, unsigned> locInfo = SM.getDecomposedLoc(loc);
 
  // Try to load the file buffer.
  bool invalidTemp = false;
  StringRef file = SM.getBufferData(locInfo.first, &invalidTemp);
  if (invalidTemp)
    return SourceLocation();
 
  const char *tokenBegin = file.data() + locInfo.second;
 
  // Lex from the start of the given location.
  Lexer lexer(SM.getLocForStartOfFile(locInfo.first),
        Ctx->getLangOpts(),
        file.begin(), tokenBegin, file.end());
  Token tok;
  lexer.LexFromRawLexer(tok);
  if (tok.isNot(tok::semi)) {
    if (!IsDecl)
    return SourceLocation();
    // Declaration may be followed with other tokens; such as an __attribute,
    // before ending with a semicolon.
    return findSemiAfterLocation(tok.getLocation(), Ctx, /*IsDecl*/true);
  }
 
  return tok.getLocation();
  }
}
