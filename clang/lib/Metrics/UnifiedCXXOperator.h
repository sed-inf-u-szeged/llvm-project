#pragma once

#include <clang/AST/AST.h>

#include <string>


namespace clang
{
	namespace metrics
	{
		namespace detail
		{
			/**
			 *	Contains all the available C++ operators in a single enum, except some special
			 *	operators which are considered seperate keywords.
			 *
			 *	C++ operators not included:
			 *		- sizeof, sizeof...
			 *		- alignof
			 *		- typeid
			 *		- conversion operators
			 *		- new and delete operators, along with the array version of these
			 *		- noexcept operator
			 *		- UDLs
			 *		- any other operator not part of the C++ 14 standard
			 */
			class UnifiedCXXOperator final
			{
			public:
				enum type
				{
					//! Unknown operator.
					UNKNOWN,

					//! Binary operator (overloadable). Syntax: a = b
					ASSIGNMENT,

					//! Binary operator (overloadable). Syntax: a + b
					ADDITION,

					//! Binary operator (overloadable). Syntax: a - b
					SUBTRACTION,

					//! Binary operator (overloadable). Syntax: a * b
					MULTIPLICATION,

					//! Binary operator (overloadable). Syntax: a / b
					DIVISION,

					//! Binary operator (overloadable). Syntax: a % b
					MODULO,

					//! Binary operator (overloadable). Syntax: a == b
					EQUAL,

					//! Binary operator (overloadable). Syntax: a != b
					NOT_EQUAL,

					//! Binary operator (overloadable). Syntax: a > b
					GREATER,

					//! Binary operator (overloadable). Syntax: a < b
					LESS,

					//! Binary operator (overloadable). Syntax: a >= b
					GREATER_EQUAL,

					//! Binary operator (overloadable). Syntax: a <= b
					LESS_EQUAL,

					//! Binary operator (overloadable). Syntax: a && b
					LOGICAL_AND,

					//! Binary operator (overloadable). Syntax: a || b
					LOGICAL_OR,

					//! Binary operator (overloadable). Syntax: a & b
					BITWISE_AND,

					//! Binary operator (overloadable). Syntax: a | b
					BITWISE_OR,

					//! Binary operator (overloadable). Syntax: a ^ b
					BITWISE_XOR,

					//! Binary operator (overloadable). Syntax: a << b
					LEFT_SHIFT,

					//! Binary operator (overloadable). Syntax: a >> b
					RIGHT_SHIFT,

					//! Binary operator (overloadable). Syntax: a += b
					COMPOUND_ADDITION,

					//! Binary operator (overloadable). Syntax: a -= b
					COMPOUND_SUBTRACTION,

					//! Binary operator (overloadable). Syntax: a *= b
					COMPOUND_MULTIPLICATION,

					//! Binary operator (overloadable). Syntax: a /= b
					COMPOUND_DIVISION,

					//! Binary operator (overloadable). Syntax: a %= b
					COMPOUND_MODULO,

					//! Binary operator (overloadable). Syntax: a &= b
					COMPOUND_BITWISE_AND,

					//! Binary operator (overloadable). Syntax: a |= b
					COMPOUND_BITWISE_OR,

					//! Binary operator (overloadable). Syntax: a ^= b
					COMPOUND_BITWISE_XOR,

					//! Binary operator (overloadable). Syntax: a <<= b
					COMPOUND_LEFT_SHIFT,

					//! Binary operator (overloadable). Syntax: a >>= b
					COMPOUND_RIGHT_SHIFT,

					//! Binary operator (overloadable). Syntax: a[b]
					SUBSCRIPT,

					//! Binary operator (overloadable). Syntax: a->b
					ARROW,

					//! Binary operator (overloadable). Syntax: a->*b
					POINTER_TO_MEMBER_ARROW,

					//! Binary operator (NOT overloadable). Syntax: a.b
					DOT,

					//! Binary operator (NOT overloadable). Syntax: a.*b
					POINTER_TO_MEMBER_DOT,

					//! Binary operator (overloadable). Syntax: a, b
					COMMA,

					//! Binary operator (NOT overloadable). Syntax: a::b
					SCOPE_RESOLUTION,




					//! Unary operator (overloadable). Syntax: *a
					DEREFERENCE,

					//! Unary operator (overloadable). Syntax: &a
					ADDRESS_OF,

					//! Unary operator (overloadable). Syntax: ~a
					BITWISE_NEGATION,

					//! Unary operator (overloadable). Syntax: !a
					LOGICAL_NEGATION,

