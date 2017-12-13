#include "ClangMetricsAction.h"
#include "Consumer.h"
#include <clang/Metrics/Output.h>
#include <clang/Metrics/MetricsUtility.h>


// Use some clang namespace for simplicity
using namespace clang;
using namespace metrics::detail;


std::unique_ptr<ASTConsumer> ClangMetricsAction::CreateASTConsumer(CompilerInstance& ci, StringRef file)
{
  rMyOutput.getFactory().onSourceOperationBegin(ci.getASTContext(), file);
  updateASTContext(ci.getASTContext());
  updateCurrentFile(file);
  return std::unique_ptr<ASTConsumer>(new Consumer(*this));
}

void ClangMetricsAction::EndSourceFileAction()
{
  aggregateMetrics();
}
