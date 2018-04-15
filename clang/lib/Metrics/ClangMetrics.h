#ifndef LLVM_CLANG_METRICS_CLANGMETRICS_H
#define LLVM_CLANG_METRICS_CLANGMETRICS_H


#include "Halstead.h"

#include <clang/AST/Decl.h>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <memory>
#include <string>

namespace clang {

class ASTContext;

namespace metrics {

class Output;

namespace detail {

class ClangMetrics {
  private:
    struct Comparator
    {
      bool operator()(std::tuple<clang::FileID, unsigned, unsigned> a, std::tuple<clang::FileID, unsigned, unsigned> b) const
      {
        if (std::get<0>(a) < std::get<0>(b))
          return true;

        if (std::get<1>(a) < std::get<1>(b))
          return true;

        if (std::get<1>(a) > std::get<1>(b))
          return false;

        return std::get<2>(a) < std::get<2>(b);
      }
    };

  public:
    //! Constructor.
    //!  \param output reference to the Output object where the results will be stored
    //!  \param context the AST context
    ClangMetrics(Output& output, ASTContext& context) : rMyOutput(output), currentFile(""), pMyASTContext(&context)
    {}

    //! Constructor.
    //!  \param output reference to the Output object where the results will be stored
    ClangMetrics(Output& output) : rMyOutput(output), currentFile(""), pMyASTContext(nullptr)
    {}

    //! If set to true, debug information will be printed to the standard output after
    //! each source operation. Default value is false.
    void debugPrintAfterVisit(bool value) noexcept
    {
      myDebugPrintAfterVisit = value;
    }

    // Aggregate the the metrics and merge the results into the output.
    void aggregateMetrics();

    // Update the current AST context.
    void updateASTContext(ASTContext& context)
    {
      pMyASTContext = &context;
    }

    // Update the current compilation unit file
    void updateCurrentFile(StringRef currentFile)
    {
      this->currentFile = currentFile;
    }

    // Returns a pointer to the actual AST context
    ASTContext* getASTContext() const
    {
      return pMyASTContext;
    }

    // Class implementing Clang's RecursiveASTVisitor pattern. Defines callbacks to the AST.
    // See the Clang documentation for more info.
    class NodeVisitor;

  protected:
    // Stores the output.
    Output& rMyOutput;

    // The current file.
    std::string currentFile;

  private:
    // The AST context.
    ASTContext* pMyASTContext;

    // Print debug info?
    bool myDebugPrintAfterVisit = false;

    // Contains all the classes in the program.
    std::unordered_set<const clang::Decl*> myClasses;

    // Contains all the methods and functions in the program.
    std::unordered_set<const clang::DeclContext*> myFunctions;

    // Contains all the enums (including C++ 11 strongly typed enums) in the program.
    std::unordered_set<const clang::EnumDecl*> myEnums;

    // Contains all the namespaces in the program.
    std::unordered_set<const clang::NamespaceDecl*> myNamespaces;

    // Maps classes/structs/unions defined within functions to the functions.
    // If a function has no "inside" classes/structs/unions, it won't be found in this map.
    std::unordered_map<const clang::DeclContext*, std::unordered_set<const clang::Decl*>> myInsideClassesByFunctions;

    // Maps McCC to functions. If a function has an McCC of 1, it won't be found in this map.
    // Note that the values stored here are one less than the final McCC.
    std::unordered_map<const clang::DeclContext*, unsigned> myMcCCByFunctions;

    // Halstead operators (first) and operands (second) by functions.
    std::unordered_map<const clang::DeclContext*, HalsteadStorage> myHalsteadByFunctions;

    // Maps inner classes to their "outside" class.
    // If the class has no inner classes, it won't be found in this map.
    std::unordered_map<const clang::Decl*, std::unordered_set<const clang::Decl*>> myInnerClassesByClasses;

    // Maps ObjC Interfaces by Implementation.
    std::unordered_map<const clang::ObjCContainerDecl*, const clang::ObjCContainerDecl*> myObjCInterfaceByImplementations;

    // Maps methods to their class.
    // If the class has no methods, it won't be found in this map.
    std::unordered_map<const clang::Decl*, std::unordered_set<const clang::Decl*>> myMethodsByClasses;

    // Maps namespaces inside another namespace to this "outside" namespace.
    // If the namespace has no inner namespaces, it won't be found in this map.
    std::unordered_map<const clang::NamespaceDecl*, std::unordered_set<const clang::NamespaceDecl*>> myInnerNamespacesByNamespaces;

    // Maps classes/structs/unions to their namespace. If a class/struct/union is in the global namespace, it won't be found in this map.

    std::unordered_map<const clang::NamespaceDecl*, std::unordered_set<const clang::Decl*>> myClassesByNamespaces;

    // Maps enums (including C++ 11 strongly typed enums) to their namespace. If an enum is in the global namespace, it won't be found in this map.
    std::unordered_map<const clang::NamespaceDecl*, std::unordered_set<const clang::EnumDecl*>> myEnumsByNamespaces;

    // Stores line numbers where there is sure to be code.
    std::set<std::pair<clang::FileID, unsigned>> myCodeLines;

    // Stores locations where there are semicolons. A single record is a pair of row/column within the file.
    std::set<std::tuple<clang::FileID, unsigned, unsigned>, Comparator> mySemicolonLocations;

    // McCC for the whole file. (starting from 1 becaus of McCC "plus 1" definition)
    unsigned myFileMcCC = 1;

};

} // namespace detail
} // namespace metrics
} // namespace clang



#endif
