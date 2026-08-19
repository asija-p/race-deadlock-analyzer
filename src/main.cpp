#include "frontend/ASTConsumer.h"
#include <clang/Tooling/Tooling.h>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace clang::tooling;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Upotreba: " << argv[0] << " <putanja_do_c_fajla>\n";
        return 1;
    }

    std::ifstream inFile(argv[1]);
    if (!inFile) {
        std::cerr << "Ne mogu da otvorim fajl: " << argv[1] << "\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string code = buffer.str();

    std::vector<std::string> args = {"-x", "c"};
    bool success = runToolOnCodeWithArgs(
        std::make_unique<DumpASTAction>(), code, args, argv[1]);

    return success ? 0 : 1;
}