					//! Unary operator (overloadable). Syntax: +a
					UNARY_PLUS,

					//! Unary operator (overloadable). Syntax: -a
					UNARY_MINUS,

					//! Unary operator (overloadable). Syntax: ++a
					PREFIX_INCREMENT,

					//! Unary operator (overloadable). Syntax: a++
					POSTFIX_INCREMENT,

					//! Unary operator (overloadable). Syntax: --a
					PREFIX_DECREMENT,

					//! Unary operator (overloadable). Syntax: a--
					POSTFIX_DECREMENT,




					//! Special operator (overloadable). Syntax: a(b, c, d, ...)
					FUNCTION_CALL,

					//! Special operator (NOT overloadable). Syntax: a ? b : c
					TERNARY,




					//! The size of this enumeration.
					_ENUM_COUNT
				};

			public:
				UnifiedCXXOperator(type op) : myType(op)
				{}

				UnifiedCXXOperator(clang::BinaryOperatorKind op)
				{
					switch (op)
					{
					case clang::BO_PtrMemD:   myType = POINTER_TO_MEMBER_DOT;   break;
					case clang::BO_PtrMemI:   myType = POINTER_TO_MEMBER_ARROW; break;
					case clang::BO_Mul:       myType = MULTIPLICATION;          break;
					case clang::BO_Div:       myType = DIVISION;                break;
					case clang::BO_Rem:       myType = MODULO;                  break;
					case clang::BO_Add:       myType = ADDITION;                break;
					case clang::BO_Sub:       myType = SUBTRACTION;             break;
					case clang::BO_Shl:       myType = LEFT_SHIFT;              break;
					case clang::BO_Shr:       myType = RIGHT_SHIFT;             break;
					case clang::BO_LT:        myType = LESS;                    break;
					case clang::BO_GT:        myType = GREATER;                 break;
					case clang::BO_LE:        myType = LESS_EQUAL;              break;
					case clang::BO_GE:        myType = GREATER_EQUAL;           break;
					case clang::BO_EQ:        myType = EQUAL;                   break;
					case clang::BO_NE:        myType = NOT_EQUAL;               break;
					case clang::BO_And:       myType = BITWISE_AND;             break;
					case clang::BO_Xor:       myType = BITWISE_XOR;             break;
					case clang::BO_Or:        myType = BITWISE_OR;              break;
					case clang::BO_LAnd:      myType = LOGICAL_AND;             break;
					case clang::BO_LOr:       myType = LOGICAL_OR;              break;
					case clang::BO_Assign:    myType = ASSIGNMENT;              break;
					case clang::BO_MulAssign: myType = COMPOUND_MULTIPLICATION; break;
					case clang::BO_DivAssign: myType = COMPOUND_DIVISION;       break;
					case clang::BO_RemAssign: myType = COMPOUND_MODULO;         break;
					case clang::BO_AddAssign: myType = COMPOUND_ADDITION;       break;
					case clang::BO_SubAssign: myType = COMPOUND_SUBTRACTION;    break;
					case clang::BO_ShlAssign: myType = COMPOUND_LEFT_SHIFT;     break;
					case clang::BO_ShrAssign: myType = COMPOUND_RIGHT_SHIFT;    break;
					case clang::BO_AndAssign: myType = COMPOUND_BITWISE_AND;    break;
					case clang::BO_XorAssign: myType = COMPOUND_BITWISE_XOR;    break;
					case clang::BO_OrAssign:  myType = COMPOUND_BITWISE_OR;     break;
					case clang::BO_Comma:     myType = COMMA;                   break;

					default: myType = UNKNOWN; break;
					}
				}

				UnifiedCXXOperator(clang::UnaryOperatorKind op)
				{
					switch (op)
					{
					case clang::UO_PostInc: myType = POSTFIX_INCREMENT; break;
					case clang::UO_PostDec: myType = POSTFIX_DECREMENT; break;
					case clang::UO_PreInc:  myType = PREFIX_INCREMENT;  break;
					case clang::UO_PreDec:  myType = PREFIX_DECREMENT;  break;
					case clang::UO_AddrOf:  myType = ADDRESS_OF;        break;
					case clang::UO_Deref:   myType = DEREFERENCE;       break;
					case clang::UO_Plus:    myType = UNARY_PLUS;        break;
					case clang::UO_Minus:   myType = UNARY_MINUS;       break;
					case clang::UO_Not:     myType = BITWISE_NEGATION;  break;
					case clang::UO_LNot:    myType = LOGICAL_NEGATION;  break;

					default: myType = UNKNOWN; break;
					}
				}

