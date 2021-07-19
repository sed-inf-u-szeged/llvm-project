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

// Factory class for actions.
class Factory : public FrontendActionFactory
{
public:
  Factory(metrics::detail::GlobalMergeData_ThreadSafe& data, const InvokeOptions& options) :
    rMyData(data),
    rMyOptions(options)
  {}

  std::unique_ptr<FrontendAction> create() override
  {
    metrics::detail::ClangMetricsAction* ptr = new metrics::detail::ClangMetricsAction(rMyData);
    ptr->debugPrintHalsteadAfterVisit(rMyOptions.enableHalsteadDebugPrint);
    ptr->shouldPrintTracingInfo = rMyOptions.enableProcessingTracePrint;
    return std::unique_ptr<FrontendAction>(ptr);
  }

private:
  metrics::detail::GlobalMergeData_ThreadSafe& rMyData;
  const InvokeOptions& rMyOptions;
};


bool metrics::invoke(Output& output, const CompilationDatabase& compilations, const CommandLineArguments& sourcePathList, InvokeOptions options)
{
  // Create the tool.
  ClangTool tool(compilations, sourcePathList);

  // Create factory and invoke main program.
  detail::GlobalMergeData_ThreadSafe gmd(output);
  std::unique_ptr<Factory> factory(new Factory(gmd, options));
  if (tool.run(factory.get()))
    return false;

  if (options.enableProcessingTracePrint) {
    std::cout << "Clang-metrics has finished processing all files, now aggregating results... " << std::endl;
  }

  gmd.call([&](detail::GlobalMergeData& mergeData) {
    // Do debug print.
    if (options.enableRangeDebugPrint)
      mergeData.debugPrintObjectRanges(std::cout);

    // Aggregate metrics.
    mergeData.aggregate();
  });

  return true;
}

void metrics::invoke(Output& output, clang::ASTContext& context, const std::vector<clang::Decl*>& declarations, const std::vector<clang::Stmt*>& statements, InvokeOptions options)
{
  detail::GlobalMergeData_ThreadSafe gmd(output);
  detail::ClangMetrics clangMetrics(gmd);
  clangMetrics.updateASTContext(context);
  clangMetrics.debugPrintHalsteadAfterVisit(options.enableHalsteadDebugPrint);

  detail::ClangMetrics::NodeVisitor visitor(clangMetrics);

  for (auto decl : declarations)
  {
    ASTPrePostTraverser traverser(context, decl, visitor);
    traverser.run();
  }
    
  for (auto stmt : statements)
  {
    ASTPrePostTraverser traverser(context, stmt, visitor);
    traverser.run();
  }  

  clangMetrics.aggregateMetrics();

  gmd.call([&](detail::GlobalMergeData& mergeData) {
    // Do debug print.
    if (options.enableRangeDebugPrint)
      mergeData.debugPrintObjectRanges(std::cout);

    // Aggregate metrics.
    mergeData.aggregate();
  });
}

Invocation::Invocation(std::unique_ptr<Output> output, InvokeOptions options) : pMyOutput(std::move(output)), options(options), pMyMergeData(std::make_unique<detail::GlobalMergeData_ThreadSafe>(*pMyOutput)) {}

Invocation::~Invocation() = default;

bool Invocation::invoke(const clang::tooling::CompilationDatabase& compilations, const clang::tooling::CommandLineArguments& sourcePathList)
{
  // Create the tool.
  ClangTool tool(compilations, sourcePathList);

  // Create factory and invoke main program.
  std::unique_ptr<Factory> factory(new Factory(*pMyMergeData, options));
  if (tool.run(factory.get()))
    return false;

  return true;
}

std::unique_ptr<Output> Invocation::aggregate()
{
  pMyMergeData->call([&](detail::GlobalMergeData& mergeData) {
    // Do debug print.
    if (options.enableRangeDebugPrint)
      mergeData.debugPrintObjectRanges(std::cout);

    // Aggregate metrics.
    mergeData.aggregate();
  });

  // Return filled output object
  return move(pMyOutput);
}