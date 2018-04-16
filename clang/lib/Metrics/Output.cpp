#include <clang/Metrics/Output.h>

#include <clang/AST/Mangle.h>
#include <clang/AST/AST.h>


using namespace clang::metrics;

namespace
{
  // Helper function to insert an element if the container does not contain it yet.
  // The inserted element will be default constructed.
  // If the element already exists, the function returns a pointer to it if condition is true.
  // Returns a pointer to the element, or nullptr, if no insertion took place and condition is false.
  template<class T> typename T::value_type::second_type* loadOnCondition(T& container, std::shared_ptr<UID> uid, bool condition)
  {
    auto it = container.find(uid);
    if (it == container.end())
      return &container[std::move(uid)];

    if (condition)
      return &it->second;

    return nullptr;
  }
}

void Output::mergeFunctionMetrics(const clang::Decl* decl, const FunctionMetrics& m)
{
  // TODO: ObjC metrics!
  if (const clang::FunctionDecl* fd = dyn_cast<FunctionDecl>(decl))
  {
    if (FunctionMetrics* om = loadOnCondition(myFunctionMetrics, rMyFactory.create(decl), fd->isThisDeclarationADefinition()))
      *om = m;
  }
}

void Output::mergeClassMetrics(const clang::Decl* decl, const ClassMetrics& m, unsigned tlocT_raw, unsigned tlocL_raw, unsigned locT_raw, unsigned locL_raw)
{
  // TODO: ObjC metrics!
  ClassMetrics* cm = nullptr;
  if (const CXXRecordDecl* cd = dyn_cast<CXXRecordDecl>(decl))
    cm = loadOnCondition(myClassMetrics, rMyFactory.create(decl), cd->getDefinition() == decl);

  if (cm)
  {
    ClassMetrics& om = *cm;

    // First occurence only.
    if (om.name.empty())
    {
      om.name = m.name;

      // NLM is set on first occurence - it cannot change later anyway.
      om.NLM = m.NLM;
    }
    else
    {
      // Subtract the "raw class" metrics to avoid counting them multiple times.
      // These are the lines inside the class definition.
      om.LOC   -= locT_raw;
      om.TLOC  -= tlocT_raw;
      om.LLOC  -= locL_raw;
      om.TLLOC -= tlocL_raw;
    }

    if (clang::ObjCInterfaceDecl::classofKind(decl->getKind()))
    {
      if (m.NLM > om.NLM)
      {
        om.NLM = m.NLM;
      }
    }

    // Merge the size metrics to the ones already here.
    om.LOC   += m.LOC;
    om.TLOC  += m.TLOC;
    om.LLOC  += m.LLOC;
    om.TLLOC += m.TLLOC;
  }
}

void Output::mergeEnumMetrics(const clang::EnumDecl* decl, const EnumMetrics& m)
{
  // TODO: ObjC metrics!
  if (EnumMetrics* cm = loadOnCondition(myEnumMetrics, rMyFactory.create(decl), decl->getDefinition() == decl))
  {
    *cm = m;
    /*EnumMetrics& om = *cm;

    // First occurence only
    if (om.name.empty())
      om.name = m.name;

    om.LOC  += m.LOC;
    om.LLOC += m.LLOC;*/
  }
}

void Output::mergeNamespaceMetrics(const clang::NamespaceDecl* decl, const NamespaceMetrics& m)
{
  NamespaceMetrics& om = myNamespaceMetrics[rMyFactory.create(decl)];

  // First occurence only.
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

void Output::mergeTranslationUnitMetrics(const std::string& file, const FileMetrics& m)
{
  myTranslationUnitMetrics[file] = m;
}
