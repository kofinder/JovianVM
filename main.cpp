/**
 * Eva LLVM executable.
 */

#include <fstream>
#include <iostream>
#include <string>

#include "./src/JovianVM.h"

void printHelp() {
  std::cout << "\nUsage: finder-vm [options]\n\n"
            << "Options:\n"
            << "    -e, --expression  Expression to parse\n"
            << "    -f, --file        File to parse\n\n";
}

int main(int argc, char const *argv[]) {
  if (argc != 3) {
    printHelp();
    return 0;
  }

  /**
   * Expression mode.
   */
  std::string mode = argv[1];

  /**
   * Program to execute.
   */
  std::string program;

  /**
   * Simple expression.
   */
  if (mode == "-e") {
    program = argv[2];
  }

  /**
   * Eva file.
   */
  else if (mode == "-f") {
    // Read the file:
    std::ifstream programFile(argv[2]);
    std::stringstream buffer;
    buffer << programFile.rdbuf() << "\n";

    // Program:
    program = buffer.str();
  }

  /**
   * Compiler instance.
   */
  JovianVM vm;

  /**
   * Generate LLVM IR.
   */
  vm.exec(program);

  return 0;
}