
// Generalized operand definitions
HALSTEAD_AUTODERIVE(NullptrLiteralOperand);
HALSTEAD_AUTODERIVE(MessageClassReceiverOperand);


// More specific operator definitions
HALSTEAD_DERIVE(ValueDeclOperand, NamedDecl)
{
  using Derive::Derive;

  std::string getDebugName() const override
  {
    std::ostringstream ss;
    ss << Derive::getDebugName() << " (" << pMyData->getNameAsString() << " @ " << pMyData << ")";
    return ss.str();
  }

  // Two operands are considered to be the same if and only if their declaration matches.
  bool isSameAs(const OpBase& other) const override
  {
    return OpBase::isSameAs(other) &&
      pMyData == static_cast<const ValueDeclOperand&>(other).pMyData;
  }
};

HALSTEAD_DERIVE(MessageSelectorOperand, ObjCMessageExpr)
{
    using Derive::Derive;
    
    std::string getDebugName() const override
    {
        std::ostringstream ss;
        ss << Derive::getDebugName() << " (" << pMyData->getSelector().getAsString() << ")";
        return ss.str();
    }
    
    // Two operands are considered to be the same if and only if their declaration matches.
    bool isSameAs(const OpBase& other) const override
    {
        return OpBase::isSameAs(other) &&
        pMyData == static_cast<const MessageSelectorOperand&>(other).pMyData;
    }
};

HALSTEAD_DERIVE(LabelDeclOperand, LabelDecl)
{
  using Derive::Derive;

  std::string getDebugName() const override
  {
    std::ostringstream ss;
    ss << Derive::getDebugName() << " (" << pMyData->getNameAsString() << " @ " << pMyData << ")";
    return ss.str();
  }

  // Two operands are considered to be the same if and only if their declaration matches.
  bool isSameAs(const OpBase& other) const override
  {
    return OpBase::isSameAs(other) &&
      pMyData == static_cast<const LabelDeclOperand&>(other).pMyData;
  }
};

HALSTEAD_DERIVE(UsingOperand, UsingDecl)
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
      pMyData == static_cast<const UsingOperand&>(other).pMyData;
  }
};

HALSTEAD_DERIVE(NamespaceOperand, NamedDecl)
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
      pMyData == static_cast<const NamespaceOperand&>(other).pMyData;
  }
};

HALSTEAD_DERIVE(TypeOperand, Type)
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

  // Two operands are considered to be the same if and only if their type matches.
  // Note: Two operands can be different even if they are of the same C++ type (eg.: typedefs).
  bool isSameAs(const OpBase& other) const override
  {
    return OpBase::isSameAs(other) &&
      pMyData == static_cast<const TypeOperand&>(other).pMyData;
  }
};

HALSTEAD_DERIVE(BoolLiteralOperand, CXXBoolLiteralExpr)
{
  using Derive::Derive;

  std::string getDebugName() const override
  {
    return Derive::getDebugName() + (pMyData->getValue() ? " (true)" : " (false)");
  }

  // Two operators are considered to be the same if and only if their value matches.
  bool isSameAs(const OpBase& other) const override
  {
    return OpBase::isSameAs(other) &&
      pMyData->getValue() == static_cast<const BoolLiteralOperand&>(other).pMyData->getValue();
  }
};

HALSTEAD_DERIVE(ObjCBoolLiteralOperand, ObjCBoolLiteralExpr)
{
  using Derive::Derive;

  std::string getDebugName() const override
  {
    return Derive::getDebugName() + (pMyData->getValue() ? " (true)" : " (false)");
  }

  // Two operators are considered to be the same if and only if their value matches.
  bool isSameAs(const OpBase& other) const override
  {
    return OpBase::isSameAs(other) &&
      pMyData->getValue() == static_cast<const ObjCBoolLiteralOperand&>(other).pMyData->getValue();
  }
};

HALSTEAD_DERIVE(IntegerLiteralOperand, IntegerLiteral)
{
  using Derive::Derive;

  std::string getDebugName() const override
  {
	SmallString<40> ss;
    pMyData->getValue().toString(ss, 10, false);
    return Derive::getDebugName() + " (" + ss.str().str() + ")";
  }

  // Two operands are considered to be the same if and only if their value matches.
  bool isSameAs(const OpBase& other) const override
  {
    if (!OpBase::isSameAs(other))
      return false;

    llvm::APInt v1 = pMyData->getValue();
    llvm::APInt v2 = static_cast<const IntegerLiteralOperand&>(other).pMyData->getValue();

    return v1.getBitWidth() == v2.getBitWidth() && v1 == v2;
  }
};

HALSTEAD_DERIVE(FloatingLiteralOperand, FloatingLiteral)
{
  using Derive::Derive;

  std::string getDebugName() const override
  {
    // TODO: Rounds the number to the nearest double. Would it be possible to print it as it is?
    return Derive::getDebugName() + " (" + std::to_string(pMyData->getValueAsApproximateDouble()) + ")";
  }

  // Two operands are considered to be the same if and only if their value matches.
  bool isSameAs(const OpBase& other) const override
  {
    // TODO: Would it be somehow possible to use the value written instead of the machine representation (with its precision limits)?
    return OpBase::isSameAs(other) &&
      pMyData->getValue().bitwiseIsEqual(static_cast<const FloatingLiteralOperand&>(other).pMyData->getValue());
  }
};

