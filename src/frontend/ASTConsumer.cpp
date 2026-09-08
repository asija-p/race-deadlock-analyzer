#include "ASTConsumer.h"
#include "CallVisitor.h"
#include "CFGPrinter.h"
#include "../analysis/deadlock/LockOrderAnalyzer.h"
#include "../analysis/deadlock/CycleDetector.h"
#include "../analysis/race/RaceAnalyzer.h"
#include "../analysis/race/RaceDetector.h"

bool QuietMode = false;
bool JsonMode = false;

// Ispisuje pun, citljiv izvestaj (CFG, lock-order parove, MemoryAccess
// zapise, i finalne rezultate deadlock/race analize) - za rucno pregledanje.
static void PrintFullReport(const std::vector<LockPair> &AllPairs,
                             const std::set<std::string> &CreatedInLoop,
                             const std::vector<MemoryAccess> &AllAccesses,
                             const std::vector<std::vector<LockPair>> &Cycles) {
    std::cout << "\n=== SVI parovi zakljucavanja (iz svih funkcija) ===\n";
    for (const LockPair &P : AllPairs) {
        std::cout << P.From << " -> " << P.To << "  | Must={";
        bool first = true;
        for (const auto &L : P.MustContextLocks) {
            if (!first) std::cout << ",";
            std::cout << L;
            first = false;
        }
        std::cout << "}  | ThreadId=" << P.ThreadId << "  | JoinedThreads={";
        bool firstJ = true;
        for (const auto &J : P.JoinedThreads) {
            if (!firstJ) std::cout << ",";
            std::cout << J;
            firstJ = false;
        }
        std::cout << "}  | CreatedInLoop=" << (P.CreatedInLoop ? "true" : "false") << "\n";
    }

    std::cout << "\n=== CreatedInLoop (ThreadId-jevi kreirani unutar petlje) ===\n";
    if (CreatedInLoop.empty()) {
        std::cout << "(prazno)\n";
    } else {
        for (const auto &T : CreatedInLoop) {
            std::cout << T << "\n";
        }
    }

    std::cout << "\n=== SVI pristupi promenljivama (MemoryAccess) ===\n";
    for (const MemoryAccess &A : AllAccesses) {
        std::cout << A.VarName << " " << (A.IsWrite ? "WRITE" : "READ")
                   << "  | ThreadId=" << A.ThreadId
                   << "  | Line=" << A.Line
                   << "  | MustLockset={";
        bool firstL = true;
        for (const auto &L : A.MustLockset) {
            if (!firstL) std::cout << ",";
            std::cout << L.first;
            firstL = false;
        }
        std::cout << "}  | MayLockset={";
        bool firstML = true;
        for (const auto &L : A.MayLockset) {
            if (!firstML) std::cout << ",";
            std::cout << L.first;
            firstML = false;
        }
        std::cout << "}\n";
    }

    auto Races = FindRaces(AllAccesses);
    std::cout << "\n=== PRONADJENI RACE PAROVI ===\n";
    if (Races.empty()) {
        std::cout << "(nijedan)\n";
    } else {
        for (const auto &Report : Races) {
            std::cout << (Report.Severity == RaceSeverity::MustRace ? "[MUST-RACE] " : "[MAY-RACE]  ")
                       << Report.A.VarName
                       << "  [" << Report.A.ThreadId << " Linija " << Report.A.Line << "]"
                       << " <-> "
                       << "[" << Report.B.ThreadId << " Linija " << Report.B.Line << "]\n";
        }
    }

    if (!Cycles.empty()) {
        std::cout << "UPOZORENJE: Moguci deadlock!\n";
        std::set<std::string> SeenSignatures;
        for (const auto &Cycle : Cycles) {
            std::string Signature;
            for (size_t i = 0; i < Cycle.size(); i++) {
                Signature += Cycle[i].From + "->";
            }
            Signature += Cycle.back().To;

            if (SeenSignatures.count(Signature)) {
                continue;
            }
            SeenSignatures.insert(Signature);

            for (size_t i = 0; i < Cycle.size(); i++) {
                std::cout << Cycle[i].From;
                if (i + 1 < Cycle.size()) std::cout << " -> ";
            }
            std::cout << " -> " << Cycle.back().To << "\n";
        }
    } else {
        std::cout << "Nije pronadjen deadlock rizik.\n";
    }
}

