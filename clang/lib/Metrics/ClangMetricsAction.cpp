#include "ClangMetricsAction.h"
#include "Consumer.h"
#include <clang/Metrics/Output.h>
#include <clang/Metrics/MetricsUtility.h>


// Use some clang namespace for simplicity
using namespace clang;
using namespace metrics::detail;


std::unique_ptr<ASTConsumer> ClangMetricsAction::CreateASTConsumer(CompilerInstance& ci, StringRef file)
{
  std::cout << "clang-metrics processing file: " << file.str() << " ..." << std::endl;
  rMyOutput.getFactory().onSourceOperationBegin(ci.getASTContext(), file);
  updateASTContext(ci.getASTContext());
  updateCurrentTU(file);
  return std::unique_ptr<ASTConsumer>(new Consumer(*this,output));
}

void ClangMetricsAction::EndSourceFileAction()
{
  aggregateMetrics();
  for (auto kv : gmd.myFileIDs)
  {
    output.filesAlreadyProcessed.insert(kv.first);
    //std::cout << "file processed: " << kv.first << std::endl;
  }
}
