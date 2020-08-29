#pragma once

#include "UID.h"

#include <llvm/ADT/StringRef.h>
#include <clang/AST/Mangle.h>

#include <memory>


namespace clang
{
  class Decl;
  class ASTContext;

  namespace metrics
  {
    /*!
     *  Abstract class used for creating custom UIDs to AST nodes.
     *
     *  \sa UID, Output
     */
    class UIDFactory
    {
    public:
      virtual ~UIDFactory() {}

      //! Creates a unique identifier (UID) for the given clang::Decl.
      virtual std::unique_ptr<UID> create(const clang::Decl* decl, std::shared_ptr<clang::MangleContext> mangleContext) = 0;
    };
  }
}
