#include "ClangMetrics.h"
#include "LOCMeasure.h"
#include <clang/Metrics/Output.h>
#include <clang/Metrics/MetricsUtility.h>

using namespace clang;
using namespace clang::metrics;
using namespace clang::metrics::detail;


const GlobalMergeData::Range GlobalMergeData::INVALID_RANGE = {};

void ClangMetrics::aggregateMetrics()
{
  using namespace std;

  rMyOutput.getFactory().onSourceOperationEnd(*pMyASTContext);

  // Helper for LOC calculation.
  /*LOCMeasure me(pMyASTContext->getSourceManager(), myCodeLines);
    
  // Function metrics:
  for (auto decl : myFunctions)
  {
    FunctionMetrics m;
    
    LOCMeasure::LOC loc;
    LOCMeasure::LOC tloc;
    if (ObjCMethodDecl::classofKind(decl->getDeclKind()))
    {
      loc    = me.calculate(cast<ObjCMethodDecl>(decl), LOCMeasure::ignore(myInsideClassesByFunctions));
      tloc   = me.calculate(cast<ObjCMethodDecl>(decl));
      m.name = cast<ObjCMethodDecl>(decl)->getNameAsString();
    }
    else if (FunctionDecl::classofKind(decl->getDeclKind()))
    {
      loc    = me.calculate(cast<FunctionDecl>(decl), LOCMeasure::ignore(myInsideClassesByFunctions));
      tloc   = me.calculate(cast<FunctionDecl>(decl));
      m.name = cast<FunctionDecl>(decl)->getNameAsString();
    }

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

    if (ObjCMethodDecl::classofKind(decl->getDeclKind()))
    {
      rMyOutput.mergeFunctionMetrics(cast<ObjCMethodDecl>(decl), m);
    }
    else if (FunctionDecl::classofKind(decl->getDeclKind()))
    {
      rMyOutput.mergeFunctionMetrics(cast<FunctionDecl>(decl), m);
    }  
  }

  // Class metrics:
  for (auto decl : myClasses)
  {   
    LOCMeasure::LOC loc;
    LOCMeasure::LOC tloc;
   
    if (dyn_cast_or_null<ObjCContainerDecl>(decl))
    {
      tloc = me.calculate(decl);
      loc = me.calculate(decl, LOCMeasure::ignore(myInnerClassesByClasses));
    
      // Interfaces and Categories have Implementation lines of code to include.
      LOCMeasure::LOC imploc;
      if (const ObjCInterfaceDecl* id = dyn_cast_or_null<ObjCInterfaceDecl>(decl))
      {
        if (const ObjCImplementationDecl* imp = id->getImplementation())
        {
          imploc = me.calculate(imp);

          tloc.logical += imploc.logical;
          tloc.total   += imploc.total;
          loc.logical  += imploc.logical;
          loc.total    += imploc.total;
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
            tloc.total   += imploc.total;
            loc.logical  += imploc.logical;
            loc.total    += imploc.total;
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

    // "Raw" metrics without methods.
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

  // Enum metrics:
  for (auto decl : myEnums)
  {
    auto loc = me.calculate(decl);

    EnumMetrics m;
    m.name = decl->getNameAsString();
    m.LOC  = loc.total;
    m.LLOC = loc.logical;

    rMyOutput.mergeEnumMetrics(decl, m);
  }

  // Namespace metrics:
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

    rMyOutput.mergeNamespaceMetrics(pMyASTContext->getSourceManager(), decl, m);
  }

  // File and TU metrics:
  {
    struct
    {
      SourceManager* sm;
      FileID fid;

      SourceLocation getLocStart() const { return sm->getLocForStartOfFile(fid); }
      SourceLocation getLocEnd() const { return sm->getLocForEndOfFile(fid); }
    } helper;

    SourceManager& sm = pMyASTContext->getSourceManager();
    helper.sm = &sm;

    FileMetrics tum {};
    for (auto it = sm.fileinfo_begin(); it != sm.fileinfo_end(); ++it)
    {
      helper.fid = sm.translateFile(it->first);

      auto loc = me.calculate(&helper);

      FileMetrics m;
      m.LOC  = loc.total;
      m.LLOC = loc.logical;

      // Init to 1, because the map may not contain any entries in a file without ifs, whiles, etc.
      m.McCC = 1;

      // Load McCC from the map if there's an entry.
      auto mcccit = myMcCCByFiles.find(helper.fid);
      if (mcccit != myMcCCByFiles.end())
        m.McCC = mcccit->second + 1;

      // Aggregate files into TU metrics.
      tum.LOC  += m.LOC;
      tum.LLOC += m.LLOC;

      // Subtract 1 because we only add it once at the end of the aggregation (because of McCC "plus one" definition).
      tum.McCC += m.McCC - 1;

      rMyOutput.mergeFileMetrics(it->first->getName(), m);
    }

    // Add 1 to the McCC of the TU, because of the "plus one" definition. Then merge.
    tum.McCC += 1;
    rMyOutput.mergeTranslationUnitMetrics(myCurrentTU, tum);
  }
    
  // Debug print Halstead metrics if requested.
  if (myDebugPrintAfterVisit)
  {
    std::cout << " --- HALSTEAD RESULTS BEGIN --- \n\n  Translation unit: " << myCurrentTU << "\n\n\n";
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
  }*/
}

