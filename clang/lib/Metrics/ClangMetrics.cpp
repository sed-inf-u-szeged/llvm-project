#include "ClangMetrics.h"
#include "LOCMeasure.h"
#include <clang/Metrics/Output.h>
#include <clang/Metrics/MetricsUtility.h>

using namespace clang;
using namespace clang::metrics;
using namespace clang::metrics::detail;

void ClangMetrics::aggregateMetrics()
{
  using namespace std;

  rMyOutput.getFactory().onSourceOperationEnd(*pMyASTContext);

  // Helper for LOC calculation.
  LOCMeasure me(pMyASTContext->getSourceManager(), myCodeLines);
    
  // Function metrics
  for (auto decl : myFunctions)
  {

    FunctionMetrics m;
    
    LOCMeasure::LOC loc;
    LOCMeasure::LOC tloc;
    if (ObjCMethodDecl::classofKind(decl->getDeclKind()))
    {
      loc = me.calculate(cast<ObjCMethodDecl>(decl), LOCMeasure::ignore(myInsideClassesByFunctions));
      tloc = me.calculate(cast<ObjCMethodDecl>(decl));
      m.name = cast<ObjCMethodDecl>(decl)->getNameAsString();
    
    }
    else if (FunctionDecl::classofKind(decl->getDeclKind()))
    {
      loc = me.calculate(cast<FunctionDecl>(decl), LOCMeasure::ignore(myInsideClassesByFunctions));
      tloc = me.calculate(cast<FunctionDecl>(decl));
      m.name = cast<FunctionDecl>(decl)->getNameAsString();
    }

    m.LOC = loc.total;
    m.TLOC = tloc.total;
    m.LLOC = loc.logical;
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

    if (ObjCMethodDecl::classofKind(decl->getDeclKind()))
    {
      rMyOutput.mergeFunctionMetrics(cast<ObjCMethodDecl>(decl), m);
    }
    else if (FunctionDecl::classofKind(decl->getDeclKind()))
    {
      rMyOutput.mergeFunctionMetrics(cast<FunctionDecl>(decl), m);
    }
      
  }

  // Class metrics
  for (auto decl : myClasses)
  {
      
    LOCMeasure::LOC loc;
    LOCMeasure::LOC tloc;
   
  if (dyn_cast_or_null<ObjCContainerDecl>(decl))
    {
      tloc = me.calculate(decl);
      loc = me.calculate(decl, LOCMeasure::ignore(myInnerClassesByClasses));
    
    // Interfaces and Categories have Implementation lines of code to include
    LOCMeasure::LOC imploc;
    if (const ObjCInterfaceDecl* id = dyn_cast_or_null<ObjCInterfaceDecl>(decl))
    {
      if (const ObjCImplementationDecl* imp = id->getImplementation())
      {
        imploc = me.calculate(imp);
        tloc.logical += imploc.logical;
        tloc.total += imploc.total;
        loc.logical += imploc.logical;
        loc.total += imploc.total;
      }
    }
    else if (const ObjCCategoryDecl* id = dyn_cast_or_null<ObjCCategoryDecl>(decl))
    {
      if (!id->IsClassExtension())
      {
        if (const ObjCCategoryImplDecl* imp = id->getImplementation())
        {
          imploc = me.calculate(imp);
          tloc.logical += imploc.logical;
          tloc.total += imploc.total;
          loc.logical += imploc.logical;
          loc.total += imploc.total;
        }
      }
    }
    }
    else
    {
      tloc = me.calculate(decl,
        LOCMeasure::ignore(myMethodsByClasses),
        LOCMeasure::merge(myMethodsByClasses, false));
      loc = me.calculate(decl,
        LOCMeasure::ignore(myInnerClassesByClasses),
        LOCMeasure::ignore(myMethodsByClasses),
        LOCMeasure::merge(myMethodsByClasses, false));
    }

    // "Raw" metrics without methods
    auto tloc_raw = me.calculate(decl);
    auto loc_raw = me.calculate(decl, LOCMeasure::ignore(myInnerClassesByClasses));

    ClassMetrics m;
    if (CXXRecordDecl::classofKind(decl->getKind()))
      m.name = cast<CXXRecordDecl>(decl)->getNameAsString();
    else if (ObjCInterfaceDecl::classofKind(decl->getKind()))
      m.name = cast<ObjCInterfaceDecl>(decl)->getNameAsString();
    else if (ObjCProtocolDecl::classofKind(decl->getKind()))
      m.name = cast<ObjCProtocolDecl>(decl)->getNameAsString();
  else if (const ObjCCategoryDecl* cd = dyn_cast_or_null<ObjCCategoryDecl>(decl))
  {
      if (cd->IsClassExtension())
      {
        if (const ObjCInterfaceDecl* intf = cd->getClassInterface())
          m.name = "Ext:" + intf->getNameAsString();
      }
      else
        m.name = cd->getNameAsString();
  }
      

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
        for (const Decl* d : it->second)
        {
      if (CXXRecordDecl::classof(decl))
      {
        if (isInterface(cast<CXXRecordDecl>(d)))
          ++m.NIN;
      }
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
    helper.sm = &pMyASTContext->getSourceManager();

    auto loc = me.calculate(&helper);

    FileMetrics m;
    m.LOC  = loc.total;
    m.LLOC = loc.logical;
    m.McCC = myFileMcCC;

    rMyOutput.mergeFileMetrics(currentFile, m);
  }
    
  // Debug print Halstead metrics if requested.
  if (myDebugPrintAfterVisit)
  {
    std::cout << " --- HALSTEAD RESULTS BEGIN --- \n\n  Filename: " << currentFile << "\n\n\n";
    for (auto& hs : myHalsteadByFunctions)
    {
    if (ObjCMethodDecl::classofKind(hs.first->getDeclKind()))
    {
      std::cout << "  Function: " << cast<ObjCMethodDecl>(hs.first)->getNameAsString() << '\n';
    }
    else if (FunctionDecl::classofKind(hs.first->getDeclKind()))
    {
      std::cout << "  Function: " << cast<FunctionDecl>(hs.first)->getNameAsString() << '\n';
    }
      std::cout << "  "; hs.second.dbgPrintOperators();
      std::cout << "  "; hs.second.dbgPrintOperands();
      std::cout << "\n  \tOperators: " << hs.second.getOperatorCount() << "\tD: " << hs.second.getDistinctOperatorCount();
      std::cout << "\n  \tOperands:  " << hs.second.getOperandCount() << "\tD: " << hs.second.getDistinctOperandCount() << "\n\n\n";
    }
    std::cout << " --- HALSTEAD RESULTS END --- \n\n";
  }
}


