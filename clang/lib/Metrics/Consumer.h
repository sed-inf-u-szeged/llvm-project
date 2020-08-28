#pragma once

#include "NodeVisitor.h"


namespace clang
{
  namespace metrics
  {
    namespace detail
    {
      //! ASTConsumer for the NodeVisitor.
      //! See the Clang documentation for more info.
      class ClangMetricsAction::Consumer final : public clang::ASTConsumer
      {
      public:
        Consumer(ClangMetricsAction& action, clang::metrics::detail::GlobalMergeData_ThreadSafe& gmd) : myVisitor(action), gmd(gmd)
        {}

        void HandleTranslationUnit(clang::ASTContext& context)
        {
          ASTPrePostTraverser traverser(context, myVisitor, &gmd);
          traverser.run();
        }

      private:
        NodeVisitor myVisitor;
        clang::metrics::detail::GlobalMergeData_ThreadSafe& gmd;
      };
    }
  }
}