				bool operator==(UnifiedCXXOperator op) const { return myType == op.myType; }

				bool isBinaryOperator() const
				{
					return myType > UNKNOWN && myType < DEREFERENCE;
				}

				bool isUnaryOperator() const
				{
					return myType >= DEREFERENCE && myType <= POSTFIX_DECREMENT;
				}

				bool isUnknown() const
				{
					return myType == UNKNOWN;
				}

				type getType() const
				{
					return myType;
				}

				std::string toString() const
				{
					switch (myType)
					{
					case UnifiedCXXOperator::ASSIGNMENT:              return "=";
					case UnifiedCXXOperator::ADDITION:                return "+";
					case UnifiedCXXOperator::SUBTRACTION:             return "-";
					case UnifiedCXXOperator::MULTIPLICATION:          return "*";
					case UnifiedCXXOperator::DIVISION:                return "/";
					case UnifiedCXXOperator::MODULO:                  return "%";
					case UnifiedCXXOperator::EQUAL:                   return "==";
					case UnifiedCXXOperator::NOT_EQUAL:               return "!=";
					case UnifiedCXXOperator::GREATER:                 return ">";
					case UnifiedCXXOperator::LESS:                    return "<";
					case UnifiedCXXOperator::GREATER_EQUAL:           return ">=";
					case UnifiedCXXOperator::LESS_EQUAL:              return "<=";
					case UnifiedCXXOperator::LOGICAL_AND:             return "&&";
					case UnifiedCXXOperator::LOGICAL_OR:              return "||";
					case UnifiedCXXOperator::BITWISE_AND:             return "&";
					case UnifiedCXXOperator::BITWISE_OR:              return "|";
					case UnifiedCXXOperator::BITWISE_XOR:             return "^";
					case UnifiedCXXOperator::LEFT_SHIFT:              return "<<";
					case UnifiedCXXOperator::RIGHT_SHIFT:             return ">>";
					case UnifiedCXXOperator::COMPOUND_ADDITION:       return "+=";
					case UnifiedCXXOperator::COMPOUND_SUBTRACTION:    return "-=";
					case UnifiedCXXOperator::COMPOUND_MULTIPLICATION: return "*=";
					case UnifiedCXXOperator::COMPOUND_DIVISION:       return "/=";
					case UnifiedCXXOperator::COMPOUND_MODULO:         return "%=";
					case UnifiedCXXOperator::COMPOUND_BITWISE_AND:    return "&=";
					case UnifiedCXXOperator::COMPOUND_BITWISE_OR:     return "|=";
					case UnifiedCXXOperator::COMPOUND_BITWISE_XOR:    return "^=";
					case UnifiedCXXOperator::COMPOUND_LEFT_SHIFT:     return "<<=";
					case UnifiedCXXOperator::COMPOUND_RIGHT_SHIFT:    return ">>=";
					case UnifiedCXXOperator::SUBSCRIPT:               return "[]";
					case UnifiedCXXOperator::ARROW:                   return "->";
					case UnifiedCXXOperator::POINTER_TO_MEMBER_ARROW: return "->*";
					case UnifiedCXXOperator::DOT:                     return ".";
					case UnifiedCXXOperator::POINTER_TO_MEMBER_DOT:   return ".*";
					case UnifiedCXXOperator::COMMA:                   return ",";
					case UnifiedCXXOperator::SCOPE_RESOLUTION:        return "::";
					case UnifiedCXXOperator::DEREFERENCE:             return "*";
					case UnifiedCXXOperator::ADDRESS_OF:              return "&";
					case UnifiedCXXOperator::BITWISE_NEGATION:        return "~";
					case UnifiedCXXOperator::LOGICAL_NEGATION:        return "!";
					case UnifiedCXXOperator::UNARY_PLUS:              return "+";
					case UnifiedCXXOperator::UNARY_MINUS:             return "-";
					case UnifiedCXXOperator::PREFIX_INCREMENT:        return "++";
					case UnifiedCXXOperator::POSTFIX_INCREMENT:       return "++";
					case UnifiedCXXOperator::PREFIX_DECREMENT:        return "--";
					case UnifiedCXXOperator::POSTFIX_DECREMENT:       return "--";
					case UnifiedCXXOperator::FUNCTION_CALL:           return "()";
					case UnifiedCXXOperator::TERNARY:                 return "?:";

					default: return "<unknown>";
					}
				}

			private:
				type myType;
			};
		}
	}
}