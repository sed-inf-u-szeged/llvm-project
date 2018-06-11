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
        Consumer(ClangMetricsAction& action) : myVisitor(action)
        {}

        void HandleTranslationUnit(clang::ASTContext& context)
        {
          ASTPrePostTraverser traverser(context, myVisitor);
          traverser.run();
        }

      private:
        NodeVisitor myVisitor;
      };
    }
  }
}