#include "frontend/ASTConsumer.h"
#include <clang/Tooling/Tooling.h>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace clang::tooling;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Upotreba: " << argv[0] << " [--quiet] <putanja_do_c_fajla>\n";
        return 1;
    }

    std::string filePath;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--quiet") {
            QuietMode = true;
        } else {
            filePath = arg;
        }
    }

    if (filePath.empty()) {
        std::cerr << "Nije naveden fajl.\n";
        return 1;
    }

    std::ifstream inFile(filePath);
    if (!inFile) {
        std::cerr << "Ne mogu da otvorim fajl: " << filePath << "\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string code = buffer.str();

    std::vector<std::string> args = {"-x", "c"};
    bool success = runToolOnCodeWithArgs(
        std::make_unique<DumpASTAction>(), code, args, filePath);

    return success ? 0 : 1;
}