// Ispisuje mašinski citljiv, kompaktan format - za automatsko testiranje
// (test_runner.cpp parsira ovaj izlaz).
static void PrintQuietReport(const std::vector<MemoryAccess> &AllAccesses,
                              const std::vector<std::vector<LockPair>> &Cycles) {
    auto Races = FindRaces(AllAccesses);
    if (!Races.empty()) {
        std::cout << "RACE\n";
        for (const auto &Report : Races) {
            std::cout << Report.A.VarName << "|"
                       << Report.A.Line << "|"
                       << Report.A.ThreadId << "|"
                       << (Report.Severity == RaceSeverity::MustRace ? "MUST" : "MAY") << "\n";
            std::cout << Report.B.VarName << "|"
                       << Report.B.Line << "|"
                       << Report.B.ThreadId << "|"
                       << (Report.Severity == RaceSeverity::MustRace ? "MUST" : "MAY") << "\n";
        }
    } else {
        std::cout << "NO_RACE\n";
    }

    if (!Cycles.empty()) {
        std::cout << "DEADLOCK\n";
        for (const auto &Cycle : Cycles) {
            for (size_t i = 0; i < Cycle.size(); i++) {
                if (i > 0) std::cout << ",";
                std::cout << Cycle[i].From << "->" << Cycle[i].To;
            }
            std::cout << "\n";
        }
    } else {
        std::cout << "SAFE\n";
    }
}

// Ispisuje rezultate u JSON formatu - za GUI (Qt aplikaciju) da lako parsira
// preko QJsonDocument, umesto da parsira nas tekstualni --quiet format.
static void PrintJsonReport(const std::vector<MemoryAccess> &AllAccesses,
                             const std::vector<std::vector<LockPair>> &Cycles) {
    auto Races = FindRaces(AllAccesses);

    std::cout << "{\n";

    std::cout << "  \"deadlocks\": [\n";
    for (size_t i = 0; i < Cycles.size(); i++) {
        std::cout << "    {\"cycle\": [";
        const auto &Cycle = Cycles[i];
        for (size_t j = 0; j < Cycle.size(); j++) {
            std::cout << "\"" << Cycle[j].From << "->" << Cycle[j].To << "\"";
            if (j + 1 < Cycle.size()) std::cout << ", ";
        }
        std::cout << "]}";
        if (i + 1 < Cycles.size()) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "  ],\n";

    std::cout << "  \"races\": [\n";
    for (size_t i = 0; i < Races.size(); i++) {
        const auto &Report = Races[i];
        const char *SeverityStr = (Report.Severity == RaceSeverity::MustRace) ? "MUST" : "MAY";
        std::cout << "    {\"var\": \"" << Report.A.VarName << "\", \"severity\": \"" << SeverityStr << "\", "
                   << "\"pair\": ["
                   << "{\"line\": " << Report.A.Line << ", \"thread\": \"" << Report.A.ThreadId << "\"}, "
                   << "{\"line\": " << Report.B.Line << ", \"thread\": \"" << Report.B.ThreadId << "\"}"
                   << "]}";
        if (i + 1 < Races.size()) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "  ]\n";

    std::cout << "}\n";
}

void DumpASTConsumer::HandleTranslationUnit(ASTContext &Context) {
    SourceManager &SM = Context.getSourceManager();
    TranslationUnitDecl *TU = Context.getTranslationUnitDecl();

    CallFinderVisitor Visitor(SM);
    std::vector<LockPair> AllPairs;
    std::set<std::string> CreatedInLoop;
    std::vector<MemoryAccess> AllAccesses;

    for (Decl *D : TU->decls()) {
        if (!SM.isInMainFile(D->getLocation())) {
            continue;
        }

        if (auto *FD = dyn_cast<FunctionDecl>(D)) {
            if (!FD->hasBody()) {
                continue;
            }

            if (!QuietMode && !JsonMode) {
                std::cout << "\n--- Funkcija: " << FD->getNameAsString() << " ---\n";
                Visitor.TraverseDecl(FD);
                PrintCFGForFunction(FD, Context);
            }

            if (FD->getNameAsString() == "main") {
                std::vector<LockPair> Pairs = FindLockOrderPairs(FD, Context, CreatedInLoop);
                for (const LockPair &P : Pairs) {
                    AllPairs.push_back(P);
                }

                std::set<std::string> RaceCreatedInLoop;
                std::vector<MemoryAccess> Accesses = FindMemoryAccesses(FD, Context, RaceCreatedInLoop);
                for (const MemoryAccess &A : Accesses) {
                    AllAccesses.push_back(A);
                }
            }
        }
    }

    
    auto Cycles = FindCycles(AllPairs);
    
    if (JsonMode) {
        PrintJsonReport(AllAccesses, Cycles);
    } else if (QuietMode) {
        PrintQuietReport(AllAccesses, Cycles);
    } else {
        PrintFullReport(AllPairs, CreatedInLoop, AllAccesses, Cycles);
    }

}

std::unique_ptr<ASTConsumer> DumpASTAction::CreateASTConsumer(
    CompilerInstance &CI, StringRef file) {
    return std::make_unique<DumpASTConsumer>();
}

