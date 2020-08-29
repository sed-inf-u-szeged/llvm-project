#include "ClangMetricsAction.h"
#include "Consumer.h"
#include <clang/Metrics/MetricsUtility.h>

// Use some clang namespace for simplicity
using namespace clang;
using namespace metrics::detail;

std::unique_ptr<ASTConsumer> ClangMetricsAction::CreateASTConsumer(CompilerInstance &ci, StringRef file)
{
  if (shouldPrintTracingInfo) {
    std::cout << "Clang-metrics processing file: " << file.str() << " ..." << std::endl;
  }
  
  updateASTContext(ci.getASTContext());
  updateCurrentTU(file);
  return std::unique_ptr<ASTConsumer>(new Consumer(*this, gmd));
}

void ClangMetricsAction::EndSourceFileAction() {
  aggregateMetrics();
}
