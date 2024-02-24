#include <getopt.h>
#include <iostream>
#include <unistd.h>

#include "config.hpp"

void print_usage(char *argv[]) {
  auto binName = argv[0];
  std::cout << "USAGE: " << binName << " <OPTION>..." << std::endl;
  std::cout << std::endl;

  std::cout << "Options:" << std::endl;

  std::cout << "  --version     "
            << "\t\t"
            << "Print GINO version." << std::endl;

  std::cout << "  --prefix      "
            << "\t\t"
            << "Print the installation prefix." << std::endl;

  std::cout << "  --flags       "
            << "\t\t"
            << "Print the compilation options used to compile GINO."
            << std::endl;

  std::cout << "  --git-commit  "
            << "\t\t"
            << "Print the git commit hash used during compilation."
            << std::endl;

  std::cout << "  --git-origin  "
            << "\t\t"
            << "Print the git origin used during compilation." << std::endl;

  std::cout << "  --flags       "
            << "\t\t"
            << "Print the compilation options used to compile GINO."
            << std::endl;
  std::cout
      << "  --llvm-prefix "
      << "\t\t"
      << "Print the installation prefix of the specific LLVM used by GINO."
      << std::endl;

  std::cout << "  --llvm-version"
            << "\t\t"
            << "Print the version of the specific LLVM used by GINO."
            << std::endl;

  return;
}

int main(int argc, char *argv[]) {

  /*
   * Fetch the inputs
   */
  if (argc < 2) {
    print_usage(argv);
    return 0;
  }

  int opt;
  static struct option long_options[] = {{"version", 0, NULL, 'n'},
                                         {"prefix", 0, NULL, 'p'},
                                         {"flags", 0, NULL, 'c'},
                                         {"git-commit", 0, NULL, 'm'},
                                         {"git-origin", 0, NULL, 'r'},
                                         {"llvm-version", 0, NULL, 'l'},
                                         {"llvm-prefix", 0, NULL, 'd'},
                                         {"llvm-flags", 0, NULL, 'o'},
                                         {NULL, 0, NULL, 0}};
  while ((opt = getopt_long(argc, argv, "npcmrldo", long_options, NULL)) !=
         -1) {

    switch (opt) {

    case 'n':
      std::cout << GINO_VERSION << std::endl;
      break;

    case 'p':
      std::cout << GINO_INSTALL_PREFIX << std::endl;
      break;

    case 'c':
      std::cout << GINO_CXX_FLAGS << std::endl;
      break;

    case 'm':
      std::cout << GINO_GIT_COMMIT << std::endl;
      break;

    case 'r':
      std::cout << GINO_GIT_ORIGIN << std::endl;
      break;

    case 'l':
      std::cout << LLVM_VERSION << std::endl;
      break;

    case 'd':
      std::cout << LLVM_INSTALL_PREFIX << std::endl;
      break;

    case 'o':
      std::cout << LLVM_BUILD_TYPE << std::endl;
      break;

    default:
      print_usage(argv);
    }
  }

  return 0;
}
