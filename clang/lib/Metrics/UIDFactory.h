#pragma once

#include "UID.h"

#include <llvm/ADT/StringRef.h>

#include <memory>


namespace clang
{
	class Decl;
	class CompilerInstance;

	namespace metrics
	{
		/*!
		 *	Abstract class used for creating custom UIDs to AST nodes.
		 *
		 *  \sa UID, Output
		 */
		class UIDFactory
		{
		public:
			virtual ~UIDFactory() {}

			//! Creates a unique identifier (UID) for the given clang::Decl.
			virtual std::unique_ptr<UID> create(const clang::Decl*) = 0;

			//! Callback for custom code to be run at the beginning of each source operation.
			//!  \param ci reference to the clang::CompilerInstance
			//!  \param filename the name of the source file
			virtual void onSourceOperationBegin(const clang::CompilerInstance& ci, llvm::StringRef filename) {}

			//! Callback for custom code to be run at the end of each source operation.
			//!  \param ci reference to the clang::CompilerInstance
			virtual void onSourceOperationEnd(const clang::CompilerInstance& ci) {}
		};
	}
}