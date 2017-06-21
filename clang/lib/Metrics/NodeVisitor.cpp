#include "NodeVisitor.h"

// Use clang namespace for simplicity
using namespace clang;
using namespace metrics::detail;

namespace
{
	// Helper function for finding the semicolon at the end of statements.
	SourceLocation findSemiAfterLocation(SourceLocation loc, ASTContext& con, bool isDecl);
}

bool ClangMetricsAction::NodeVisitor::VisitCXXRecordDecl(const CXXRecordDecl* decl)
{
	// Calculating Halstead for functions only
	if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

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

	// Skip the rest if this is only a forward declaration
	if (!decl->hasDefinition())
		return true;

	// Add the current node
	rMyAction.myClasses.insert(decl);

	// If this class has an "outer class" (so this is an inner class), we add this class to the list of
	// inner classes of the outer class
	if (const CXXRecordDecl* outerClass = static_cast<const CXXRecordDecl*>(decl->getParent()))
	{
		// If this is indeed a class and not a namespace, we add this class as an inner class
		if (CXXRecordDecl::classofKind(outerClass->getKind()))
			rMyAction.myInnerClassesByClasses[outerClass].insert(decl);
	}

	// Check if this class was defined within a function
	if (const FunctionDecl* function = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
	{
		rMyAction.myInsideClassesByFunctions[function].insert(decl);
	}

	// Get the namespace of the class (unless it's the global namespace - no need to store that)
	const DeclContext* parent = decl;
	while (parent = parent->getParent())
	{
		// Store the class by its namespace, then break the loop (no need to look further)
		if (NamespaceDecl::classofKind(parent->getDeclKind()))
		{
			rMyAction.myClassesByNamespaces[static_cast<const NamespaceDecl*>(parent)].insert(decl);
			break;
		}
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitFunctionDecl(const FunctionDecl* decl)
{
	if (decl->isDefined())
	{
		// Add the current node
		rMyAction.myFunctions.insert(decl);
	}

	HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[decl];
	handleFunctionRelatedHalsteadStuff(hs, decl);

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitFunctionTemplateDecl(const clang::FunctionTemplateDecl* decl)
{
	if (decl->isThisDeclarationADefinition())
	{
		// Add template keyword (operator)
		rMyAction.myHalsteadByFunctions[decl->getTemplatedDecl()].add<Halstead::TemplateOperator>();
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitTemplateTypeParmDecl(const clang::TemplateTypeParmDecl* decl)
{
	if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];
		ASTContext& con = rMyAction.getCompilerInstance().getASTContext();

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

bool ClangMetricsAction::NodeVisitor::VisitNonTypeTemplateParmDecl(const clang::NonTypeTemplateParmDecl* decl)
{
	// Only handle default arguments here. Everything else is done in VisitValueDecl().
	if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

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

bool ClangMetricsAction::NodeVisitor::VisitTemplateTemplateParmDecl(const clang::TemplateTemplateParmDecl* decl)
{
	if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

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
			hs.add<Halstead::TemplateNameOperator>(tmp);
		}
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitCXXMethodDecl(const CXXMethodDecl* decl)
{
	// No need to add the current node - it will be done by VisitFunctionDecl() anyway.

	// Get the class in which the method is defined
	const CXXRecordDecl* parentClass = decl->getParent();

	// Add this method to the set of methods in that class
	rMyAction.myMethodsByClasses[parentClass].insert(decl);

	// Forward to the method handler.
	HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[decl];
	handleMethodRelatedHalsteadStuff(hs, decl);

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitEnumDecl(const EnumDecl* decl)
{
	rMyAction.myEnums.insert(decl);

	// Get the namespace of the enum (unless it's the global namespace - no need to store that)
	const DeclContext* parent = decl;
	while (parent = parent->getParent())
	{
		// Store the enum by its namespace, then break the loop (no need to look further)
		if (NamespaceDecl::classofKind(parent->getDeclKind()))
		{
			rMyAction.myEnumsByNamespaces[static_cast<const NamespaceDecl*>(parent)].insert(decl);
			break;
		}
	}

	// Halstead stuff
	if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

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

bool ClangMetricsAction::NodeVisitor::VisitNamespaceDecl(const NamespaceDecl* decl)
{
	// Add the current node
	rMyAction.myNamespaces.insert(decl);

	// Check if this is an inner namespace
	if (const DeclContext* p = decl->getParent())
	{
		if (NamespaceDecl::classofKind(p->getDeclKind()))
			rMyAction.myInnerNamespacesByNamespaces[static_cast<const NamespaceDecl*>(p)].insert(decl);
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitValueDecl(const ValueDecl* decl)
{
	// Get function in which this decl is declared in
	if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

		// Handle CXXMethodDecl bug (?). For local classes, methods will be identified as ValueDecls at first.
		if (CXXMethodDecl::classof(decl))
		{
			const CXXMethodDecl* mdecl = static_cast<const CXXMethodDecl*>(decl);
			handleMethodRelatedHalsteadStuff(hs, mdecl);
			handleFunctionRelatedHalsteadStuff(hs, mdecl);
		}
		else
		{
			// A value declaration always have an operator (its type).
			// However, we only handle it here if this declaration is not part of
			// a DeclStmt. DeclStmts are handled elsewhere. This is due to multi-decl-single-stmt, like 'int x, y, z;'.
			// (kinda hacky, yeah, but this is clang after all :P)
			// We also don't add a type for enum constant declarations.
			ASTContext& con = rMyAction.getCompilerInstance().getASTContext();
			const Stmt* ds = con.getParents(*decl).begin()->get<Stmt>();
			if (!ds || !DeclStmt::classof(ds))
				if (!EnumConstantDecl::classof(decl))
					handleQualType(hs, decl->getType(), true);

			// A value declaration can have an operand too (its name).
			if (!decl->getDeclName().isEmpty())
				hs.add<Halstead::ValueDeclOperand>(decl);
		}
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitVarDecl(const clang::VarDecl* decl)
{
	// Only handle init syntax and 'static' here. Everything else is done in VisitValueDecl().
	if (decl->hasInit())
	{
		// Get function in which this decl is declared in
		if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
		{
			// Handle initialization.
			switch (decl->getInitStyle())
			{
			case VarDecl::InitializationStyle::CInit:
				rMyAction.myHalsteadByFunctions[f].add<Halstead::OperatorOperator>(BinaryOperatorKind::BO_Assign);
				break;

			case VarDecl::InitializationStyle::CallInit:
				// Unfortunately implicit constructor calls are also considered "CallInit" even if there are no parentheses.
				// To avoid counting these, first check the init expression and branch on its "constructorness".
				{
					const Expr* init = decl->getInit();
					if (CXXConstructExpr::classof(init))
					{
						// Get the parentheses range in the source code. Only add the Halstead operator if it's not invalid.
						SourceRange range = static_cast<const CXXConstructExpr*>(init)->getParenOrBraceRange();
						if (range.isValid())
							rMyAction.myHalsteadByFunctions[f].add<Halstead::ParenthesesInitSyntaxOperator>();
					}
					else
					{
						// If it's not a constructor there's no such problem - the parentheses must be there.
						rMyAction.myHalsteadByFunctions[f].add<Halstead::ParenthesesInitSyntaxOperator>();
					}
				}
				break;

			case VarDecl::InitializationStyle::ListInit:
				rMyAction.myHalsteadByFunctions[f].add<Halstead::BracesInitSyntaxOperator>();
				break;
			}
		}
	}

	// Only need to handle 'static' for local variables.
	// FieldDecls (class members) cannot be static for local classes.
	if (decl->isStaticLocal())
	{
		if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
			rMyAction.myHalsteadByFunctions[f].add<Halstead::StaticOperator>();
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitFieldDecl(const clang::FieldDecl* decl)
{
	// Only handle init and 'mutable' syntax here. Everything else is done in VisitValueDecl().
	if (decl->hasInClassInitializer())
	{
		// Get function in which this decl is declared in
		if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
		{
			// Handle initialization.
			switch (decl->getInClassInitStyle())
			{
			case InClassInitStyle::ICIS_CopyInit:
				rMyAction.myHalsteadByFunctions[f].add<Halstead::OperatorOperator>(BinaryOperatorKind::BO_Assign);
				break;

			case InClassInitStyle::ICIS_ListInit:
				rMyAction.myHalsteadByFunctions[f].add<Halstead::BracesInitSyntaxOperator>();
				break;
			}
		}
	}

	if (decl->isMutable())
	{
		if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
			rMyAction.myHalsteadByFunctions[f].add<Halstead::MutableOperator>();
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitAccessSpecDecl(const clang::AccessSpecDecl* decl)
{
	// Get function in which this decl is declared in
	if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::AccessSpecDeclOperator>(decl);

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitUsingDecl(const clang::UsingDecl* decl)
{
	// Get function in which this decl is declared in
	if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

		// Add using keyword (operator).
		hs.add<Halstead::UsingOperator>();

		// The using keyword also has an operand.
		// TODO: Multiple names? foo::bar? How much does that count?
		hs.add<Halstead::UsingOperand>(decl);
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitUsingDirectiveDecl(const clang::UsingDirectiveDecl* decl)
{
	// Get function in which this decl is declared in
	if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

		// Add using keyword (operator).
		hs.add<Halstead::UsingOperator>();

		// Add namespace keyword (operator).
		hs.add<Halstead::NamespaceOperator>();

		// Add name of the namespace (operand).
		// TODO: Multiple names? foo::bar? How much does that count?
		hs.add<Halstead::NamespaceOperand>(decl->getNominatedNamespace());
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitNamespaceAliasDecl(const clang::NamespaceAliasDecl* decl)
{
	if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

		// Add namespace keyword (operator).
		hs.add<Halstead::NamespaceOperator>();

		// Add alias name (operand).
		hs.add<Halstead::NamespaceOperand>(decl);

		// Add target name (operand).
		hs.add<Halstead::NamespaceOperand>(decl->getAliasedNamespace());

		// Add equal sign (operator) which is part of the alias declaration.
		hs.add<Halstead::OperatorOperator>(BinaryOperatorKind::BO_Assign);
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitTypeAliasDecl(const clang::TypeAliasDecl* decl)
{
	ASTContext& con = rMyAction.getCompilerInstance().getASTContext();

	// Get function in which this decl is declared in
	if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

		// Add using keyword (operator).
		hs.add<Halstead::UsingOperator>();

		// Add alias type (operand).
		hs.add<Halstead::TypeOperand>(con.getTypeDeclType(decl).getTypePtr());

		// Add aliased type (operator).
		handleQualType(hs, decl->getUnderlyingType(), true);

		// Add equal sign (operator).
		hs.add<Halstead::OperatorOperator>(BinaryOperatorKind::BO_Assign);
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitTypedefDecl(const clang::TypedefDecl* decl)
{
	ASTContext& con = rMyAction.getCompilerInstance().getASTContext();

	// Get function in which this decl is declared in
	if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

		// Add typedef keyword (operator).
		hs.add<Halstead::TypedefOperator>();

		// Add new type (operand).
		hs.add<Halstead::TypeOperand>(con.getTypeDeclType(decl).getTypePtr());

		// Add original type (operator).
		handleQualType(hs, decl->getUnderlyingType(), true);
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitFriendDecl(const clang::FriendDecl* decl)
{
	if (const FunctionDecl* f = static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

		// Add 'friend' keyword (operator).
		hs.add<Halstead::FriendOperator>();

		// Add type (operand).
		// Note: The implementation does not catch the possible 'class' keyword after 'friend'.
		if (const TypeSourceInfo* t = decl->getFriendType())
			hs.add<Halstead::TypeOperand>(t->getType().getTypePtr());
		else if (const NamedDecl* fd = decl->getFriendDecl())
		{
			if (CXXMethodDecl::classof(fd))
			{
				handleMethodRelatedHalsteadStuff(hs, static_cast<const CXXMethodDecl*>(fd));
				handleFunctionRelatedHalsteadStuff(hs, static_cast<const FunctionDecl*>(fd));
			}
			else if (FunctionDecl::classof(fd))
				handleFunctionRelatedHalsteadStuff(hs, static_cast<const FunctionDecl*>(fd));
		}

		// Note: Local classes cannot contain template friends, thus there is no need to handle that.
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitDecl(const Decl* decl)
{
	// Add starting, actual and ending line numbers to set.
	// These are the only places where the decl is sure to contain code.
	SourceManager& sm = rMyAction.getCompilerInstance().getSourceManager();
	rMyAction.myCodeLines.insert(sm.getSpellingLineNumber(decl->getLocStart()));
	rMyAction.myCodeLines.insert(sm.getSpellingLineNumber(decl->getLocation()));
	rMyAction.myCodeLines.insert(sm.getSpellingLineNumber(decl->getLocEnd()));

	// Handle semicolons.
	SourceLocation semiloc = findSemiAfterLocation(decl->getLocEnd(), rMyAction.getCompilerInstance().getASTContext(), false);
	handleSemicolon(sm, static_cast<const FunctionDecl*>(decl->getParentFunctionOrMethod()), semiloc);

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitStmt(const Stmt* stmt)
{
	// Add starting and ending line numbers to set.
	// These are the only places where the statement is sure to contain code.
	SourceManager& sm = rMyAction.getCompilerInstance().getSourceManager();
	rMyAction.myCodeLines.insert(sm.getSpellingLineNumber(stmt->getLocStart()));
	rMyAction.myCodeLines.insert(sm.getSpellingLineNumber(stmt->getLocEnd()));

	// Handle semicolons.
	SourceLocation semiloc = findSemiAfterLocation(stmt->getLocEnd(), rMyAction.getCompilerInstance().getASTContext(), false);
	handleSemicolon(sm, getFunctionFromStmt(*stmt), semiloc);

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitDeclStmt(const clang::DeclStmt* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

		if (stmt->isSingleDecl())
		{
			// Handle the qualified type of single decl.
			if (ValueDecl::classof(stmt->getSingleDecl()))
				handleQualType(hs, static_cast<const ValueDecl*>(stmt->getSingleDecl())->getType(), true);

			return true;
		}

		const DeclGroup& group = stmt->getDeclGroup().getDeclGroup();
		if (group.size() == 0)
			return true;

		if (!ValueDecl::classof(group[0]))
			return true;

		// Handle the qualified type of the first decl.
		handleQualType(hs, static_cast<const ValueDecl*>(group[0])->getType(), true);

		// Only need to check for pointer and reference decl syntax.
		for (unsigned i = 1; i < group.size(); ++i)
		{
			QualType type = cast<const ValueDecl>(group[i])->getType();

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
		}

		// Declaration names are already declared in their respective function.
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitIfStmt(const clang::IfStmt* stmt)
{
	increaseMcCCStmt(stmt);

	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];
		hs.add<Halstead::IfOperator>();

		if (stmt->getElse())
			hs.add<Halstead::ElseOperator>();
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitForStmt(const clang::ForStmt* stmt)
{
	increaseMcCCStmt(stmt);

	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::ForOperator>();

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitCXXForRangeStmt(const clang::CXXForRangeStmt* stmt)
{
	increaseMcCCStmt(stmt);

	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::ForOperator>();

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitWhileStmt(const clang::WhileStmt* stmt)
{
	increaseMcCCStmt(stmt);

	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::WhileOperator>();

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitDoStmt(const clang::DoStmt* stmt)
{
	increaseMcCCStmt(stmt);

	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];
		hs.add<Halstead::DoOperator>();
		hs.add<Halstead::WhileOperator>();
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitSwitchStmt(const clang::SwitchStmt* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::SwitchOperator>();

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitCaseStmt(const clang::CaseStmt* stmt)
{
	increaseMcCCStmt(stmt);

	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::CaseOperator>();

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitDefaultStmt(const clang::DefaultStmt* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::DefaultCaseOperator>();

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitBreakStmt(const clang::BreakStmt* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::BreakOperator>();

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitContinueStmt(const clang::ContinueStmt* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::ContinueOperator>();

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitLabelStmt(const clang::LabelStmt* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::LabelDeclOperand>(stmt->getDecl());

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitGotoStmt(const clang::GotoStmt* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

		// Add goto keyword (operator).
		hs.add<Halstead::GotoOperator>();

		// Add its label (operand).
		hs.add<Halstead::LabelDeclOperand>(stmt->getLabel());
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitCXXTryStmt(const clang::CXXTryStmt* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::TryOperator>();

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitCXXCatchStmt(const clang::CXXCatchStmt* stmt)
{
	increaseMcCCStmt(stmt);

	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::CatchOperator>();

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitCXXThrowExpr(const clang::CXXThrowExpr* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::ThrowOperator>();

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitReturnStmt(const clang::ReturnStmt* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::ReturnOperator>();

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitUnaryExprOrTypeTraitExpr(const UnaryExprOrTypeTraitExpr* stmt)
{
	// Only increase Halstead operator count in case of 'alignof' and 'sizeof'
	if (stmt->getKind() == UnaryExprOrTypeTrait::UETT_AlignOf || stmt->getKind() == UnaryExprOrTypeTrait::UETT_SizeOf)
	{
		// Find parent function
		const Decl* parent = getDeclFromStmt(*stmt);
		if (const DeclContext* function = parent->getParentFunctionOrMethod())
		{
			// Increase operator count of said function
			const FunctionDecl* const f = static_cast<const FunctionDecl*>(function);
			if (stmt->getKind() == UnaryExprOrTypeTrait::UETT_AlignOf)
				rMyAction.myHalsteadByFunctions[f].add<Halstead::AlignofOperator>();
			else
				rMyAction.myHalsteadByFunctions[f].add<Halstead::SizeofOperator>();
		}
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitDeclRefExpr(const DeclRefExpr* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
	{
		// Functions are handled by VisitCallExpr(), because after declaration a function can also become
		// an operand if used as an argument to another function.
		// Here we only handle ValueDecls that are always operands.
		const ValueDecl* decl = stmt->getDecl();
		if (!FunctionDecl::classof(decl))
			rMyAction.myHalsteadByFunctions[f].add<Halstead::ValueDeclOperand>(decl);

		// Add Halstead operators/operands if this is a nested name.
		handleNestedName(f, stmt->getQualifier());
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitCallExpr(const clang::CallExpr* stmt)
{
	// UDLs are handled at the different "literal" callbacks, so we must skip those here.
	// TODO: Template UDLs are still not handled (or at least not tested).
	if (UserDefinedLiteral::classof(stmt))
		return true;

	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

		// If this is a call to an overloaded operator, we must hande it "syntactically" as if it was a built-in operator.
		// Clang only classifies "operator call expression" as such if the "traditional" infix syntax is used.
		// For example for 'a == b', the '==' is an CXXOperatorCallExpr whereas in the case of 'a.operator==(b)' it is a method call.
		if (CXXOperatorCallExpr::classof(stmt))
		{	
			UnifiedCXXOperator op = convertOverloadedOperator(static_cast<const CXXOperatorCallExpr*>(stmt));
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
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitCXXConstructExpr(const clang::CXXConstructExpr* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

		// Iterate over the arguments.
		for (const Expr* arg : stmt->arguments())
		{
			// As a top AST node, 'arg' cannot be a DeclRefExpr to a FunctionDecl, as there's always at least
			// an implicit conversion (function to pointer decay) before. This means we can call the lambda as it is
			// without checking the type of 'arg'.
			handleCallArgs(hs, arg);
		}
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitMemberExpr(const clang::MemberExpr* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

		// Functions are handled by VisitCallExpr(), because after declaration a function can also become
		// an operand if used as an argument to another function.
		// Here we only handle ValueDecls that are always operands.
		const ValueDecl* decl = stmt->getMemberDecl();
		if (!FunctionDecl::classof(decl))
			hs.add<Halstead::ValueDeclOperand>(decl);

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

bool ClangMetricsAction::NodeVisitor::VisitCXXThisExpr(const clang::CXXThisExpr* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
	{
		// Add this keyword.
		rMyAction.myHalsteadByFunctions[f].add<Halstead::ThisExprOperator>();
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitCXXNewExpr(const clang::CXXNewExpr* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];

		// Add new keyword (operator).
		hs.add<Halstead::NewExprOperator>();

		// Add the type allocated by the new keyword.
		hs.add<Halstead::TypeOperator>(stmt->getAllocatedType().getTypePtr());

		// Note: placement params are handled automatically by the visitor.
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitCXXDeleteExpr(const clang::CXXDeleteExpr* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::DeleteExprOperator>();

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitExplicitCastExpr(const clang::ExplicitCastExpr* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
	{
		HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[f];
		
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

bool ClangMetricsAction::NodeVisitor::VisitIntegerLiteral(const IntegerLiteral* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
	{
		if (auto udl = searchForParent<UserDefinedLiteral>(stmt))
			rMyAction.myHalsteadByFunctions[f].add<Halstead::UserDefinedLiteralOperand>(udl, stmt);
		else
			rMyAction.myHalsteadByFunctions[f].add<Halstead::IntegerLiteralOperand>(stmt);
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitFloatingLiteral(const FloatingLiteral* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
	{
		if (auto udl = searchForParent<UserDefinedLiteral>(stmt))
			rMyAction.myHalsteadByFunctions[f].add<Halstead::UserDefinedLiteralOperand>(udl, stmt);
		else
			rMyAction.myHalsteadByFunctions[f].add<Halstead::FloatingLiteralOperand>(stmt);
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitCharacterLiteral(const CharacterLiteral* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
	{
		if (auto udl = searchForParent<UserDefinedLiteral>(stmt))
			rMyAction.myHalsteadByFunctions[f].add<Halstead::UserDefinedLiteralOperand>(udl, stmt);
		else
			rMyAction.myHalsteadByFunctions[f].add<Halstead::CharacterLiteralOperand>(stmt);
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitStringLiteral(const StringLiteral* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
	{
		if (auto udl = searchForParent<UserDefinedLiteral>(stmt))
			rMyAction.myHalsteadByFunctions[f].add<Halstead::UserDefinedLiteralOperand>(udl, stmt);
		else
			rMyAction.myHalsteadByFunctions[f].add<Halstead::StringLiteralOperand>(stmt);
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitCXXBoolLiteralExpr(const CXXBoolLiteralExpr* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::BoolLiteralOperand>(stmt);

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitCXXNullPtrLiteralExpr(const CXXNullPtrLiteralExpr* stmt)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*stmt))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::NullptrLiteralOperand>();

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitBinaryOperator(const BinaryOperator* op)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*op))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::OperatorOperator>(op->getOpcode());

	// Increase McCC only if this is a logical and/or operator
	if (op->getOpcode() == BinaryOperatorKind::BO_LAnd || op->getOpcode() == BinaryOperatorKind::BO_LOr)
	{
		increaseMcCCStmt(op);
	}

	return true;
}

bool ClangMetricsAction::NodeVisitor::VisitUnaryOperator(const clang::UnaryOperator* op)
{
	if (const FunctionDecl* f = getFunctionFromStmt(*op))
		rMyAction.myHalsteadByFunctions[f].add<Halstead::OperatorOperator>(op->getOpcode());

	return true;
}





// ----------------------------------------------- HELPER FUNCTIONS -----------------------------------------------

void ClangMetricsAction::NodeVisitor::increaseMcCCStmt(const Stmt* stmt)
{
	// Increase per file McCC
	++rMyAction.myFileMcCC;

	const Decl* parent = getDeclFromStmt(*stmt);

	// Check whether we've got a parent and if it's a function or not
	if (!parent || !FunctionDecl::classofKind(parent->getKind()))
		return;

	// Increase the McCC by one
	++rMyAction.myMcCCByFunctions[static_cast<const FunctionDecl*>(parent)];
}

const Decl* ClangMetricsAction::NodeVisitor::getDeclFromStmt(const Stmt& stmt)
{
	ASTContext& con = rMyAction.getCompilerInstance().getASTContext();
	auto parents = con.getParents(stmt);

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

const clang::FunctionDecl* ClangMetricsAction::NodeVisitor::getFunctionFromStmt(const clang::Stmt& stmt)
{
	// Find function by trying to find the statement's decl - works for most cases.
	if (const Decl* d = getDeclFromStmt(stmt))
	{
		if (FunctionDecl::classofKind(d->getKind()))
			return static_cast<const FunctionDecl*>(d);

		if (const FunctionDecl* f = static_cast<const FunctionDecl*>(d->getParentFunctionOrMethod()))
			return f;
	}

	// If the above fails, then brute-force search by source location comparision.
	// Needed for 'decltype' and possibly some other cases.

	// TODO: If performance requires it, store the locations in an ordered set for faster access. Modify VisitFunctionDecl()
	//       to save the locations of each function. This code should rarely ever run though, so only optimize if needed.

	// Reference to the SourceManager for getting line and column info of SourceLocations.
	const SourceManager& sm = rMyAction.getCompilerInstance().getSourceManager();

	// Helper lambda. Returns whether rhs is after lhs.
	auto isAfter = [&sm](SourceLocation lhs, SourceLocation rhs)
	{
		const unsigned ll = sm.getSpellingLineNumber(lhs);
		const unsigned rl = sm.getSpellingLineNumber(rhs);

		if (rl < ll)
			return false;

		if (ll < rl)
			return true;

		return sm.getSpellingColumnNumber(lhs) < sm.getSpellingColumnNumber(rhs);
	};

	// Helper lambda. Returns the distance (number of characters) from lhs to rhs.
	// Works only if rhs is after lhs.
	auto distance = [&sm](SourceLocation lhs, SourceLocation rhs)
	{
		const unsigned lines   = sm.getSpellingLineNumber(rhs) - sm.getSpellingLineNumber(lhs);
		const unsigned columns = sm.getSpellingColumnNumber(rhs) - sm.getSpellingColumnNumber(lhs);

		return lines + columns;
	};

	// Variables for 'minvalue' calculation.
	unsigned closestDistance = std::numeric_limits<unsigned>::max();
	const FunctionDecl* closestFunction = nullptr;

	// Iterate over all functions and try to find the closest to the statement that contains the statement.
	const SourceRange stmtRange = stmt.getSourceRange();
	for (const FunctionDecl* f : rMyAction.myFunctions)
	{
		const SourceRange range = f->getSourceRange();
		
		// Is the statement within the function?
		if (isAfter(range.getBegin(), stmtRange.getBegin()) && isAfter(stmtRange.getEnd(), range.getEnd()))
		{
			// Check if the distance between the start of the function and the statement is less than the closest distance.
			unsigned dist = distance(range.getBegin(), stmtRange.getBegin());
			if (dist < closestDistance)
			{
				// Update values.
				closestDistance = dist;
				closestFunction = f;
			}
		}
	}

	// If no function was found, nullptr will be returned.
	return closestFunction;
}

void ClangMetricsAction::NodeVisitor::handleNestedName(const clang::FunctionDecl* func, const clang::NestedNameSpecifier* nns)
{
	HalsteadStorage& hs = rMyAction.myHalsteadByFunctions[func];

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

void ClangMetricsAction::NodeVisitor::handleQualType(HalsteadStorage& hs, const clang::QualType& qtype, bool isOperator)
{
	// Desugar the type of any "internal" qualifiers (eg.: reference to const) where the qualifier
	// is hidden as part of the type itself.
	if (const Type* type = qtype.getTypePtr())
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
				handleQualType(hs, static_cast<const PointerType*>(type)->getPointeeType(), isOperator);
			}
			else if (type->isReferenceType())
			{
				if (type->isLValueReferenceType())
					hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::LV_REF);
				else if (type->isRValueReferenceType())
					hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::RV_REF);

				handleQualType(hs, static_cast<const ReferenceType*>(type)->getPointeeTypeAsWritten(), isOperator);
			}
			else if (type->isMemberPointerType())
			{
				hs.add<Halstead::ScopeResolutionOperator>();
				hs.add<Halstead::QualifierOperator>(Halstead::QualifierOperator::POINTER);

				handleQualType(hs, static_cast<const MemberPointerType*>(type)->getPointeeType(), isOperator);
			}
			else if (AutoType::classof(type))
			{
				AutoTypeKeyword word = static_cast<const AutoType*>(type)->getKeyword();
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

void ClangMetricsAction::NodeVisitor::handleCallArgs(HalsteadStorage& hs, const Stmt* arg)
{
	// Iterate over the children of the statement in the AST.
	for (const Stmt* sub : arg->children())
	{
		// We're only interested in DeclRefExprs.
		if (DeclRefExpr::classof(sub))
		{
			const ValueDecl* argDecl = static_cast<const DeclRefExpr*>(sub)->getDecl();

			// Every ValueDecl is already added by VisitDeclRefExpr() except FunctionDecls, because in this case
			// they are considered operands - we add them here manually.
			if (FunctionDecl::classof(argDecl))
				hs.add<Halstead::ValueDeclOperand>(argDecl);
		}

		// Recursion into the children of the child.
		if (sub->child_begin() != sub->child_end())
			handleCallArgs(hs, sub);
	}
}

UnifiedCXXOperator ClangMetricsAction::NodeVisitor::convertOverloadedOperator(const CXXOperatorCallExpr* stmt)
{
	// Get the operator's kind
	OverloadedOperatorKind op = stmt->getOperator();

	// Add as if it was a built-in operator.
	UnifiedCXXOperator ot = UnifiedCXXOperator::UNKNOWN;
	switch (op)
	{
		// "Primitive" cases
	case clang::OO_Tilde: ot = UnifiedCXXOperator::BITWISE_NEGATION; break;
	case clang::OO_Exclaim: ot = UnifiedCXXOperator::LOGICAL_NEGATION; break;
	case clang::OO_Pipe: ot = UnifiedCXXOperator::BITWISE_OR; break;
	case clang::OO_Equal: ot = UnifiedCXXOperator::ASSIGNMENT; break;
	case clang::OO_Less: ot = UnifiedCXXOperator::LESS; break;
	case clang::OO_Greater: ot = UnifiedCXXOperator::GREATER; break;
	case clang::OO_PlusEqual: ot = UnifiedCXXOperator::COMPOUND_ADDITION; break;
	case clang::OO_MinusEqual: ot = UnifiedCXXOperator::COMPOUND_SUBTRACTION; break;
	case clang::OO_StarEqual: ot = UnifiedCXXOperator::COMPOUND_MULTIPLICATION; break;
	case clang::OO_SlashEqual: ot = UnifiedCXXOperator::COMPOUND_DIVISION; break;
	case clang::OO_PercentEqual: ot = UnifiedCXXOperator::COMPOUND_MODULO; break;
	case clang::OO_CaretEqual: ot = UnifiedCXXOperator::COMPOUND_BITWISE_XOR; break;
	case clang::OO_AmpEqual: ot = UnifiedCXXOperator::COMPOUND_BITWISE_AND; break;
	case clang::OO_PipeEqual: ot = UnifiedCXXOperator::COMPOUND_BITWISE_OR; break;
	case clang::OO_LessLess: ot = UnifiedCXXOperator::LEFT_SHIFT; break;
	case clang::OO_GreaterGreater: ot = UnifiedCXXOperator::RIGHT_SHIFT; break;
	case clang::OO_LessLessEqual: ot = UnifiedCXXOperator::COMPOUND_LEFT_SHIFT; break;
	case clang::OO_GreaterGreaterEqual: ot = UnifiedCXXOperator::COMPOUND_RIGHT_SHIFT; break;
	case clang::OO_EqualEqual: ot = UnifiedCXXOperator::EQUAL; break;
	case clang::OO_ExclaimEqual: ot = UnifiedCXXOperator::NOT_EQUAL; break;
	case clang::OO_LessEqual: ot = UnifiedCXXOperator::LESS_EQUAL; break;
	case clang::OO_GreaterEqual: ot = UnifiedCXXOperator::GREATER_EQUAL; break;
	case clang::OO_AmpAmp: ot = UnifiedCXXOperator::LOGICAL_AND; break;
	case clang::OO_PipePipe: ot = UnifiedCXXOperator::LOGICAL_OR; break;
	case clang::OO_Slash: ot = UnifiedCXXOperator::DIVISION; break;
	case clang::OO_Percent: ot = UnifiedCXXOperator::MODULO; break;
	case clang::OO_Caret: ot = UnifiedCXXOperator::BITWISE_XOR; break;
	case clang::OO_Comma: ot = UnifiedCXXOperator::COMMA; break;
	case clang::OO_ArrowStar: ot = UnifiedCXXOperator::POINTER_TO_MEMBER_ARROW; break;
	case clang::OO_Arrow: ot = UnifiedCXXOperator::ARROW; break;
	case clang::OO_Subscript: ot = UnifiedCXXOperator::SUBSCRIPT; break;

		// Special cases where additional handling is needed.
	case clang::OO_Plus:
		if (stmt->getNumArgs() == 2)
			ot = UnifiedCXXOperator::ADDITION;
		else
			ot = UnifiedCXXOperator::UNARY_PLUS;
		break;

	case clang::OO_Minus:
		if (stmt->getNumArgs() == 2)
			ot = UnifiedCXXOperator::SUBTRACTION;
		else
			ot = UnifiedCXXOperator::UNARY_MINUS;
		break;

	case clang::OO_Star:
		if (stmt->getNumArgs() == 2)
			ot = UnifiedCXXOperator::MULTIPLICATION;
		else
			ot = UnifiedCXXOperator::DEREFERENCE;
		break;

	case clang::OO_Amp:
		if (stmt->getNumArgs() == 2)
			ot = UnifiedCXXOperator::BITWISE_AND;
		else
			ot = UnifiedCXXOperator::ADDRESS_OF;
		break;

	case clang::OO_PlusPlus:
		if (stmt->getDirectCallee()->getNumParams() == 1)
			ot = UnifiedCXXOperator::POSTFIX_INCREMENT;  // postfix
		else
			ot = UnifiedCXXOperator::PREFIX_INCREMENT;   // prefix
		break;

	case clang::OO_MinusMinus:
		if (stmt->getDirectCallee()->getNumParams() == 1)
			ot = UnifiedCXXOperator::POSTFIX_DECREMENT;  // postfix
		else
			ot = UnifiedCXXOperator::PREFIX_DECREMENT;   // prefix
		break;
	}

	return ot;
}

void ClangMetricsAction::NodeVisitor::handleSemicolon(SourceManager& sm, const FunctionDecl* f, SourceLocation semiloc)
{
	if (!f || semiloc.isInvalid())
		return;
	
	unsigned line = sm.getSpellingLineNumber(semiloc);
	unsigned column = sm.getSpellingColumnNumber(semiloc);

	// Ensure that the semicolon we found is within the range of the function.
	unsigned sl = sm.getSpellingLineNumber(f->getLocStart());
	unsigned sc = sm.getSpellingColumnNumber(f->getLocStart());
	unsigned el = sm.getSpellingLineNumber(f->getLocEnd());
	unsigned ec = sm.getSpellingColumnNumber(f->getLocEnd());
	if (!(sl <= line && line <= el))
		return;

	if (sl == line && !(sc < column))
		return;

	if (el == line && !(column < ec))
		return;

	// If this is the first time we see this semicolon, add it as an operator
	// and register it, so that it won't be counted multiple times.
	auto it = rMyAction.mySemicolonLocations.find({ line, column });
	if (it == rMyAction.mySemicolonLocations.end())
	{
		rMyAction.myHalsteadByFunctions[f].add<Halstead::SemicolonOperator>();
		rMyAction.mySemicolonLocations.emplace(line, column);
	}
}

void ClangMetricsAction::NodeVisitor::handleFunctionRelatedHalsteadStuff(HalsteadStorage& hs, const clang::FunctionDecl* decl)
{
	// A function always have a return type which can be considered as an operator (Halstead), unless it's
	// a constructor, a destructor or a cast operator (in which case the typename is considered part of the operator keyword).
	if (!CXXConstructorDecl::classof(decl) && !CXXDestructorDecl::classof(decl) && !CXXConversionDecl::classof(decl))
		handleQualType(hs, decl->getReturnType(), true);

	// A function always have a name, which can be considered as an operand (Halstead).
	hs.add<Halstead::ValueDeclOperand>(decl);

	// Check for alternative function declaration (trailing return).
	if (decl->getType()->isFunctionProtoType())
	{
		if (static_cast<const FunctionProtoType*>(decl->getType().getTypePtr())->hasTrailingReturn())
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
}

void ClangMetricsAction::NodeVisitor::handleMethodRelatedHalsteadStuff(HalsteadStorage& hs, const clang::CXXMethodDecl* decl)
{
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
	if (CXXConstructorDecl::classof(decl) && static_cast<const CXXConstructorDecl*>(decl)->isExplicitSpecified())
		hs.add<Halstead::ExplicitOperator>();

	if (CXXConversionDecl::classof(decl) && static_cast<const CXXConversionDecl*>(decl)->isExplicitSpecified())
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
												ASTContext &Ctx,
												bool IsDecl) {
	SourceManager &SM = Ctx.getSourceManager();
	if (loc.isMacroID()) {
		if (!Lexer::isAtEndOfMacroExpansion(loc, SM, Ctx.getLangOpts(), &loc))
		return SourceLocation();
	}
	loc = Lexer::getLocForEndOfToken(loc, /*Offset=*/0, SM, Ctx.getLangOpts());
 
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
				Ctx.getLangOpts(),
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