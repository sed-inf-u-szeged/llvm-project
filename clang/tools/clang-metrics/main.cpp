// Includes
#include <clang/Metrics/BasicUID.h>
#include <clang/Metrics/Output.h>
#include <clang/Metrics/invoke.h>

#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/Support/CommandLine.h>

#include <utility>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>


using namespace std;
using namespace clang::metrics;

const char sep = ',';


void printFunctions(const Output& output)
{
  typedef pair<shared_ptr<const UID>, FunctionMetrics> record_t;

  vector<record_t> functions(output.function_begin(), output.function_end());
  if (functions.empty())
  {
    cout << "  No function metrics recorded - skipping...\n";
    return;
  }

  sort(functions.begin(), functions.end(), [](const record_t& lhs, const record_t& rhs)
  {
    return lhs.second.name < rhs.second.name;
  });

  std::ofstream out("Function-Metrics.csv");
  out << "Name,LOC,TLOC,LLOC,TLLOC,McCC,NOS,NL,NLE,HCPL,HDIF,HPL,HPV,HVOL,HEFF,HNDB,HTRP\n";

  for (auto it = functions.begin(); it != functions.end(); ++it)
  {
    const FunctionMetrics& m = it->second;

    out
      << m.name << sep
      << m.LOC << sep
      << m.TLOC << sep
      << m.LLOC << sep
      << m.TLLOC << sep
      << m.McCC << sep
      << m.NOS << sep
      << m.NL << sep
      << m.NLE << sep
      << m.HCPL() << sep
      << m.HDIF() << sep
      << m.HPL() << sep
      << m.HPV() << sep
      << m.HVOL() << sep
      << m.HEFF() << sep
      << m.HNDB() << sep
      << m.HTRP() << endl;
  }

  cout << "  Function metrics written to 'Function-Metrics.csv'\n";
}

void printClasses(const Output& output)
{
  typedef pair<shared_ptr<const UID>, ClassMetrics> record_t;

  vector<record_t> classes(output.class_begin(), output.class_end());
  if (classes.empty())
  {
    cout << "  No class metrics recorded - skipping...\n";
    return;
  }

  sort(classes.begin(), classes.end(), [](const record_t& lhs, const record_t& rhs)
  {
    return lhs.second.name < rhs.second.name;
  });

  std::ofstream out("Class-Metrics.csv");
  out << "Name,LOC,TLOC,LLOC,TLLOC,NLM\n";

  for (auto it = classes.begin(); it != classes.end(); ++it)
  {
    const ClassMetrics& m = it->second;

    out
      << m.name << sep
      << m.LOC << sep
      << m.TLOC << sep
      << m.LLOC << sep
      << m.TLLOC << sep
      << m.NLM << endl;
  }

  cout << "  Class metrics written to 'Class-Metrics.csv'\n";
}

void printEnums(const Output& output)
{
  typedef pair<shared_ptr<const UID>, EnumMetrics> record_t;

  vector<record_t> enums(output.enum_begin(), output.enum_end());
  if (enums.empty())
  {
    cout << "  No enum metrics recorded - skipping...\n";
    return;
  }

  sort(enums.begin(), enums.end(), [](const record_t& lhs, const record_t& rhs)
  {
    return lhs.second.name < rhs.second.name;
  });

  std::ofstream out("Enum-Metrics.csv");
  out << "Name,LOC,LLOC\n";

  for (auto it = enums.begin(); it != enums.end(); ++it)
  {
    const EnumMetrics& m = it->second;

    out
      << m.name << sep
      << m.LOC << sep
      << m.LLOC << endl;
  }

  cout << "  Enum metrics written to 'Enum-Metrics.csv'\n";
}

