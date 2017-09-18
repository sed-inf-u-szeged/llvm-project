#pragma once

#include "UID.h"
#include "UIDFactory.h"

#include <clang/AST/AST.h>
#include <clang/AST/Mangle.h>

#include <string>
#include <functional>
#include <assert.h>


namespace clang
{
  class DeclContext;
  class CompilerInstance;

  namespace metrics
  {
    /*!
     *  ID for matching the same entities togather across different translation units by a specially generated
     *  "mangled" name.
     */
    class BasicUID final : public UID
    {
    public:
      //! Constructor.
      //!  \param mangledName the mangled name identifying the decl
      BasicUID(const std::string& mangledName) : myMangledName(mangledName)
      {}

    private:
      bool equals(const UID& rhs) const override
      {
        //assert(typeid(rhs) == typeid(BasicUID) && "Invalid UID type!");

        return myMangledName == static_cast<const BasicUID&>(rhs).myMangledName;
      }

      std::size_t hash() const override
      {
        return std::hash<std::string>()(myMangledName);
      }

    private:
      // Stores the mangled name of the entity
      const std::string myMangledName;
    };

    /*!
     *  Factory for creating CAM_Ids.
     */
    class BasicUIDFactory final : public UIDFactory
    {
    private:
      void onSourceOperationEnd(const clang::CompilerInstance& inst) override;
      std::unique_ptr<UID> create(const clang::Decl* decl) override;

    private:
      const clang::CompilerInstance* pMyInstance = nullptr;
      std::unique_ptr<clang::MangleContext> pMyCtx;
    };
  }
}