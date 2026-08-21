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

    std::vector<std::vector<std::string>> Cycles = FindCycles(AllPairs);

    std::cout << "\n=== REZULTAT ANALIZE ===\n";
    if (Cycles.empty()) {
        std::cout << "Nije pronadjen deadlock rizik.\n";
    } else {
        std::cout << "UPOZORENJE: Moguci deadlock! Pronadjen ciklus u redosledu zakljucavanja:\n";
        for (const auto &Cycle : Cycles) {
            for (const std::string &Mutex : Cycle) {
                std::cout << Mutex << " ";
            }
            std::cout << "\n";
        }
    }
}

std::unique_ptr<ASTConsumer> DumpASTAction::CreateASTConsumer(
    CompilerInstance &CI, StringRef file) {
    return std::make_unique<DumpASTConsumer>();
}