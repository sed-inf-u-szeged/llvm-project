#pragma once

#include <vector>
#include <string>


namespace clang
{
  class ASTContext;
  class Decl;
  class Stmt;

  namespace tooling
  {
    class CompilationDatabase;
    typedef std::vector<std::string> CommandLineArguments;
  }

  namespace metrics
  {
    class Output;

    //! List of possible options passed to invoke().
    struct InvokeOptions
    {
      //! If true, Halstead debug data will be printed to the standard output after each source operation.
      //! Default value is false.
      bool enableHalsteadDebugPrint = false;

      //! If true, range debug data (used for LOC calculation) will be printed to the standard output at the end of the calculation.
      //! Default value is false.
      bool enableRangeDebugPrint = false;

      //! If true, the currently processed .ast files are printed so that if the
      //! program crashes, we can see where it crashed exactly.
      bool enableProcessingTracePrint = false;
    };

    //! Invokes metrics calculation with the given arguments.
    //!  \param output reference to the Output object storing the results
    //!  \param compilations compilation database, can be queried by CommonOptionsParser::getCompilations() from command line arguments
    //!  \param sourcePathList list of source files, can be queried by CommonOptionsParser::getSourcePathList() from command line arguments
    //!  \param options additional options specific to the metrics library
    //!  \return true on success, false on failure
    bool invoke(Output& output, const clang::tooling::CompilationDatabase& compilations, const clang::tooling::CommandLineArguments& sourcePathList, InvokeOptions options = InvokeOptions());

    //! Invokes metrics calculation with the given arguments.
    //!  \param output reference to the Output object storing the results
    //!  \param context ASTContext containing all the AST related information
    //!  \param declarations list of declaration nodes needed to be traversed
    //!  \param statements list of statement nodes needed to be traversed
    //!  \param options additional options specific to the metrics library
    void invoke(Output& output, clang::ASTContext& context, const std::vector<clang::Decl*>& declarations, const std::vector<clang::Stmt*>& statements, InvokeOptions options = InvokeOptions());
  }
}