HALSTEAD_DERIVE(CharacterLiteralOperand, CharacterLiteral)
{
  using Derive::Derive;

  std::string getDebugName() const override
  {
    return Derive::getDebugName() + " (" + static_cast<char>(pMyData->getValue()) + ")";
  }

  // Two operands are considered to be the same if and only if their value matches.
  bool isSameAs(const OpBase& other) const override
  {
    return OpBase::isSameAs(other) &&
      pMyData->getValue() == static_cast<const CharacterLiteralOperand&>(other).pMyData->getValue();
  }
};

HALSTEAD_DERIVE(StringLiteralOperand, StringLiteral)
{
  using Derive::Derive;

  std::string getDebugName() const override
  {
    std::string ret = Derive::getDebugName() + " (";

    const char* data = pMyData->getBytes().data();
    unsigned size = pMyData->getByteLength();
    ret.reserve(ret.size() + size + 1);
    for (unsigned i = 0; i < size; ++i)
      ret.push_back(data[i]);
    ret.push_back(')');

    return ret;
  }

  // Two operands are considered to be the same if and only if their value matches.
  bool isSameAs(const OpBase& other) const override
  {
    return OpBase::isSameAs(other) &&
      pMyData->getBytes() == static_cast<const StringLiteralOperand&>(other).pMyData->getBytes();
  }
};

HALSTEAD_MANUAL_DERIVE(UserDefinedLiteralOperand)
{
public:
  UserDefinedLiteralOperand(const clang::UserDefinedLiteral* udl, const clang::CharacterLiteral* lit) : pMyUDL(udl), pMyCharacterLit(lit)
  {
    myLiteralType = CHARACTER;
  }

  UserDefinedLiteralOperand(const clang::UserDefinedLiteral* udl, const clang::StringLiteral* lit) : pMyUDL(udl), pMyStringLit(lit)
  {
    myLiteralType = STRING;
  }

  UserDefinedLiteralOperand(const clang::UserDefinedLiteral* udl, const clang::IntegerLiteral* lit) : pMyUDL(udl), pMyIntegerLit(lit)
  {
    myLiteralType = INTEGER;
  }

  UserDefinedLiteralOperand(const clang::UserDefinedLiteral* udl, const clang::FloatingLiteral* lit) : pMyUDL(udl), pMyFloatingLit(lit)
  {
    myLiteralType = FLOATING;
  }

private:
  std::string getDebugName() const override
  {
    std::string ret;
    switch (myLiteralType)
    {
    case Halstead::UserDefinedLiteralOperand::CHARACTER:
      ret = (char)pMyCharacterLit->getValue();
      break;

    case Halstead::UserDefinedLiteralOperand::STRING:
      ret = std::string(pMyStringLit->getBytes().data(), pMyStringLit->getBytes().data() + pMyStringLit->getByteLength());
      break;

    case Halstead::UserDefinedLiteralOperand::INTEGER:
    {
      SmallString<40> ss;
      pMyIntegerLit->getValue().toString(ss, 10, false);
      ret = ss.str().str();
      break;
    }

    case Halstead::UserDefinedLiteralOperand::FLOATING:
      ret = std::to_string(pMyFloatingLit->getValueAsApproximateDouble());
      break;
    }

    return "UserDefinedLiteralOperand (" + ret + ')';
  }

  // To be considered the same the UDL operators must be the same along with the literal value.
  bool isSameAs(const OpBase& other) const override
  {
    if (!OpBase::isSameAs(other))
      return false;

    const UserDefinedLiteralOperand& o = static_cast<const UserDefinedLiteralOperand&>(other);

    bool callee = pMyUDL->getDirectCallee() == o.pMyUDL->getDirectCallee();

    bool value = myLiteralType == o.myLiteralType;
    if (value)
    {
      switch (myLiteralType)
      {
      case UserDefinedLiteralOperand::CHARACTER:
        value = pMyCharacterLit->getValue() == o.pMyCharacterLit->getValue();
        break;

      case UserDefinedLiteralOperand::STRING:
        value = !pMyStringLit->getBytes().compare(o.pMyStringLit->getBytes());
        break;

      case UserDefinedLiteralOperand::INTEGER:
        value = pMyIntegerLit->getValue() == o.pMyIntegerLit->getValue();
        break;

      case UserDefinedLiteralOperand::FLOATING:
        value = pMyFloatingLit->getValue().bitwiseIsEqual(pMyFloatingLit->getValue());
        break;
      }
    }

    return callee && value;    
  }

private:
  enum
  {
    CHARACTER,
    STRING,
    INTEGER,
    FLOATING

  } myLiteralType;

  const clang::UserDefinedLiteral* pMyUDL;

  union
  {
    const clang::CharacterLiteral* pMyCharacterLit;
    const clang::StringLiteral*    pMyStringLit;
    const clang::IntegerLiteral*   pMyIntegerLit;
    const clang::FloatingLiteral*  pMyFloatingLit;
  };
};
