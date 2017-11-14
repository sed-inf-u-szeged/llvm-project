#pragma once

#include "Halstead.h"
#include "ClangMetrics.h"
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>


#include <utility>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <memory>


namespace clang
{
  namespace metrics
  {
    class Output;

    namespace detail
    {
      /*!
       *  FrontendAction which calculates basic code measurements.
       *  Serves as an entry point for each translation unit operation.
       */
      class ClangMetricsAction final : public clang::ASTFrontendAction, public ClangMetrics
      {

      public:
        //! Constructor.
        //!  \param output reference to the Output object where the results will be stored
        ClangMetricsAction(Output& output) : ClangMetrics(output)
        {}

      private:
        // Called by the libtooling library. Should not be called manually.
        std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& ci, clang::StringRef file) override;

        // Called by the libtooling library at the end of source operations. Should not be called manually.
        void EndSourceFileAction() override;

      private:
        // ASTConsumer for the NodeVisitor.
        // See the Clang documentation for more info.
        class Consumer;

      };
    }
  }
}
