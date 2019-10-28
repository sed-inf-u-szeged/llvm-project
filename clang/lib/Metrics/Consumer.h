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
        Consumer(ClangMetricsAction& action, Output& output) : myVisitor(action), output(output)
        {}

        void HandleTranslationUnit(clang::ASTContext& context)
        {
          ASTPrePostTraverser traverser(context, myVisitor, &output);
          traverser.run();
        }

      private:
        NodeVisitor myVisitor;
        Output& output;
      };
    }
  }
}