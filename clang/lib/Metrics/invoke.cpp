#include "ClangMetricsAction.h"
#include "NodeVisitor.h"

#include <clang/Metrics/ASTPrePostVisitor.h>
#include <clang/Metrics/invoke.h>
#include <clang/Metrics/Output.h>

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
    Factory(Output& output, detail::GlobalMergeData& data, const InvokeOptions& options) :
      rMyOutput(output),
      rMyData(data),
      rMyOptions(options)
    {}

    FrontendAction* create() override
    {
      detail::ClangMetricsAction* ptr = new detail::ClangMetricsAction(rMyOutput, rMyData);
      ptr->debugPrintHalsteadAfterVisit(rMyOptions.enableHalsteadDebugPrint);
      return ptr;
    }

  private:
    Output& rMyOutput;
    detail::GlobalMergeData& rMyData;
    const InvokeOptions& rMyOptions;
  };

  // Create factory and invoke main program.
  detail::GlobalMergeData gmd;
  std::unique_ptr<Factory> factory(new Factory(output, gmd, options));
  if (tool.run(factory.get()))
    return false;

  // Do debug print.
  if (options.enableRangeDebugPrint)
    gmd.debugPrintObjectRanges(std::cout);

  // Aggregate metrics.
  gmd.aggregate(output);

  return true;
}

void metrics::invoke(Output& output, clang::ASTContext& context, const std::vector<clang::Decl*>& declarations, const std::vector<clang::Stmt*>& statements, InvokeOptions options)
{
  detail::GlobalMergeData gmd;
  detail::ClangMetrics clangMetrics(output, gmd, context);
  clangMetrics.debugPrintHalsteadAfterVisit(options.enableHalsteadDebugPrint);

  output.getFactory().onSourceOperationBegin(context, "");

  detail::ClangMetrics::NodeVisitor visitor(clangMetrics);

  for (auto decl : declarations)
  {
    ASTPrePostTraverser traverser(context, visitor, decl);
    traverser.run();
  }
    
  for (auto stmt : statements)
  {
    ASTPrePostTraverser traverser(context, visitor, stmt);
    traverser.run();
  }  

  clangMetrics.aggregateMetrics();

  // Do debug print.
  if (options.enableRangeDebugPrint)
    gmd.debugPrintObjectRanges(std::cout);

  gmd.aggregate(output);
}
