#include "CFGPrinter.h"
#include <clang/Analysis/CFG.h>
#include <iostream>

void PrintCFGForFunction(FunctionDecl *FD, ASTContext &Context) {
    if (!FD->hasBody()) {
        return;
    }

    std::cout << "\n=== CFG za funkciju: " << FD->getNameAsString() << " ===\n";

    std::unique_ptr<CFG> Cfg = CFG::buildCFG(
        FD, FD->getBody(), &Context, CFG::BuildOptions());

    if (!Cfg) {
        std::cout << "Nije uspelo pravljenje CFG-a.\n";
        return;
    }

    Cfg->print(llvm::outs(), LangOptions(), /*ShowColors=*/false);
}