void GlobalMergeData::addDecl(const clang::Decl* decl)
{
  assert(pMyAnalyzer && "Pointer to ClangMetrics should already be set at this point.");

  UIDFactory& factory = pMyAnalyzer->getUIDFactory();
  
  Range::range_t type;
  Range::oper_t  oper;
  Object::kind_t  kind;
  const Range* parent = nullptr;

  auto createTemporaryRange = [this](const Decl* decl)
  {
     SourceManager& sm = pMyAnalyzer->getASTContext()->getSourceManager();

      SourceLocation start = decl->getLocStart();
      SourceLocation end   = decl->getLocEnd();

      Range r;
      r.fileID      = fileid(sm.getFilename(start));
      r.lineBegin   = sm.getExpansionLineNumber(start);
      r.lineEnd     = sm.getExpansionLineNumber(end);
      r.columnBegin = sm.getExpansionColumnNumber(start);
      r.columnEnd   = sm.getExpansionColumnNumber(end);

      return r;
  };

  auto addNamespaceRange = [&](const DeclContext* pn)
  {
    if (isa<NamespaceDecl>(pn))
    {
      auto it = myObjects.find({ factory.create(cast<Decl>(pn)), Object::kind_t() });
      if (it != myObjects.end())
      {

        Range r = createTemporaryRange(decl);

        auto rit = it->second.lower_bound(&r);
        if (rit != it->second.begin())
        {
          const Range* p = *(--rit);
          if (containsRange(*p, r))
            parent = p;
        }
      }
    }
  };

  if (auto d = dyn_cast<FunctionDecl>(decl))
  {
    type = (d->isThisDeclarationADefinition() ? Range::DEFINITION : Range::DECLARATION);
    kind = Object::FUNCTION;
    oper = Range::LOC_METHOD;

    if (auto d = dyn_cast<CXXMethodDecl>(decl))
    {
      const Range* parentRange = getDefinition(factory.create(d->getParent()));
      if (parentRange && parentRange->type == Range::DEFINITION && !containsRange(*parentRange, createTemporaryRange(d)))
        parent = parentRange;
    }
  }
  else if (auto d = dyn_cast<CXXRecordDecl>(decl))
  {
    type = (d->getDefinition() == d ? Range::DEFINITION : Range::DECLARATION);
    kind = Object::CLASS;
    oper = Range::LOC_SUBTRACT;

    const DeclContext* pn = d->getParent();
    if (pn)
    {
      if (isa<CXXRecordDecl>(pn) || isa<FunctionDecl>(pn))
      {
        const Range* parentRange = getDefinition(factory.create(cast<Decl>(pn)));
        if (parentRange && parentRange->type == Range::DEFINITION)
          parent = parentRange;
      }
      else
      {
        addNamespaceRange(pn);
      }
    }
  }
  else if (auto d = dyn_cast<EnumDecl>(decl))
  {
    type = (d->getDefinition() == d ? Range::DEFINITION : Range::DECLARATION);
    kind = Object::ENUM;
    oper = Range::oper_t();
  }
  else if (auto d = dyn_cast<NamespaceDecl>(decl))
  {
    type = Range::DEFINITION;
    kind = Object::NAMESPACE;
    oper = Range::LOC_SUBTRACT;

    if (const DeclContext* pn = d->getParent())
      addNamespaceRange(pn);
  }
  else
  {
    return;
  }

  const Range& range = createRange(type, decl->getSourceRange(), parent, oper);
  myObjects[{ factory.create(decl), kind }].insert(&range);
}

void GlobalMergeData::addCodeLine(SourceLocation loc)
{
  assert(pMyAnalyzer && "Pointer to ClangMetrics should already be set at this point.");
  SourceManager& sm = pMyAnalyzer->getASTContext()->getSourceManager();

  unsigned fid = fileid(sm.getFilename(loc));
  if (fid == 0)
    return;

  myCodeLines.emplace(fid, sm.getExpansionLineNumber(loc));
}

