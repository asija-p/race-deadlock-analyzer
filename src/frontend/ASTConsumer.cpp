#include "ASTConsumer.h"
#include "CallVisitor.h"
#include "CFGPrinter.h"
#include "../analysis/LockOrderAnalyzer.h"

void DumpASTConsumer::HandleTranslationUnit(ASTContext &Context) {
    SourceManager &SM = Context.getSourceManager();
    TranslationUnitDecl *TU = Context.getTranslationUnitDecl();

    CallFinderVisitor Visitor(SM);

    for (Decl *D : TU->decls()) {
        if (!SM.isInMainFile(D->getLocation())) {
            continue;
        }

        if (auto *FD = dyn_cast<FunctionDecl>(D)) {
            if (!FD->hasBody()) {
                continue;
            }
            std::cout << "\n--- Funkcija: " << FD->getNameAsString() << " ---\n";
            Visitor.TraverseDecl(FD);
            PrintCFGForFunction(FD, Context);

            std::vector<LockPair> Pairs = FindLockOrderPairs(FD, Context);
            std::cout << "\n--- Parovi zakljucavanja ---\n";
            for (const LockPair &P : Pairs) {
                std::cout << P.first << " zakljucan pre " << P.second << "\n";
            }
        }
    }
}

std::unique_ptr<ASTConsumer> DumpASTAction::CreateASTConsumer(
    CompilerInstance &CI, StringRef file) {
    return std::make_unique<DumpASTConsumer>();
}