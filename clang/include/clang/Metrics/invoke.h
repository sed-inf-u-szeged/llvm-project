#pragma once

#include <vector>
#include <string>


namespace clang
{
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
			//! If true, debug data will be printed to the standard output after each source operation.
			//! Default value is false.
			bool enableDebugPrint = false;
		};

		//! Invokes metrics calculation with the given arguments.
		//!  \param output reference to the Output object storing the results
		//!  \param compilations compilation database, can be queried by CommonOptionsParser::getCompilations() from command line arguments
		//!  \param sourcePathList list of source files, can be queried by CommonOptionsParser::getSourcePathList() from command line arguments
		//!  \param options additional options specific to the metrics library
		//!  \return true on success, false on failure
		bool invoke(Output& output, clang::tooling::CompilationDatabase& compilations, const clang::tooling::CommandLineArguments& sourcePathList, InvokeOptions options = InvokeOptions());
	}
}