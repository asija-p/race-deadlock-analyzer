#include "LockOrderAnalyzer.h"
#include <clang/Analysis/CFG.h>
#include <iostream>
#include <set>
#include <map>

using namespace clang;

// Napred deklarisemo, jer se ProcessBlock i AnalyzeFunctionBody
// sada pozivaju medjusobno (ProcessBlock -> AnalyzeFunctionBody -> ProcessBlock -> ...)
static std::set<std::string> AnalyzeFunctionBody(
    const FunctionDecl *FD,
    ASTContext &Context,
    std::set<std::string> InitialLocks,
    std::vector<LockPair> &Result,
    std::set<const FunctionDecl*> &CallStack);

// Pomocna funkcija - obradjuje jedan blok i njegove naredbe,
// azurirajuci aktivne lockove i beleze parove.
// Vraca AZURIRAN skup aktivnih lockova (posle prolaska kroz ovaj blok).
static std::set<std::string> ProcessBlock(
    const CFGBlock *Block,
    std::set<std::string> ActiveLocks,   // KOPIJA, namerno (ne referenca!)
    std::vector<LockPair> &Result,
    ASTContext &Context,
    std::set<const FunctionDecl*> &CallStack) {

    for (const CFGElement &Elem : *Block) {
        auto CS = Elem.getAs<CFGStmt>();
        if (!CS) {
            continue;
        }
        const Stmt *S = CS->getStmt();
        auto *Call = dyn_cast<CallExpr>(S);
        if (!Call) {
            continue;
        }

        const FunctionDecl *Callee = Call->getDirectCallee();
        if (!Callee) {
            continue;
        }

        std::string FuncName = Callee->getNameAsString();

        if (FuncName == "pthread_mutex_lock" || FuncName == "pthread_mutex_unlock") {
            if (Call->getNumArgs() == 0) {
                continue;
            }
            // Izvlacimo ime mutexa (isti trik kao u CallVisitor)
            std::string MutexName = "?";
            const Expr *Arg = Call->getArg(0);
            if (auto *Unary = dyn_cast<UnaryOperator>(Arg->IgnoreParenImpCasts())) {
                Arg = Unary->getSubExpr();
            }
            if (auto *Ref = dyn_cast<DeclRefExpr>(Arg->IgnoreParenImpCasts())) {
                MutexName = Ref->getDecl()->getNameAsString();
            }

            if (FuncName == "pthread_mutex_lock") {
                for (const std::string &Prev : ActiveLocks) {
                    Result.push_back({Prev, MutexName});
                }
                ActiveLocks.insert(MutexName);
            } else {
                ActiveLocks.erase(MutexName);
            }
            continue;
        }

        // NOVO: poziv korisnicke funkcije (nije lock/unlock) - udji unutra
        const FunctionDecl *Definition = Callee->getDefinition();
        if (Definition && Definition->hasBody() && !CallStack.count(Definition)) {
            ActiveLocks = AnalyzeFunctionBody(
                Definition, Context, ActiveLocks, Result, CallStack);
        }
        // Ako nema tela (npr. bibliotecka funkcija) ili je rekurzija -
        // preskacemo, konzervativno pretpostavljamo da ne dira lockove
    }

    return ActiveLocks;
}

// Racuna fixpoint nad CFG-om, pocevsi od InitialLocks na ulazu.
// Vraca lockset na IZLAZU iz funkcije (lockset na ulazu u EXIT blok).
static std::set<std::string> ComputeLockPairs(
    const CFG &Cfg,
    std::set<std::string> InitialLocks,
    std::vector<LockPair> &Result,
    ASTContext &Context,
    std::set<const FunctionDecl*> &CallStack) {

    std::map<const CFGBlock*, std::set<std::string>> LocksetAtEntry;
    std::vector<const CFGBlock*> Worklist;

    const CFGBlock *Entry = &Cfg.getEntry();
    LocksetAtEntry[Entry] = InitialLocks;
    Worklist.push_back(Entry);

    while (!Worklist.empty()) {
        const CFGBlock *Block = Worklist.back();
        Worklist.pop_back();

        std::set<std::string> InSet = LocksetAtEntry[Block];
        std::set<std::string> OutSet = ProcessBlock(Block, InSet, Result, Context, CallStack);

        for (const CFGBlock::AdjacentBlock &Succ : Block->succs()) {
            if (!Succ.isReachable()) continue;
            const CFGBlock *SuccBlock = Succ.getReachableBlock();

            std::set<std::string> Merged = OutSet;
            auto It = LocksetAtEntry.find(SuccBlock);
            if (It != LocksetAtEntry.end()) {
                Merged.insert(It->second.begin(), It->second.end());
            }

            if (It == LocksetAtEntry.end() || Merged != It->second) {
                LocksetAtEntry[SuccBlock] = Merged;
                Worklist.push_back(SuccBlock);
            }
        }
    }

    // Vracamo lockset na ulazu u EXIT blok - to je "izlazno stanje" cele funkcije
    const CFGBlock *ExitBlock = &Cfg.getExit();
    auto It = LocksetAtEntry.find(ExitBlock);
    if (It != LocksetAtEntry.end()) {
        return It->second;
    }
    return InitialLocks; // fallback, ne bi trebalo da se desi
}

// Analizira telo funkcije FD pocevsi od InitialLocks, dodaje parove u Result,
// i vraca lockset koji vazi POSLE izvrsavanja cele funkcije (za pozivaoca).
static std::set<std::string> AnalyzeFunctionBody(
    const FunctionDecl *FD,
    ASTContext &Context,
    std::set<std::string> InitialLocks,
    std::vector<LockPair> &Result,
    std::set<const FunctionDecl*> &CallStack) {

    if (!FD->hasBody()) {
        return InitialLocks;
    }

    std::unique_ptr<CFG> Cfg = CFG::buildCFG(
        FD, FD->getBody(), &Context, CFG::BuildOptions());

    if (!Cfg) {
        return InitialLocks;
    }

    CallStack.insert(FD);
    std::set<std::string> OutLocks =
        ComputeLockPairs(*Cfg, InitialLocks, Result, Context, CallStack);
    CallStack.erase(FD);

    return OutLocks;
}

std::vector<LockPair> FindLockOrderPairs(FunctionDecl *FD, ASTContext &Context) {
    std::vector<LockPair> Result;
    std::set<const FunctionDecl*> CallStack;
    std::set<std::string> InitialLocks;

    AnalyzeFunctionBody(FD, Context, InitialLocks, Result, CallStack);

    return Result;
}