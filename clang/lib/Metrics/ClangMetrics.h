#ifndef LLVM_CLANG_METRICS_CLANGMETRICS_H
#define LLVM_CLANG_METRICS_CLANGMETRICS_H


#include "Halstead.h"

#include <clang/Metrics/UID.h>
#include <clang/Metrics/Output.h>
#include <llvm/Support/FileSystem/UniqueID.h>

#include <clang/AST/Decl.h>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <memory>
#include <string>
#include <stack>
#include <mutex>

namespace clang {

class ASTContext;

namespace metrics {

class Output;
class UIDFactory;

namespace detail {

// Forward declare main class.
class ClangMetrics;

// Stores data that must be persisted across multiple files and TUs.
class GlobalMergeData
{
public:
  struct Range
  {
    // The direct parent of the range. Null if the parent is the global namespace.
    const Range* parent;

    // Values defining the range: ID of the file, starting and ending lines/columns.
    unsigned fileID;
    unsigned lineBegin, lineEnd;
    unsigned short columnBegin, columnEnd;

    // The type of the range.
    enum range_t : unsigned char
    {
      // This range corresponds to an ODR declaration.
      DECLARATION,

      // This range corresponds to an ODR definition.
      // For simplicity, namespaces are also considered definitions here.
      DEFINITION
    } type;

    // Operation to do when merging to parent.
    enum oper_t : unsigned char
    {
      // No operation will be performed.
      NO_OP,

      // The TLOC/TLLOC of this node will be added to the parent's LOC/LLOC/TLOC/TLLOC.
      LOC_MERGE,

      // The LOC/LLOC of this node will be subtracted from the parent's LOC/LLOC.
      LOC_SUBTRACT
    } operation;

    // The number of statements in this range. Incremented on the fly.
    mutable unsigned numberOfStatements;

  };

  struct Object
  {
    // The UID of the object.
    std::shared_ptr<UID> uid;

    // The kind of the declaration making up this range.
    enum kind_t
    {
      FUNCTION,
      CLASS,
      INTERFACE,
      ENUM,
      NAMESPACE
    } kind;
  };

private:
  struct RangeComparator
  {
    bool operator()(const Range& lhs, const Range& rhs) const
    {
      return
        std::tie(lhs.fileID, lhs.lineBegin, lhs.columnBegin, lhs.lineEnd, lhs.columnEnd)
        <
        std::tie(rhs.fileID, rhs.lineBegin, rhs.columnBegin, rhs.lineEnd, rhs.columnEnd);
    }
  };

  struct RangePtrComparator
  {
    bool operator()(const Range* lhs, const Range* rhs) const
    {
      return RangeComparator()(*lhs, *rhs);
    }
  };

  struct ObjectHasher
  {
    size_t operator()(const Object& obj) const
    {
      return UIDHasher()(obj.uid);
    }
  };

  struct ObjectEq
  {
    bool operator()(const Object& lhs, const Object& rhs) const
    {
      return UIDEq()(lhs.uid, rhs.uid);
    }
  };

  typedef std::unordered_map<std::string, unsigned> filemap_t;
  typedef std::unordered_map<unsigned, std::string> reverse_filemap_t;
  typedef std::set<std::pair<unsigned, unsigned>>   codelines_t;
  typedef std::set<Range, RangeComparator>          rangeset_t;

  typedef std::unordered_map<Object, std::set<const Range*, RangePtrComparator>, ObjectHasher, ObjectEq> objectmap_t;
  typedef std::unordered_map<const Range*, const Object*> rangemap_t;

public:
  GlobalMergeData(Output& output) : rMyOutput(output) {}

  // Adds a declaration, creating a UID and a range for it, and mapping the two togather.
  void addDecl(const clang::Decl* decl, ClangMetrics& currentAnalyzer);

  // Adds a SourceLocation where there is code.
  void addCodeLine(SourceLocation loc, ClangMetrics& currentAnalyzer);

  // Returns a pointer to the range containing the given location, if there is one.
  const Range* getParentRange(SourceLocation loc, ClangMetrics& currentAnalyzer);

