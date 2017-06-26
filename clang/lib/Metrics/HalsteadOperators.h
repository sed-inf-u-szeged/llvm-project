
// Generalized operator definitions
HALSTEAD_AUTODERIVE(AlignofOperator);
HALSTEAD_AUTODERIVE(SizeofOperator);
HALSTEAD_AUTODERIVE(IfOperator);
HALSTEAD_AUTODERIVE(ElseOperator);
HALSTEAD_AUTODERIVE(ForOperator);
HALSTEAD_AUTODERIVE(WhileOperator);
HALSTEAD_AUTODERIVE(DoOperator);
HALSTEAD_AUTODERIVE(SwitchOperator);
HALSTEAD_AUTODERIVE(CaseOperator);
HALSTEAD_AUTODERIVE(DefaultCaseOperator);
HALSTEAD_AUTODERIVE(BreakOperator);
HALSTEAD_AUTODERIVE(ContinueOperator);
HALSTEAD_AUTODERIVE(TryOperator);
HALSTEAD_AUTODERIVE(CatchOperator);
HALSTEAD_AUTODERIVE(ThrowOperator);
HALSTEAD_AUTODERIVE(ReturnOperator);
HALSTEAD_AUTODERIVE(TrailingReturnArrowOperator);
HALSTEAD_AUTODERIVE(AutoOperator);
HALSTEAD_AUTODERIVE(DecltypeOperator);
HALSTEAD_AUTODERIVE(ClassOperator);
HALSTEAD_AUTODERIVE(StructOperator);
HALSTEAD_AUTODERIVE(UnionOperator);
HALSTEAD_AUTODERIVE(UsingOperator);
HALSTEAD_AUTODERIVE(NamespaceOperator);
HALSTEAD_AUTODERIVE(TypedefOperator);
HALSTEAD_AUTODERIVE(ScopeResolutionOperator);
HALSTEAD_AUTODERIVE(NewExprOperator);
HALSTEAD_AUTODERIVE(DeleteExprOperator);
HALSTEAD_AUTODERIVE(StaticCastOperator);
HALSTEAD_AUTODERIVE(ConstCastOperator);
HALSTEAD_AUTODERIVE(ReinterpretCastOperator);
HALSTEAD_AUTODERIVE(DynamicCastOperator);
HALSTEAD_AUTODERIVE(CStyleCastOperator);
HALSTEAD_AUTODERIVE(FunctionalCastOperator);
HALSTEAD_AUTODERIVE(ThisExprOperator);
HALSTEAD_AUTODERIVE(TemplateOperator);
HALSTEAD_AUTODERIVE(TypenameOperator);
HALSTEAD_AUTODERIVE(ParenthesesInitSyntaxOperator);
HALSTEAD_AUTODERIVE(BracesInitSyntaxOperator);
HALSTEAD_AUTODERIVE(SemicolonOperator);
HALSTEAD_AUTODERIVE(VariadicEllipsisOperator);
HALSTEAD_AUTODERIVE(PackExpansionOperator);
HALSTEAD_AUTODERIVE(PackDeclarationOperator);
HALSTEAD_AUTODERIVE(PackSizeofOperator);
HALSTEAD_AUTODERIVE(DefaultFunctionOperator);
HALSTEAD_AUTODERIVE(DeleteFunctionOperator);
HALSTEAD_AUTODERIVE(EnumOperator);
HALSTEAD_AUTODERIVE(ExplicitOperator);
HALSTEAD_AUTODERIVE(GotoOperator);
HALSTEAD_AUTODERIVE(FriendOperator);
HALSTEAD_AUTODERIVE(InlineOperator);
HALSTEAD_AUTODERIVE(MutableOperator);
HALSTEAD_AUTODERIVE(StaticOperator);
HALSTEAD_AUTODERIVE(VirtualOperator);
HALSTEAD_AUTODERIVE(PureVirtualDeclarationOperator);


// More specific operator definitions
HALSTEAD_DERIVE(AccessSpecDeclOperator, AccessSpecDecl)
{
	using Derive::Derive;

	std::string getDebugName() const override
	{
		const char* name;
		switch (pMyData->getAccess())
		{
		case clang::AccessSpecifier::AS_public:
			name = " (public)";
			break;
		case clang::AccessSpecifier::AS_protected:
			name = " (protected)";
			break;
		case clang::AccessSpecifier::AS_private:
			name = " (public)";
			break;
		default:
			name = " (<unknown>)";
			break;
		}

		return Derive::getDebugName() + name;
	}

	// Two access specifiers are the same if they are the same :).
	bool isSameAs(const OpBase& other) const override
	{
		return OpBase::isSameAs(other) &&
			pMyData->getAccess() == static_cast<const AccessSpecDeclOperator&>(other).pMyData->getAccess();
	}
};