void GlobalMergeData::aggregate(Output& output) const
{
  struct LOCInfo
  {
    unsigned LOC   = 0;
    unsigned LLOC  = 0;
    unsigned TLOC  = 0;
    unsigned TLLOC = 0;
  };

  std::unordered_map<const Range*, LOCInfo> locmap;
  for (const Range& range : myRanges)
  {
    LOCInfo& info = locmap[&range];

    info.LOC  = range.lineEnd - range.lineBegin + 1;

    auto beg  = myCodeLines.lower_bound({ range.fileID, range.lineBegin });
    auto end  = myCodeLines.upper_bound({ range.fileID, range.lineEnd });
    info.LLOC = std::distance(beg, end);

    info.TLOC  = info.LOC;
    info.TLLOC = info.LLOC;

    if (const Range* parent = range.parent)
    {
      LOCInfo& parentInfo = locmap[parent];
      if (range.operation == Range::LOC_METHOD)
      {
        parentInfo.LOC   += info.TLOC;
        parentInfo.LLOC  += info.TLLOC;
        parentInfo.TLOC  += info.TLOC;
        parentInfo.TLLOC += info.TLLOC;
      }
      else
      {
        parentInfo.LOC  -= info.LOC;
        parentInfo.LLOC -= info.LLOC;
      }
    }
  }

  for (auto& object : myObjects)
  {
    if (object.first.kind == Object::FUNCTION)
    {
      FunctionMetrics& m = output.myFunctionMetrics[object.first.uid];
      auto it = locmap.find(getDefinition(object.first.uid));
      if (it != locmap.end())
      {
        /* TODO: Remove this! */m.name = object.first.uid->getDebugName();
        m.LOC   = it->second.LOC;
        m.LLOC  = it->second.LLOC;
        m.TLOC  = it->second.TLOC;
        m.TLLOC = it->second.TLLOC;
      }
    }
    else if (object.first.kind == Object::CLASS)
    {
      ClassMetrics& m = output.myClassMetrics[object.first.uid];
      auto it = locmap.find(getDefinition(object.first.uid));
      if (it != locmap.end())
      {
        /* TODO: Remove this! */m.name = object.first.uid->getDebugName();
        m.LOC   = it->second.LOC;
        m.LLOC  = it->second.LLOC;
        m.TLOC  = it->second.TLOC;
        m.TLLOC = it->second.TLLOC;
      }
    }
    else if (object.first.kind == Object::ENUM)
    {
      EnumMetrics& m = output.myEnumMetrics[object.first.uid];
      auto it = locmap.find(getDefinition(object.first.uid));
      if (it != locmap.end())
      {
        /* TODO: Remove this! */m.name = object.first.uid->getDebugName();
        m.LOC  = it->second.LOC;
        m.LLOC = it->second.LLOC;
      }
    }
    else if (object.first.kind == Object::NAMESPACE)
    {
      NamespaceMetrics& m = output.myNamespaceMetrics[object.first.uid];
      for (const Range* range : object.second)
      {
        auto it = locmap.find(range);
        if (it != locmap.end())
        {
          /* TODO: Remove this! */m.name = object.first.uid->getDebugName();
          m.LOC   += it->second.LOC;
          m.LLOC  += it->second.LLOC;
          m.TLOC  += it->second.TLOC;
          m.TLLOC += it->second.TLLOC;
        }
      }
    }
  }
}