  // Aggregates metrics into the output.
  // This is the final step of the calculation, called after all files have been processed.
  void aggregate() const;

  // Calculates LLOC between two lines.
  unsigned calculateLLOC(const std::string& filename, unsigned lineBegin, unsigned lineEnd)
  {
    return calculateLLOC(fileid(filename), lineBegin, lineEnd);
  }

  // Prints debug information for ranges.
  void debugPrintObjectRanges(std::ostream& os) const;

  // Mapping declarations to their ranges
  //std::unordered_map<const clang::CXXRecordDecl *, const GlobalMergeData::Range *> declToRangeMap;

  // Returns the file ID of the filename. Creates a new ID if there isn't one
  // already.
  unsigned fileid(const std::string &filename);

private:
  // Creates a new range and returns a reference to it.
  // Note that ranges that only differ in their type or parent from a range already added, will not be created.
  const Range& createRange(Range::range_t type, const std::string& filename,
    unsigned lineBegin, unsigned lineEnd, unsigned columnBegin, unsigned columnEnd, const Range* parent, Range::oper_t operation);

  // Creates a new range and returns a reference to it.
  // Note that ranges that only differ in their type or parent from a range already added, will not be created.
  const Range& createRange(Range::range_t type, SourceLocation start, SourceLocation end, const Range* parent, Range::oper_t operation, ClangMetrics& currentAnalyzer);

  // Creates a new range and returns a reference to it.
  // Note that ranges that only differ in their type or parent from a range already added, will not be created.
  const Range& createRange(Range::range_t type, SourceRange r, const Range* parent, Range::oper_t operation, ClangMetrics& currentAnalyzer);

  // Returns the range where the definition is, or the first declaration if there is no definition.
  // Returns nullptr if there exists neither definition nor declaration for the UID.
  const Range* getDefinition(std::shared_ptr<UID> uid) const;

  // Returns whether range outer contains range inner.
  static bool containsRange(const Range& outer, const Range& inner);

  // Calculates LLOC between two lines.
  unsigned calculateLLOC(unsigned fileID, unsigned lineBegin, unsigned lineEnd) const;

private:
  // Invalid range. Returned by createRange() when the input is not valid.
  static const Range INVALID_RANGE;

  // Value of the next ID to be assigned. Zero is reserved as an invalid ID.
  unsigned myNextFileID = 1;

public:
  // Stores the output.
  Output& rMyOutput;

  // Maps filenames to unique integer IDs. Must be public for linking.
  filemap_t myFileIDs;

  // Maps unique integer IDs to filenames. Must be public for linking.
  reverse_filemap_t reverseMyFileIDs;

  //All files traversed
  std::map<llvm::sys::fs::UniqueID, const clang::FileEntry*> files;

private:
  // Stores (file ID, code line) pairs in an ordered set.
  codelines_t myCodeLines;

  // Stores Range objects in an ordered set.
  // It is safe to take the address of the elements, as their location in memory does not change.
  rangeset_t myRanges;

  // Maps ranges to specific declarations, identified by their UID.
  objectmap_t myObjects;

  // Maps objects to the range of the object.
  rangemap_t myRangeMap;
};

class GlobalMergeData_ThreadSafe
{
  GlobalMergeData globalMergeData;
  std::recursive_mutex globalMergeDataMutex;

public:
  GlobalMergeData_ThreadSafe(Output& output) : globalMergeData(output) {}
  template <class Operation>
  auto call(Operation o) -> decltype(o(globalMergeData))
  {
    std::lock_guard<std::recursive_mutex> lock(globalMergeDataMutex);
    return o(globalMergeData);
  }
};

class ClangMetrics
{
private:
  struct FileIDHasher
  {
    size_t operator()(const FileID& id) const
    {
      return id.getHashValue();
    }
  };

public:
  //! Constructor.
  //!  \param output reference to the Output object where the results will be stored
  ClangMetrics(GlobalMergeData_ThreadSafe& data) :
    myCurrentTU(""),
    rMyGMD(data),
    pMyASTContext(nullptr),
    pMyMangleContext(nullptr),
    diagnosticsEngine(IntrusiveRefCntPtr<clang::DiagnosticIDs>(new DiagnosticIDs()),
      &*IntrusiveRefCntPtr<clang::DiagnosticOptions>(new DiagnosticOptions()))
  {
  }

