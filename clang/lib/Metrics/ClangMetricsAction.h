#pragma once

#include "Halstead.h"

#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include <utility>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <memory>


namespace clang
{
  namespace metrics
  {
    class Output;

    namespace detail
    {
      /*!
       *  FrontendAction which calculates basic code measurements.
       *  Serves as an entry point for each translation unit operation.
       */
      class ClangMetricsAction final : public clang::ASTFrontendAction
      {
      private:
        struct Comparator
        {
          bool operator()(std::pair<unsigned, unsigned> a, std::pair<unsigned, unsigned> b) const
          {
            if (a.first < b.first)
              return true;

            if (a.first > b.first)
              return false;

            return a.second < b.second;
          }
        };

      public:
        //! Constructor.
        //!  \param output reference to the Output object where the results will be stored
        ClangMetricsAction(Output& output) : rMyOutput(output)
        {}

        //! If set to true, debug information will be printed to the standard output after
        //! each source operation. Default value is false.
        void debugPrintAfterVisit(bool value) noexcept
        {
          myDebugPrintAfterVisit = value;
        }

      private:
        // Called by the libtooling library. Should not be called manually.
        std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& ci, clang::StringRef file) override;

        // Called by the libtooling library at the end of source operations. Should not be called manually.
        void EndSourceFileAction() override;

      private:
        // Inner class implementing Clang's RecursiveASTVisitor pattern. Defines callbacks to the AST.
        // See the Clang documentation for more info.
        class NodeVisitor;

        // ASTConsumer for the NodeVisitor.
        // See the Clang documentation for more info.
        class Consumer;

      private:
        // Stores the output.
        Output& rMyOutput;

        // Print debug info?
        bool myDebugPrintAfterVisit = false;

        // Contains all the classes in the program.
        std::unordered_set<const clang::CXXRecordDecl*> myClasses;

        // Contains all the methods and functions in the program.
        std::unordered_set<const clang::FunctionDecl*> myFunctions;

        // Contains all the enums (including C++ 11 strongly typed enums) in the program.
        std::unordered_set<const clang::EnumDecl*> myEnums;

        // Contains all the namespaces in the program.
        std::unordered_set<const clang::NamespaceDecl*> myNamespaces;

        // Maps classes/structs/unions defined within functions to the functions.
        // If a function has no "inside" classes/structs/unions, it won't be found in this map.
        std::unordered_map<const clang::FunctionDecl*, std::unordered_set<const clang::CXXRecordDecl*>> myInsideClassesByFunctions;

        // Maps McCC to functions. If a function has an McCC of 1, it won't be found in this map.
        // Note that the values stored here are one less than the final McCC.
        std::unordered_map<const clang::FunctionDecl*, unsigned> myMcCCByFunctions;

        // Halstead operators (first) and operands (second) by functions.
        std::unordered_map<const clang::FunctionDecl*, HalsteadStorage> myHalsteadByFunctions;

        // Maps inner classes to their "outside" class.
        // If the class has no inner classes, it won't be found in this map.
        std::unordered_map<const clang::CXXRecordDecl*, std::unordered_set<const clang::CXXRecordDecl*>> myInnerClassesByClasses;

        // Maps methods to their class.
        // If the class has no methods, it won't be found in this map.
        std::unordered_map<const clang::CXXRecordDecl*, std::unordered_set<const clang::CXXMethodDecl*>> myMethodsByClasses;

        // Maps namespaces inside another namespace to this "outside" namespace.
        // If the namespace has no inner namespaces, it won't be found in this map.
        std::unordered_map<const clang::NamespaceDecl*, std::unordered_set<const clang::NamespaceDecl*>> myInnerNamespacesByNamespaces;

        // Maps classes/structs/unions to their namespace. If a class/struct/union is in the global namespace, it won't be found in this map.
        std::unordered_map<const clang::NamespaceDecl*, std::unordered_set<const clang::CXXRecordDecl*>> myClassesByNamespaces;

        // Maps enums (including C++ 11 strongly typed enums) to their namespace. If an enum is in the global namespace, it won't be found in this map.
        std::unordered_map<const clang::NamespaceDecl*, std::unordered_set<const clang::EnumDecl*>> myEnumsByNamespaces;

        // Stores line numbers where there is sure to be code
        std::set<unsigned> myCodeLines;

        // Stores locations where there are semicolons. A single record is a pair of row/column within the file.
        std::set<std::pair<unsigned, unsigned>, Comparator> mySemicolonLocations;

        // McCC for the whole file. (starting from 1 becaus of McCC "plus 1" definition)
        int myFileMcCC = 1;
      };
    }
  }
}