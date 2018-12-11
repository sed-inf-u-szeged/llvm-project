#include "ClangMetrics.h"

#include <clang/Metrics/Output.h>
#include <clang/Metrics/MetricsUtility.h>

using namespace clang;
using namespace clang::metrics;
using namespace clang::metrics::detail;


const GlobalMergeData::Range GlobalMergeData::INVALID_RANGE = {};

void ClangMetrics::aggregateMetrics()
{
  using namespace std;

  UIDFactory& factory = rMyOutput.getFactory();
  factory.onSourceOperationEnd(*pMyASTContext);

  // Function metrics:
  for (auto& met : myFunctionMetrics)
  {
    auto declaration = cast<Decl>(met.first);
    FunctionMetrics& m = rMyOutput.myFunctionMetrics[factory.create(declaration)];

    if (declaration != nullptr &&
        declaration->getAsFunction() != nullptr &&
        declaration->getAsFunction()->isThisDeclarationADefinition())
    {
      m.McCC = met.second.McCC;
      m.H_Operators  = met.second.hsStorage.getOperatorCount();
      m.H_Operands   = met.second.hsStorage.getOperandCount();
      m.HD_Operators = met.second.hsStorage.getDistinctOperatorCount();
      m.HD_Operands  = met.second.hsStorage.getDistinctOperandCount();
      m.NOS          = met.second.NOS;
      m.NL           = met.second.NL.getNestingLevel();
      m.NLE          = met.second.NLE.getNestingLevel();
    }
  }

  // File and TU metrics:
  {
    SourceManager& sm = pMyASTContext->getSourceManager();

    FileMetrics& tum = rMyOutput.myTranslationUnitMetrics[myCurrentTU];
    std::vector<const FileEntry*> fileEntries;
    for (auto it = sm.fileinfo_begin(); it != sm.fileinfo_end(); ++it)
    {
      fileEntries.push_back(it->first);
    }

    for (auto fileEntiry : fileEntries)
    {
      // Get line numbers.
      FileID fid = sm.translateFile(fileEntiry);
      unsigned lineBegin = sm.getExpansionLineNumber(sm.getLocForStartOfFile(fid));
      unsigned lineEnd   = sm.getExpansionLineNumber(sm.getLocForEndOfFile(fid));

      // Create metrics object.
      FileMetrics& m = rMyOutput.myFileMetrics[fileEntiry->getName()];

      // Calculate file LOC/LLOC.
      m.LOC = lineEnd - lineBegin + 1;
      m.LLOC = rMyGMD.calculateLLOC(fileEntiry->getName(), lineBegin, lineEnd);

      // Load McCC from the map if there's an entry. Otherwise leave it at 1.
      m.McCC = 1;
      auto mcccit = myMcCCByFiles.find(fid);
      if (mcccit != myMcCCByFiles.end())
        m.McCC = mcccit->second + 1;

      // Aggregate files into TU metrics.
      tum.LOC  += m.LOC;
      tum.LLOC += m.LLOC;

      // Subtract 1 because we only add it once at the end of the aggregation (because of McCC "plus one" definition).
      tum.McCC += m.McCC - 1;
    }

    // Add 1 to the McCC of the TU, because of the "plus one" definition. Then merge.
    tum.McCC += 1;
  }
    
  // Debug print Halstead metrics if requested.
  if (myDebugPrintAfterVisit)
  {
    std::cout << " --- HALSTEAD RESULTS BEGIN --- \n\n  Translation unit: " << myCurrentTU << "\n\n\n";
    for (auto& hs : myFunctionMetrics)
    {
      HalsteadStorage& storage = hs.second.hsStorage;

      if (ObjCMethodDecl::classofKind(hs.first->getDeclKind()))
      {
        std::cout << "  Function: " << cast<ObjCMethodDecl>(hs.first)->getNameAsString() << '\n';
      }
      else if (FunctionDecl::classofKind(hs.first->getDeclKind()))
      {
        std::cout << "  Function: " << cast<FunctionDecl>(hs.first)->getNameAsString() << '\n';
      }

      std::cout << "  "; storage.dbgPrintOperators();
      std::cout << "  "; storage.dbgPrintOperands();
      std::cout << "\n  \tOperators: " << storage.getOperatorCount() << "\tD: " << storage.getDistinctOperatorCount();
      std::cout << "\n  \tOperands:  " << storage.getOperandCount() << "\tD: " << storage.getDistinctOperandCount() << "\n\n\n";
    }
    std::cout << " --- HALSTEAD RESULTS END --- \n\n";
  }
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
    oper = Range::NO_OP;

    if (auto d = dyn_cast<CXXMethodDecl>(decl))
    {
      const Range* parentRange = getDefinition(factory.create(d->getParent()));
      if (parentRange && parentRange->type == Range::DEFINITION)
      {
        if (!containsRange(*parentRange, createTemporaryRange(d)))
          oper = Range::LOC_MERGE;

        parent = parentRange;
      }
    }
  }
  else if (auto d = dyn_cast<CXXRecordDecl>(decl))
  {
    type = (d->getDefinition() == d ? Range::DEFINITION : Range::DECLARATION);
    kind = isInterface(d) ? Object::INTERFACE : Object::CLASS;
    oper = Range::NO_OP;

    const DeclContext* pn = d->getParent();
    if (pn)
    {
      if (isa<CXXRecordDecl>(pn) || isa<FunctionDecl>(pn))
      {
        oper = Range::LOC_SUBTRACT;

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
  else if (auto d = dyn_cast<ObjCProtocolDecl>(decl))
  {
    oper = Range::NO_OP;
    kind = Object::INTERFACE;
    type = (d->isThisDeclarationADefinition() ? Range::DEFINITION : Range::DECLARATION);
    addNamespaceRange(d->getParent());
  }
  else if(auto d = dyn_cast<ObjCCategoryDecl>(decl))
  {
    oper = Range::NO_OP;
    kind = d->IsClassExtension() ? Object::INTERFACE : Object::CLASS;
    type = Range::DEFINITION;
    addNamespaceRange(d->getParent());
  }
  else if(auto d = dyn_cast<ObjCInterfaceDecl>(decl))
  {
    oper = Range::NO_OP;
    kind = Object::CLASS;
    type = (d->isThisDeclarationADefinition() ? Range::DEFINITION : Range::DECLARATION);
    addNamespaceRange(d->getParent());
  }
  else if(auto d = dyn_cast<ObjCCategoryImplDecl>(decl))
  {
    parent = getDefinition(factory.create(dyn_cast<Decl>(d->getCategoryDecl())));
    createRange(Range::DEFINITION, decl->getSourceRange(), parent, Range::LOC_MERGE);
    return;
  }
  else if(auto d = dyn_cast<ObjCImplementationDecl>(decl))
  {
    parent = getDefinition(factory.create(dyn_cast<Decl>(d->getClassInterface())));
    createRange(Range::DEFINITION, decl->getSourceRange(), parent, Range::LOC_MERGE);
    return;
  }
  else if(auto d = dyn_cast<ObjCMethodDecl>(decl))
  {
    type = (d->isThisDeclarationADefinition() ? Range::DEFINITION : Range::DECLARATION);
    kind = Object::FUNCTION;
    oper = Range::NO_OP;
    
    const DeclContext* parentContext = d->getParent();
  
    if(const ObjCCategoryImplDecl* impl = dyn_cast<ObjCCategoryImplDecl>(parentContext))
    {
      parentContext = impl->getCategoryDecl();
    }
    else if(const ObjCImplementationDecl* impl = dyn_cast<ObjCImplementationDecl>(parentContext))
    {
      parentContext = impl->getClassInterface();
    }
    
    const Range* parentRange = getDefinition(factory.create(dyn_cast<Decl>(parentContext)));
    if (parentRange && parentRange->type == Range::DEFINITION)
    {
      parent = parentRange;
    }
  }
  else if (auto d = dyn_cast<EnumDecl>(decl))
  {
    type = (d->getDefinition() == d ? Range::DEFINITION : Range::DECLARATION);
    kind = Object::ENUM;
    oper = Range::NO_OP;

    const DeclContext* pn = d->getParent();
    if (pn)
    {
      if (isa<NamespaceDecl>(pn))
      {
        addNamespaceRange(pn);
      }
    }
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
  auto objit = myObjects.emplace(Object{ factory.create(decl), kind }, std::set<const Range*, RangePtrComparator>{}).first;
  objit->second.insert(&range);
  myRangeMap[&range] = &objit->first;
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

const GlobalMergeData::Range* clang::metrics::detail::GlobalMergeData::getParentRange(SourceLocation loc)
{
  if (myRanges.empty())
    return nullptr;

  assert(pMyAnalyzer && "Pointer to ClangMetrics should already be set at this point.");
  SourceManager& sm = pMyAnalyzer->getASTContext()->getSourceManager();

  unsigned file   = fileid(sm.getFilename(loc));
  unsigned line   = sm.getExpansionLineNumber(loc);
  unsigned column = sm.getExpansionColumnNumber(loc);

  auto it = myRanges.upper_bound(Range{ nullptr, file, line, column });
  if (it == myRanges.begin())
    return nullptr;

  return &*(--it);
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

    info.LOC   = range.lineEnd - range.lineBegin + 1;
    info.LLOC  = calculateLLOC(range.fileID, range.lineBegin, range.lineEnd);

    info.TLOC  = info.LOC;
    info.TLLOC = info.LLOC;

    if (const Range* parent = range.parent)
    {
      LOCInfo& parentInfo = locmap[parent];
      if (range.operation == Range::LOC_MERGE)
      {
        parentInfo.LOC   += info.TLOC;
        parentInfo.LLOC  += info.TLLOC;
        parentInfo.TLOC  += info.TLOC;
        parentInfo.TLLOC += info.TLLOC;
      }
      else if (range.operation == Range::LOC_SUBTRACT)
      {
        parentInfo.LOC  -= info.LOC;
        parentInfo.LLOC -= info.LLOC;
      }
    }
  }

  std::unordered_map<const Range*, const Object*> objmap;
  for (auto& object : myObjects)
  {
    if (object.first.kind == Object::FUNCTION)
    {
      FunctionMetrics& m = output.myFunctionMetrics[object.first.uid];

      const Range* range = getDefinition(object.first.uid);
      if (range)
      {
        if (range->parent)
        {
          auto itp = myRangeMap.find(range->parent);
          assert(itp != myRangeMap.end() && "All ranges should be added to the range map.");
          if (itp->second->kind == Object::CLASS || itp->second->kind == Object::INTERFACE)
          {
            ++output.myClassMetrics[itp->second->uid].NLM;
          }
        }

        auto it = locmap.find(range);
        if (it != locmap.end())
        {
          m.name  = object.first.uid->getName();
          m.LOC   = it->second.LOC;
          m.LLOC  = it->second.LLOC;
          m.TLOC  = it->second.TLOC;
          m.TLLOC = it->second.TLLOC;
        }
      }
    }
    else if (object.first.kind == Object::CLASS || object.first.kind == Object::INTERFACE)
    {
      ClassMetrics& m = output.myClassMetrics[object.first.uid];

      const Range* range = getDefinition(object.first.uid);
      if (range)
      {
        if (range->parent)
        {
          auto itp = myRangeMap.find(range->parent);
          assert(itp != myRangeMap.end() && "All ranges should be added to the range map.");
          if (itp->second->kind == Object::NAMESPACE)
          {
            if (object.first.kind == Object::CLASS)
              ++output.myNamespaceMetrics[itp->second->uid].NCL;
            else /* if INTERFACE */
              ++output.myNamespaceMetrics[itp->second->uid].NIN;
          }
        }

        auto it = locmap.find(range);
        if (it != locmap.end())
        {
          m.name  = object.first.uid->getName();
          m.LOC   = it->second.LOC;
          m.LLOC  = it->second.LLOC;
          m.TLOC  = it->second.TLOC;
          m.TLLOC = it->second.TLLOC;
        }
      }
    }
    else if (object.first.kind == Object::ENUM)
    {
      EnumMetrics& m = output.myEnumMetrics[object.first.uid];

      const Range* range = getDefinition(object.first.uid);
      if (range)
      {
        if (range->parent)
        {
          auto itp = myRangeMap.find(range->parent);
          assert(itp != myRangeMap.end() && "All ranges should be added to the range map.");
          if (itp->second->kind == Object::NAMESPACE)
          {
            ++output.myNamespaceMetrics[itp->second->uid].NEN;
          }
        }

        auto it = locmap.find(range);
        if (it != locmap.end())
        {
          m.name = object.first.uid->getName();
          m.LOC  = it->second.LOC;
          m.LLOC = it->second.LLOC;
        }
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
          m.name   = object.first.uid->getName();
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

    os << " NOS: " << range.numberOfStatements << '\n';
  };

  for (auto& object : myObjects)
  {
    switch (object.first.kind)
    {
    case Object::FUNCTION:  os << "FUNCTION: ";  break;
    case Object::CLASS:     os << "CLASS: ";     break;
    case Object::INTERFACE: os << "INTERFACE: ";     break;
    case Object::ENUM:      os << "ENUM: ";      break;
    case Object::NAMESPACE: os << "NAMESPACE: "; break;
    }

    std::string debugName = object.first.uid->getName();
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
  
  range.numberOfStatements = 0;

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

unsigned GlobalMergeData::calculateLLOC(unsigned fileID, unsigned lineBegin, unsigned lineEnd) const
{
  auto beg = myCodeLines.lower_bound({ fileID, lineBegin });
  auto end = myCodeLines.upper_bound({ fileID, lineEnd });
  return std::distance(beg, end);
}