  //! If set to true, debug information will be printed to the standard output after
  //! each source operation. Default value is false.
  void debugPrintHalsteadAfterVisit(bool value) noexcept
  {
    myDebugPrintAfterVisit = value;
  }

  // Aggregate the the metrics and merge the results into the output.
  void aggregateMetrics();

  // Update the current AST context.
  void updateASTContext(ASTContext& context)
  {
    pMyASTContext = &context;
    pMyMangleContext.reset(clang::ItaniumMangleContext::create(context, diagnosticsEngine));
  }

  // Update the current compilation unit file
  void updateCurrentTU(StringRef currentTU)
  {
    myCurrentTU = currentTU.str();
  }

  // Returns a pointer to the actual AST context
  ASTContext* getASTContext() const
  {
    return pMyASTContext;
  }

  // Returns a pointer to the actual AST context
  std::string getTU() const
  {
    return myCurrentTU;
  }

  // Returns a pointer to the actual AST context
  std::shared_ptr<MangleContext> getMangleContext() const
  {
    return pMyMangleContext;
  }

  // Class implementing Clang's RecursiveASTVisitor pattern. Defines callbacks to the AST.
  // See the Clang documentation for more info.
  class NodeVisitor;

protected:
  // The name of the file of the current translation unit.
  std::string myCurrentTU;

private:

  class NestingLevelCounter
  {
    public:
      NestingLevelCounter()
        : maxLevel (0)
      {
        currentLevel.push(0);
      }

      void changeLevel(bool increase)
      {
        if (increase)
        {
          unsigned& currentLevelValue = currentLevel.top();
          currentLevelValue += 1;
          if (currentLevelValue > maxLevel)
            maxLevel = currentLevelValue;
        }
        else
        {
          currentLevel.top() -= 1;
        }
      }

      void stackLevel(bool increase)
      {
        if (increase)
          currentLevel.push(currentLevel.top());
        else
          if (currentLevel.size() > 1)
            currentLevel.pop();
          // else TODO warning
      }

      unsigned getNestingLevel()
      {
        return maxLevel;
      }

    protected:
      std::stack<unsigned> currentLevel;
      unsigned maxLevel;
  };

  struct FunctionMetricsData
  {
    // McCabe's complexity for the function.
    unsigned McCC = 1;
    // Number of statements
    unsigned NOS = 0;

    // Nesting level
    NestingLevelCounter NL;

    // Nesting level else-if
    NestingLevelCounter NLE;

    // Storage for Halstead operators and operands.
    HalsteadStorage hsStorage;
  };

private:
  // Reference to the global state.
  GlobalMergeData_ThreadSafe& rMyGMD;

  // The AST context.
  ASTContext* pMyASTContext;

  // The Mangle context.
  std::shared_ptr<MangleContext> pMyMangleContext;

  // The diagnosticsEngine used to create the mangleContext
  clang::DiagnosticsEngine diagnosticsEngine;

  // Print debug info?
  bool myDebugPrintAfterVisit = false;

public:
  // This will make the clang-metrics print what is it exactly doing during analysis (which files its processing, etc)
  // This is set with the InvokeOptions
  bool shouldPrintTracingInfo = false;

private:
  // Contains function metrics calculated per TU.
  std::unordered_map<const clang::DeclContext*, FunctionMetricsData> myFunctionMetrics;

  // Stores locations where there are semicolons. A single record is a pair of row/column within the file.
  std::set<std::tuple<clang::FileID, unsigned, unsigned, unsigned, unsigned>> mySemicolonLocations;

  // McCC per file. If a file has an McCC of 1, it won't be found in this map.
  // Note that the values stored here are one less than the final McCC.
  std::unordered_map<FileID, unsigned, FileIDHasher> myMcCCByFiles;
};

} // namespace detail
} // namespace metrics
} // namespace clang



#endif
