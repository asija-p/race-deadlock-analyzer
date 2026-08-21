#include "LockOrderAnalyzer.h"
#include <clang/Analysis/CFG.h>
#include <iostream>
#include <set>

using namespace clang;

// Pomocna funkcija - obradjuje jedan blok i njegove naredbe,
// azurirajuci aktivne lockove i beleze parove.
// Vraca AZURIRAN skup aktivnih lockova (posle prolaska kroz ovaj blok).
static std::set<std::string> ProcessBlock(
    const CFGBlock *Block,
    std::set<std::string> ActiveLocks,   // KOPIJA, namerno (ne referenca!)
    std::vector<LockPair> &Result) {

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
        if (!Callee || Call->getNumArgs() == 0) {
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

        std::string FuncName = Callee->getNameAsString();

        if (FuncName == "pthread_mutex_lock") {
            for (const std::string &Prev : ActiveLocks) {
                Result.push_back({Prev, MutexName});
            }
            ActiveLocks.insert(MutexName);
        } else if (FuncName == "pthread_mutex_unlock") {
            ActiveLocks.erase(MutexName);
        }
    }

    return ActiveLocks;
}

static void TraverseCFG(
    const CFGBlock *Block,
    std::set<std::string> ActiveLocks,
    std::vector<LockPair> &Result,
    std::set<const CFGBlock*> &Visited) {

    // Ako smo vec bili ovde, ne ulazimo ponovo (sprecava beskonacnu petlju)
    if (Visited.count(Block)) {
        return;
    }
    Visited.insert(Block);

    // Obradi naredbe u OVOM bloku, dobij azurirano stanje lock-ova
    std::set<std::string> LocksAfterThisBlock = ProcessBlock(Block, ActiveLocks, Result);

    // Idi rekurzivno u SVAKI sledeci blok (Succs)
    for (const CFGBlock::AdjacentBlock &Succ : Block->succs()) {
        if (Succ.isReachable()) {
            TraverseCFG(Succ.getReachableBlock(), LocksAfterThisBlock, Result, Visited);
        }
    }
}

std::vector<LockPair> FindLockOrderPairs(FunctionDecl *FD, ASTContext &Context) {
    std::vector<LockPair> Result;

    if (!FD->hasBody()) {
        return Result;
    }

    std::unique_ptr<CFG> Cfg = CFG::buildCFG(
        FD, FD->getBody(), &Context, CFG::BuildOptions());

    if (!Cfg) {
        return Result;
    }

    const CFGBlock *Entry = &Cfg->getEntry();
    std::set<std::string> InitialLocks;
    std::set<const CFGBlock*> Visited;

    TraverseCFG(Entry, InitialLocks, Result, Visited);

    return Result;
}
