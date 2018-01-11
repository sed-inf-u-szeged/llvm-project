#pragma once

namespace clang
{
  class CXXRecordDecl;

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
}