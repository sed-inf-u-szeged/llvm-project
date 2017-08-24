#include "ClangMetricsAction.h"
#include "Consumer.h"
#include "LOCMeasure.h"
#include "Output.h"
#include "MetricsUtility.h"


// Use some clang namespace for simplicity
using namespace clang;
using namespace metrics::detail;


std::unique_ptr<ASTConsumer> ClangMetricsAction::CreateASTConsumer(CompilerInstance& ci, StringRef file)
{
  return std::unique_ptr<ASTConsumer>(new Consumer(*this));
}

bool metrics::detail::ClangMetricsAction::BeginSourceFileAction(clang::CompilerInstance& ci, clang::StringRef filename)
{
  rMyOutput.getFactory().onSourceOperationBegin(ci, filename);

  return true;
}

void ClangMetricsAction::EndSourceFileAction()
{
  using namespace std;

  rMyOutput.getFactory().onSourceOperationEnd(getCompilerInstance());

  // Helper for LOC calculation.
  LOCMeasure me(getCompilerInstance().getSourceManager(), myCodeLines);

  // Function metrics
  for (auto decl : myFunctions)
  {
    auto loc  = me.calculate(decl, LOCMeasure::ignore(myInsideClassesByFunctions));
    auto tloc = me.calculate(decl);

    FunctionMetrics m;
    m.name  = decl->getNameAsString();
    m.LOC   = loc.total;
    m.TLOC  = tloc.total;
    m.LLOC  = loc.logical;
    m.TLLOC = tloc.logical;

    auto it = myMcCCByFunctions.find(decl);
    if (it != myMcCCByFunctions.end())
      m.McCC = it->second + 1;
    else
      m.McCC = 1;

    const HalsteadStorage& hs = myHalsteadByFunctions[decl];
    m.H_Operators  = hs.getOperatorCount();
    m.H_Operands   = hs.getOperandCount();
    m.HD_Operators = hs.getDistinctOperatorCount();
    m.HD_Operands  = hs.getDistinctOperandCount();

    rMyOutput.mergeFunctionMetrics(decl, m);
  }

  // Class metrics
  for (auto decl : myClasses)
  {
    auto tloc = me.calculate(decl,
      LOCMeasure::ignore(myMethodsByClasses),
      LOCMeasure::merge(myMethodsByClasses, false));
    auto loc = me.calculate(decl,
      LOCMeasure::ignore(myInnerClassesByClasses),
      LOCMeasure::ignore(myMethodsByClasses),
      LOCMeasure::merge(myMethodsByClasses, false));

    // "Raw" metrics without methods
    auto tloc_raw = me.calculate(decl);
    auto loc_raw  = me.calculate(decl, LOCMeasure::ignore(myInnerClassesByClasses));

    ClassMetrics m;
    m.name  = decl->getNameAsString();
    m.LOC   = loc.total;
    m.TLOC  = tloc.total;
    m.LLOC  = loc.logical;
    m.TLLOC = tloc.logical;

    // Number of local methods - already stored as the size of the set of the corresponding decl
    // in myMethodsByClasses.
    {
      auto it = myMethodsByClasses.find(decl);
      if (it != myMethodsByClasses.end())
        m.NLM = it->second.size();
      else
        m.NLM = 0;
    }

    rMyOutput.mergeClassMetrics(decl, m, tloc_raw.total, tloc_raw.logical, loc_raw.total, loc_raw.logical);
  }

  // Enum metrics
  for (auto decl : myEnums)
  {
    auto loc = me.calculate(decl);

    EnumMetrics m;
    m.name = decl->getNameAsString();
    m.LOC  = loc.total;
    m.LLOC = loc.logical;

    rMyOutput.mergeEnumMetrics(decl, m);
  }

  // Namespace metrics
  for (auto decl : myNamespaces)
  {
    auto tloc = me.calculate(decl);
    auto loc = me.calculate(decl, LOCMeasure::ignore(myInnerNamespacesByNamespaces));

    NamespaceMetrics m;
    m.name  = decl->getNameAsString();
    m.LOC   = loc.total;
    m.TLOC  = tloc.total;
    m.LLOC  = loc.logical;
    m.TLLOC = tloc.logical;

    // Number of classes and interfaces in the namespace - already stored in the set of the corresponding
    // decl in myClassesByNamespaces.
    {
      auto it = myClassesByNamespaces.find(decl);
      if (it != myClassesByNamespaces.end())
      {
        m.NCL = it->second.size();

        // Iterate over the classes and count the interfaces
        m.NIN = 0;
        for (const CXXRecordDecl* d : it->second)
        {
          if (isInterface(d))
            ++m.NIN;
        }
      }
      else
      {
        m.NCL = 0;
        m.NIN = 0;
      }
    }

    // Number of enums in the namespace - already stored as the size of the set of the corresponding
    // decl in myEnumsByNamespaces.
    {
      auto it = myEnumsByNamespaces.find(decl);
      if (it != myEnumsByNamespaces.end())
        m.NEN = it->second.size();
      else
        m.NEN = 0;
    }

    rMyOutput.mergeNamespaceMetrics(decl, m);
  }

  // File metrics
  {
    struct
    {
      SourceManager* sm;

      SourceLocation getLocStart() const { return sm->getLocForStartOfFile(sm->getMainFileID()); }
      SourceLocation getLocEnd() const { return sm->getLocForEndOfFile(sm->getMainFileID()); }
    } helper;
    helper.sm = &getCompilerInstance().getSourceManager();

    auto loc = me.calculate(&helper);

    FileMetrics m;
    m.LOC  = loc.total;
    m.LLOC = loc.logical;
    m.McCC = myFileMcCC;

    rMyOutput.mergeFileMetrics(getCurrentFile(), m);
  }

  // Debug print Halstead metrics if requested.
  if (myDebugPrintAfterVisit)
  {
    std::cout << " --- HALSTEAD RESULTS BEGIN --- \n\n  Filename: " << getCurrentFile().str() << "\n\n\n";
    for (auto& hs : myHalsteadByFunctions)
    {
      std::cout << "  Function: " << hs.first->getNameAsString() << '\n';
      std::cout << "  "; hs.second.dbgPrintOperators();
      std::cout << "  "; hs.second.dbgPrintOperands();
      std::cout << "\n  \tOperators: " << hs.second.getOperatorCount() << "\tD: " << hs.second.getDistinctOperatorCount();
      std::cout << "\n  \tOperands:  " << hs.second.getOperandCount() << "\tD: " << hs.second.getDistinctOperandCount() << "\n\n\n";
    }
    std::cout << " --- HALSTEAD RESULTS END --- \n\n";
  }
}
