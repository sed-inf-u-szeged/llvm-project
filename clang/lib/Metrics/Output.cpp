#include <clang/Metrics/Output.h>

#include <clang/AST/Mangle.h>
#include <clang/AST/AST.h>


using namespace clang::metrics;

void Output::mergeFunctionMetrics(const clang::FunctionDecl* decl, const FunctionMetrics& m)
{
  FunctionMetrics& om = myFunctionMetrics[rMyFactory.create(decl)];

  om = m;
}

void Output::mergeClassMetrics(const clang::CXXRecordDecl* decl, const ClassMetrics& m, unsigned tlocT_raw, unsigned tlocL_raw, unsigned locT_raw, unsigned locL_raw)
{
  ClassMetrics& om = myClassMetrics[rMyFactory.create(decl)];

  // First occurence only
  if (om.name.empty())
  {
    om.name = m.name;

    // NLM is set on first occurence - it cannot change later anyway
    om.NLM = m.NLM;
  }
  else
  {
    // Subtract the "raw class" metrics to avoid counting them multiple times
    // These are the lines inside the class definition
    om.LOC   -= locT_raw;
    om.TLOC  -= tlocT_raw;
    om.LLOC  -= locL_raw;
    om.TLLOC -= tlocL_raw;
  }

  // Merge the size metrics to the ones already here
  om.LOC   += m.LOC;
  om.TLOC  += m.TLOC;
  om.LLOC  += m.LLOC;
  om.TLLOC += m.TLLOC;
}

void Output::mergeEnumMetrics(const clang::EnumDecl* decl, const EnumMetrics& m)
{
  EnumMetrics& om = myEnumMetrics[rMyFactory.create(decl)];

  // First occurence only
  if (om.name.empty())
    om.name = m.name;

  om.LOC  += m.LOC;
  om.LLOC += m.LLOC;
}

void Output::mergeNamespaceMetrics(const clang::NamespaceDecl* decl, const NamespaceMetrics& m)
{
  NamespaceMetrics& om = myNamespaceMetrics[rMyFactory.create(decl)];

  // First occurence only
  if (om.name.empty())
    om.name = m.name;

  om.LOC   += m.LOC;
  om.TLOC  += m.TLOC;
  om.LLOC  += m.LLOC;
  om.TLLOC += m.TLLOC;

  om.NCL += m.NCL;
  om.NEN += m.NEN;
  om.NIN += m.NIN;
}

void Output::mergeFileMetrics(const std::string& file, const FileMetrics& m)
{
  myFileMetrics[file] = m;
}