#include <clang/Metrics/invoke.h>
#include <clang/Metrics/Output.h>
#include "ClangMetricsAction.h"

#include <clang/Tooling/Tooling.h>


// Use namespaces for simplicity.
using namespace clang;
using namespace tooling;
using namespace metrics;


bool metrics::invoke(Output& output, const CompilationDatabase& compilations, const CommandLineArguments& sourcePathList, InvokeOptions options)
{
  // Create the tool.
  ClangTool tool(compilations, sourcePathList);

  // Factory class for actions.
  class Factory : public FrontendActionFactory
  {
  public:
    Factory(Output& output, const InvokeOptions& options) : rMyOutput(output), rMyOptions(options)
    {}

    FrontendAction* create() override
    {
      detail::ClangMetricsAction* ptr = new detail::ClangMetricsAction(rMyOutput);
      ptr->debugPrintAfterVisit(rMyOptions.enableDebugPrint);
      return ptr;
    }

  private:
    Output& rMyOutput;
    const InvokeOptions& rMyOptions;
  };

  // Create factory and invoke main program.
  std::unique_ptr<Factory> factory(new Factory(output, options));
  return !tool.run(factory.get());
}
