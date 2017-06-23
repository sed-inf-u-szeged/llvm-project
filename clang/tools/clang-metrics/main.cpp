// Includes
#include <clang/Metrics/CAM_Id.h>
#include <clang/Metrics/Output.h>
#include <clang/Metrics/invoke.h>

#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/Support/CommandLine.h>

#include <iostream>


// TODO: Write help message.
const llvm::cl::extrahelp commonHelp(clang::tooling::CommonOptionsParser::HelpMessage);
const llvm::cl::extrahelp moreHelp("\n\nWrite additional help text here.\n");


int main(int argc, const char** argv)
{
	using namespace std;
	using namespace clang::metrics;

	// Parse command line arguments.
	llvm::cl::OptionCategory optCat("clang-metrics options");
	clang::tooling::CommonOptionsParser parser(argc, argv, optCat);

	CAMIDFactory factory;
	Output output(factory);

	bool ret = invoke(output, parser.getCompilations(), parser.getSourcePathList());
	cout << "\n\nExecution " << (ret ? "finished SUCCESSFULLY" : "FAILED") << ".\nPress ENTER to exit!\n\n";

	for (auto test : output.functions())
	{
		cout << test.second.name << endl;
		cout << "\tOperators: " << test.second.H_Operators << "\tD: " << test.second.HD_Operators << endl;
		cout << "\tOperatnds: " << test.second.H_Operands << "\tD: " << test.second.HD_Operands << endl;
	}

	cin.get();

	return ret;
}