void printNamespaces(const Output& output)
{
  typedef pair<shared_ptr<const UID>, NamespaceMetrics> record_t;

  vector<record_t> namespaces(output.namespace_begin(), output.namespace_end());
  if (namespaces.empty())
  {
    cout << "  No namespace metrics recorded - skipping...\n";
    return;
  }

  sort(namespaces.begin(), namespaces.end(), [](const record_t& lhs, const record_t& rhs)
  {
    return lhs.second.name < rhs.second.name;
  });

  std::ofstream out("Namespace-Metrics.csv");
  out << "Name,LOC,TLOC,LLOC,TLLOC,NCL,NEN,NIN\n";

  for (auto it = namespaces.begin(); it != namespaces.end(); ++it)
  {
    const NamespaceMetrics& m = it->second;

    out
      << m.name << sep
      << m.totalMetrics.LOC << sep
      << m.totalMetrics.TLOC << sep
      << m.totalMetrics.LLOC << sep
      << m.totalMetrics.TLLOC << sep
      << m.totalMetrics.NCL << sep
      << m.totalMetrics.NEN << sep
      << m.totalMetrics.NIN << endl;
  }

  cout << "  Namespace metrics written to 'Namespace-Metrics.csv'\n";
}

void printFiles(const Output& output)
{
  typedef pair<string, FileMetrics> record_t;

  vector<record_t> files(output.file_begin(), output.file_end());
  if (files.empty())
  {
    cout << "  No file metrics recorded - skipping...\n";
    return;
  }

  sort(files.begin(), files.end(), [](const record_t& lhs, const record_t& rhs)
  {
    return lhs.first < rhs.first;
  });

  std::ofstream out("File-Metrics.csv");
  out << "Name,LOC,LLOC,McCC\n";

  for (const record_t& m : files)
  {
    out
      << m.first << sep
      << m.second.LOC << sep
      << m.second.LLOC << sep
      << m.second.McCC << endl;
  }

  cout << "  File metrics written to 'File-Metrics.csv'\n";
}

void printTUs(const Output& output)
{
  typedef pair<string, FileMetrics> record_t;

  vector<record_t> tus(output.tu_begin(), output.tu_end());
  if (tus.empty())
  {
    cout << "  No translation unit metrics recorded - skipping...\n";
    return;
  }

  sort(tus.begin(), tus.end(), [](const record_t& lhs, const record_t& rhs)
  {
    return lhs.first < rhs.first;
  });

  std::ofstream out("TU-Metrics.csv");
  out << "Name,LOC,LLOC,McCC\n";

  for (const record_t& m : tus)
  {
    out
      << m.first << sep
      << m.second.LOC << sep
      << m.second.LLOC << sep
      << m.second.McCC << endl;
  }

  cout << "  Translation unit metrics written to 'TU-Metrics.csv'\n";
}

int main(int argc, const char** argv)
{
  // Help message.
  const llvm::cl::extrahelp commonHelp(clang::tooling::CommonOptionsParser::HelpMessage);
  const llvm::cl::extrahelp moreHelp("\n\nExample usage: clang-metrics -extra-arg=\"-fno-delayed-template-parsing\" file1.cpp file2.cpp --\n");

  // Command line parameters.
  llvm::cl::OptionCategory optCat("clang-metrics options");
  llvm::cl::opt<bool> optHalsteadDebugPrint("hs-debug-print", llvm::cl::desc("Print Halstead metrics calculation related debug information."), llvm::cl::cat(optCat));
  llvm::cl::opt<bool> optRangeDebugPrint("range-debug-print", llvm::cl::desc("Print ranges used for LOC calculation."), llvm::cl::cat(optCat));

  // Parse command line arguments.
  clang::tooling::CommonOptionsParser parser(argc, argv, optCat);

  BasicUIDFactory factory;
  Output output(factory);

  InvokeOptions options;
  options.enableHalsteadDebugPrint = optHalsteadDebugPrint;
  options.enableRangeDebugPrint    = optRangeDebugPrint;

  cout << "Calculating code metrics...\n";
  bool ret = invoke(output, parser.getCompilations(), parser.getSourcePathList(), options);
  if (!ret)
  {
    cerr << "clang-metrics: Execution FAILED!\n";
    return EXIT_FAILURE;
  }

  printFunctions(output);
  printClasses(output);
  printEnums(output);
  printNamespaces(output);
  printFiles(output);
  printTUs(output);

  cout << "\nExecution finished.\n\n";

  return EXIT_SUCCESS;
}
