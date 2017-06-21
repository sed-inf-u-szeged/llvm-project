#pragma once

#include "UID.h"
#include "UIDFactory.h"

#include <string>
#include <functional>
#include <assert.h>


namespace clang
{
	class DeclContext;
	class MangleContext;
	class CompilerInstance;

	namespace metrics
	{
		/*!
		 *	ID for matching the same entities togather across different translation units by a specially generated
		 *  "mangled" name.
		 */
		class CAM_Id final : public UID
		{
		public:
			//! Constructor.
			//!  \param mangledName the mangled name identifying the decl
			CAM_Id(const std::string& mangledName) : myMangledName(mangledName)
			{}

		private:
			bool equals(const UID& rhs) const override
			{
				assert(typeid(rhs) == typeid(CAM_Id) && "Invalid UID type!");

				return myMangledName == static_cast<const CAM_Id&>(rhs).myMangledName;
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
		 *	Factory for creating CAM_Ids.
		 */
		class CAMIDFactory final : public UIDFactory
		{
		private:
			void onSourceOperationEnd(const clang::CompilerInstance& inst) override;
			std::unique_ptr<UID> create(const clang::Decl* decl) override;

		private:
			const clang::CompilerInstance* pMyInstance = nullptr;
			clang::MangleContext* pMyCtx = nullptr;
		};
	}
}