#pragma once

namespace clang
{
  class CXXRecordDecl;
  class ClassScopeFunctionSpecializationDecl;
  class ASTContext;
  class FunctionDecl;

  namespace metrics
  {
    // Returns true if and only if decl is an interface.
    // An interface in C++ (according to this function) is defined as follows:
    //  - It is of class or struct type with an identifier (anonymous classes are not allowed).
    //  - It has a virtual destructor.
    //  - Other than the virtual destructor, it has only static functions, static const variables and pure virtual functions (with or without implementation).
    //  - It contains no anonymous classes.
    //  - All of its base classes can be categorized as interfaces too, according to the above definition.
    bool isInterface(const clang::CXXRecordDecl* decl);
  }

  /// This is a colossal hack. Class scope function specializations are not allowed by the c++ standard, on the other hand
  /// they are allowed in Clang as a Microsoft extension.
  /// They build very differently than normal templates, and if the class doesn't get instantiated, they don't build at all.
  /// So to bypass this, we build them manually. Deducing the original template and template arguments by hand.
  /// This may break if Clang gets updated and it may not always produce correct results.
  bool manuallyExpandClassScopeFunctionSpecializationDecl(const clang::ClassScopeFunctionSpecializationDecl * decl, clang::ASTContext & context);

  ///Helper to determine if function is a lambda (couldn't find this call in libtool)
  bool isLambda(const clang::FunctionDecl *decl);
}


