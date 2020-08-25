#include "ClangMetricsAction.h"
#include "Consumer.h"
#include <clang/Metrics/MetricsUtility.h>
#include <clang/Metrics/Output.h>

// Use some clang namespace for simplicity
using namespace clang;
using namespace metrics::detail;

std::unique_ptr<ASTConsumer> ClangMetricsAction::CreateASTConsumer(CompilerInstance &ci, StringRef file)
{
  if (shouldPrintTracingInfo) {
    std::cout << "Clang-metrics processing file: " << file.str() << " ..." << std::endl;
  }

  rMyOutput.getFactory().onSourceOperationBegin(ci.getASTContext(), file);
  updateASTContext(ci.getASTContext());
  updateCurrentTU(file);
  return std::unique_ptr<ASTConsumer>(new Consumer(*this, output));
}

void ClangMetricsAction::EndSourceFileAction() {
  aggregateMetrics();
  gmd.call([&](detail::GlobalMergeData& mergeData) {
    for (auto kv : mergeData.myFileIDs) {
      output.filesAlreadyProcessed.insert(kv.first);
    }
  });

}
