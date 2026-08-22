#include "ASTConsumer.h"
#include "CallVisitor.h"
#include "CFGPrinter.h"
#include "../analysis/LockOrderAnalyzer.h"
#include "../analysis/CycleDetector.h"

void DumpASTConsumer::HandleTranslationUnit(ASTContext &Context) {
    SourceManager &SM = Context.getSourceManager();
    TranslationUnitDecl *TU = Context.getTranslationUnitDecl();

    CallFinderVisitor Visitor(SM);
    std::vector<LockPair> AllPairs;

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
            for (const LockPair &P : Pairs) {
                AllPairs.push_back(P);
            }
        }
    }

    std::cout << "\n=== SVI parovi zakljucavanja (iz svih funkcija) ===\n";
    for (const LockPair &P : AllPairs) {
        std::cout << P.first << " -> " << P.second << "\n";
    }

    auto Cycles = FindCycles(AllPairs);
    if (!Cycles.empty()) {
        std::cout << "UPOZORENJE: Moguci deadlock!\n";
        for (const auto &Cycle : Cycles) {
            for (size_t i = 0; i < Cycle.size(); i++) {
                std::cout << Cycle[i];
                if (i + 1 < Cycle.size()) std::cout << " -> ";
            }
            std::cout << "\n";
        }
    } else {
        std::cout << "Nije pronadjen deadlock rizik.\n";
    }
}

std::unique_ptr<ASTConsumer> DumpASTAction::CreateASTConsumer(
    CompilerInstance &CI, StringRef file) {
    return std::make_unique<DumpASTConsumer>();
}