void GlobalMergeData::debugPrintObjectRanges(std::ostream& os) const
{
  auto printRange = [&os](const Range& range)
  {
    os << '#' << &range << " in file ID " << range.fileID;
    os << " (" << range.lineBegin << ", " << range.columnBegin << ") -> (" << range.lineEnd << ", " << range.columnEnd << ')';

    if (range.parent)
      os << " in #" << range.parent;

    os << '\n';
  };

  for (auto& object : myObjects)
  {
    switch (object.first.kind)
    {
    case Object::FUNCTION:  os << "FUNCTION: ";  break;
    case Object::CLASS:     os << "CLASS: ";     break;
    case Object::ENUM:      os << "ENUM: ";      break;
    case Object::NAMESPACE: os << "NAMESPACE: "; break;
    }

    std::string debugName = object.first.uid->getDebugName();
    if (!debugName.empty())
      os << debugName << '\n';
    else
      os << "(no debug information given)\n";

    if (!object.second.empty())
    {
      if (object.first.kind != Object::NAMESPACE)
      {
        os << "Definition at: ";
        const Range* def = getDefinition(object.first.uid);
        if (def && def->type == Range::DEFINITION)
          printRange(*def);
        else
          os << "no definition\n";

        if (object.second.size() == 1)
        {
          const Range* dr = *object.second.begin();
          if (dr->type == Range::DEFINITION)
          {
            os << "Declarations: definition only\n";
          }
          else
          {
            os << "Declaration: ";
            printRange(*dr);
          }
        }
        else if (object.second.size() == 2)
        {
          auto it = object.second.begin();
          if ((*it)->type == Range::DEFINITION)
            ++it;

          os << "Declaration: ";
          printRange(**it);
        }
        else
        {
          os << "Declarations:\n";
          for (const Range* r : object.second)
          {
            if (r->type == Range::DECLARATION)
              os << " - ", printRange(*r);
          }
        }
      }
      else
      {
        if (object.second.size() == 1)
        {
          os << "Range: ";
          printRange(**object.second.begin());
        }
        else
        {
          os << "Ranges:\n";
          for (const Range* r : object.second)
            os << " - ", printRange(*r);
        }
      }
    }
    else
    {
      os << "No range information stored.\n";
    }

    os << '\n';
  }

  os << "\nFiles:\n";
  for (auto& file : myFileIDs)
    os << " - ID: " << file.second << "\tFile: " << file.first << '\n';

  os << '\n';
}

unsigned GlobalMergeData::fileid(const std::string& filename)
{
  if (filename.empty())
    return 0;

  auto it = myFileIDs.find(filename);
  if (it != myFileIDs.end())
    return it->second;

  return myFileIDs.emplace(filename, myNextFileID++).first->second;
}

const GlobalMergeData::Range&
GlobalMergeData::createRange(Range::range_t type, const std::string& filename, unsigned lineBegin,
  unsigned lineEnd, unsigned columnBegin, unsigned columnEnd, const Range* parent, Range::oper_t operation)
{
  if (lineBegin > lineEnd)
    return INVALID_RANGE;

  if (lineBegin == lineEnd && columnBegin > columnEnd)
    return INVALID_RANGE;

  unsigned fid = fileid(filename);
  if (fid == 0)
    return INVALID_RANGE;

  Range range;
  range.parent      = parent;
  range.fileID      = fid;
  range.lineBegin   = lineBegin;
  range.lineEnd     = lineEnd;
  range.columnBegin = columnBegin;
  range.columnEnd   = columnEnd;
  range.type        = type;
  range.operation   = operation;

  return *myRanges.insert(range).first;
}

const GlobalMergeData::Range&
GlobalMergeData::createRange(Range::range_t type, SourceLocation start, SourceLocation end, const Range* parent, Range::oper_t operation)
{
  if (start.isInvalid() || end.isInvalid())
    return INVALID_RANGE;

  assert(pMyAnalyzer && "Pointer to ClangMetrics should already be set at this point.");

  SourceManager& sm = pMyAnalyzer->getASTContext()->getSourceManager();
  return createRange(type, sm.getFilename(start), sm.getExpansionLineNumber(start),
    sm.getExpansionLineNumber(end), sm.getExpansionColumnNumber(start), sm.getExpansionColumnNumber(end), parent, operation);
}

const GlobalMergeData::Range& GlobalMergeData::createRange(Range::range_t type, SourceRange r, const Range* parent, Range::oper_t operation)
{
  return createRange(type, r.getBegin(), r.getEnd(), parent, operation);
}

const GlobalMergeData::Range* GlobalMergeData::getDefinition(std::shared_ptr<UID> uid) const
{
  auto it = myObjects.find({ uid, Object::kind_t() });
  if (it == myObjects.end())
    return nullptr;

  if (!it->second.empty())
  {
    for (const Range* range : it->second)
    {
      if (range->type == Range::DEFINITION)
        return range;
    }

    return *it->second.begin();
  }

  return nullptr;
}

bool GlobalMergeData::containsRange(const Range& outer, const Range& inner)
{
  if (outer.fileID != inner.fileID)
    return false;

  if (outer.lineBegin > inner.lineBegin)
    return false;

  if (outer.lineEnd < inner.lineEnd)
    return false;

  if (outer.lineBegin == inner.lineBegin && outer.columnBegin > inner.columnBegin)
    return false;

  if (outer.lineEnd == inner.lineEnd && outer.columnEnd < inner.columnEnd)
    return false;

  return true;
}