HALSTEAD_MANUAL_DERIVE(OperatorOperator)
{
public:
	OperatorOperator(UnifiedCXXOperator kind) : myKind(kind)
	{}

	std::string getDebugName() const override
	{
		std::string type;
		if (myKind.isBinaryOperator())
			type = "binary ";
		else if (myKind.isUnaryOperator())
			type = "unary ";

		return "OperatorOperator (" + type + myKind.toString() + ')';
	}

	// Two operators are the same if they are of the same kind.
	bool isSameAs(const OpBase& other) const override
	{
		return OpBase::isSameAs(other) &&
			myKind == static_cast<const OperatorOperator&>(other).myKind;
	}

private:
	UnifiedCXXOperator myKind;
};

HALSTEAD_DERIVE(TypeOperator, Type)
{
	using Derive::Derive;

	std::string getDebugName() const override
	{
		std::string name;
		if (const clang::TypedefType* type = pMyData->getAs<clang::TypedefType>())
			name = type->getDecl()->getNameAsString();
		else
			name = pMyData->getCanonicalTypeInternal().getAsString();
		return Derive::getDebugName() + " (" + name + ")";
	}

	// Two operators are considered to be the same if and only if they are of the same type.
	bool isSameAs(const OpBase& other) const override
	{
		return OpBase::isSameAs(other) &&
			pMyData == static_cast<const TypeOperator&>(other).pMyData;
	}
};

HALSTEAD_DERIVE(TemplateNameOperator, TemplateDecl)
{
	using Derive::Derive;

	std::string getDebugName() const override
	{
		return Derive::getDebugName() + " (" + pMyData->getNameAsString() + ")";
	}

	// Two operators are considered to be the same if and only if they point to the same template.
	bool isSameAs(const OpBase& other) const override
	{
		return OpBase::isSameAs(other) &&
			pMyData == static_cast<const TemplateNameOperator&>(other).pMyData;
	}
};

HALSTEAD_DERIVE(NamespaceNameOperator, NamedDecl)
{
	using Derive::Derive;

	std::string getDebugName() const override
	{
		return Derive::getDebugName() + " (" + pMyData->getNameAsString() + ")";
	}

	// Two operands are considered to be the same if and only if their declaration matches.
	bool isSameAs(const OpBase& other) const override
	{
		return OpBase::isSameAs(other) &&
			pMyData == static_cast<const NamespaceNameOperator&>(other).pMyData;
	}
};

HALSTEAD_DERIVE(FunctionOperator, FunctionDecl)
{
	using Derive::Derive;

	std::string getDebugName() const override
	{
		std::ostringstream ss;
		ss << Derive::getDebugName() << " (" << pMyData->getNameAsString() << " @ " << pMyData << ")";
		return ss.str();
	}

	// Two operators are considered to be the same if and only if they are the same decl.
	bool isSameAs(const OpBase& other) const override
	{
		return OpBase::isSameAs(other) &&
			pMyData == static_cast<const FunctionOperator&>(other).pMyData;
	}
};

HALSTEAD_MANUAL_DERIVE(QualifierOperator)
{
public:
	enum qual_type
	{
		CONST,
		VOLATILE,
		POINTER,
		LV_REF,
		RV_REF
	};

public:
	QualifierOperator(qual_type kind) : myKind(kind)
	{}

private:
	std::string getDebugName() const override
	{
		std::string name;
		switch (myKind)
		{
		case Halstead::QualifierOperator::CONST:    name = "const";    break;
		case Halstead::QualifierOperator::VOLATILE: name = "volatile"; break;
		case Halstead::QualifierOperator::POINTER:  name = "*";        break;
		case Halstead::QualifierOperator::LV_REF:   name = "&";        break;
		case Halstead::QualifierOperator::RV_REF:   name = "&&";       break;
		}

		return "QualifierOperator (" + std::move(name) + ")";
	}

	// Two operators are the same if they are of the same kind.
	bool isSameAs(const OpBase& other) const override
	{
		return OpBase::isSameAs(other) &&
			myKind == static_cast<const QualifierOperator&>(other).myKind;
	}

private:
	qual_type myKind;
};
