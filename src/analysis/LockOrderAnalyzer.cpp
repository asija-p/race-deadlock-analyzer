#include "LockOrderAnalyzer.h"
#include <clang/Analysis/CFG.h>
#include <iostream>
#include <set>
#include <map>

using namespace clang;

static std::string ExtractVarName(const Expr *Arg) {
    Arg = Arg->IgnoreParenImpCasts();
    if (auto *Unary = dyn_cast<UnaryOperator>(Arg)) {
        Arg = Unary->getSubExpr()->IgnoreParenImpCasts();
    }

    // indeksiranje niza (npr. locks[0])
    if (auto *ArrSub = dyn_cast<ArraySubscriptExpr>(Arg)) {
        std::string ArrayName = "?";
        const Expr *Base = ArrSub->getBase()->IgnoreParenImpCasts();
        if (auto *BaseRef = dyn_cast<DeclRefExpr>(Base)) {
            ArrayName = BaseRef->getDecl()->getNameAsString();
        }

        const Expr *IndexExpr = ArrSub->getIdx()->IgnoreParenImpCasts();
        if (auto *IntLit = dyn_cast<IntegerLiteral>(IndexExpr)) {
            return ArrayName + "[" + std::to_string(IntLit->getValue().getSExtValue()) + "]";
        }
        return ArrayName + "[?]";
    }

    if (auto *Ref = dyn_cast<DeclRefExpr>(Arg)) {
        return Ref->getDecl()->getNameAsString();
    }
    return "?";
}

// Prevodi lokalno ime (npr. parametar "m") u stvarno ime (npr. "m1"),
// koristeci ParamMap. Ako ime nije u mapi, vraca ga nepromenjeno.
static std::string ResolveName(const std::string &Name,
                                 const std::map<std::string, std::string> &ParamMap) {
    auto It = ParamMap.find(Name);
    if (It != ParamMap.end()) {
        return It->second;
    }
    return Name;
}

static std::set<std::string> AnalyzeFunctionBody(
    const FunctionDecl *FD,
    ASTContext &Context,
    std::set<std::string> InitialLocks,
    std::vector<LockPair> &Result,
    std::set<const FunctionDecl*> &CallStack,
    const std::map<std::string, std::string> &ParamMap);

static std::set<std::string> ProcessBlock(
    const CFGBlock *Block,
    std::set<std::string> ActiveLocks,
    std::vector<LockPair> &Result,
    ASTContext &Context,
    std::set<const FunctionDecl*> &CallStack,
    const std::map<std::string, std::string> &ParamMap) {

    for (const CFGElement &Elem : *Block) {
        auto CS = Elem.getAs<CFGStmt>();
        if (!CS) continue;
        const Stmt *S = CS->getStmt();
        auto *Call = dyn_cast<CallExpr>(S);
        if (!Call) continue;

        const FunctionDecl *Callee = Call->getDirectCallee();
        if (!Callee) continue;

        std::string FuncName = Callee->getNameAsString();

        if (FuncName == "pthread_mutex_lock" || FuncName == "pthread_mutex_trylock" ||
            FuncName == "pthread_mutex_unlock") {
            if (Call->getNumArgs() == 0) continue;

            // NOVO: izvuci ime, pa ga prevedi kroz ParamMap ako treba
            std::string RawName = ExtractVarName(Call->getArg(0));
            std::string MutexName = ResolveName(RawName, ParamMap);

            if (FuncName == "pthread_mutex_lock" || FuncName == "pthread_mutex_trylock") {
                for (const std::string &Prev : ActiveLocks) {
                    LockPair P;
                    P.From = Prev;
                    P.To = MutexName;
                    P.ContextLocks = ActiveLocks;
                    Result.push_back(P);
                }
                ActiveLocks.insert(MutexName);
            } else {
                ActiveLocks.erase(MutexName);
            }
            continue;
        }

        if (FuncName == "pthread_create") {
            if (Call->getNumArgs() >= 3) {
                const Expr *ThreadArg = Call->getArg(2)->IgnoreParenImpCasts();
                if (auto *Cast = dyn_cast<CastExpr>(ThreadArg)) {
                    ThreadArg = Cast->getSubExpr()->IgnoreParenImpCasts();
                }
                if (auto *Ref = dyn_cast<DeclRefExpr>(ThreadArg)) {
                    if (auto *ThreadFD = dyn_cast<FunctionDecl>(Ref->getDecl())) {
                        const FunctionDecl *ThreadDef = ThreadFD->getDefinition();
                        if (ThreadDef && ThreadDef->hasBody() && !CallStack.count(ThreadDef)) {
                            std::set<std::string> EmptyLocks;
                            std::map<std::string, std::string> EmptyParamMap;
                            AnalyzeFunctionBody(ThreadDef, Context, EmptyLocks, Result,
                                                 CallStack, EmptyParamMap);
                        }
                    }
                }
            }
            continue;
        }

        // Obican poziv korisnicke funkcije - udji unutra
        const FunctionDecl *Definition = Callee->getDefinition();
        if (Definition && Definition->hasBody() && !CallStack.count(Definition)) {

            // NOVO: napravi novu ParamMap za POZVANU funkciju - uparuje
            // njene parametre sa stvarnim argumentima OVOG poziva
            std::map<std::string, std::string> NewParamMap;
            unsigned NumParams = Definition->getNumParams();
            for (unsigned i = 0; i < Call->getNumArgs() && i < NumParams; i++) {
                std::string ArgName = ExtractVarName(Call->getArg(i));
                // Ako je i sam argument bio parametar (nasledjen iz spoljasnjeg
                // poziva), prevedi ga kroz TRENUTNU ParamMap pre upisa
                ArgName = ResolveName(ArgName, ParamMap);

                std::string ParamName = Definition->getParamDecl(i)->getNameAsString();
                if (ArgName != "?") {
                    NewParamMap[ParamName] = ArgName;
                }
            }

            ActiveLocks = AnalyzeFunctionBody(
                Definition, Context, ActiveLocks, Result, CallStack, NewParamMap);
        }
    }

    return ActiveLocks;
}

static std::set<std::string> ComputeLockPairs(
    const CFG &Cfg,
    std::set<std::string> InitialLocks,
    std::vector<LockPair> &Result,
    ASTContext &Context,
    std::set<const FunctionDecl*> &CallStack,
    const std::map<std::string, std::string> &ParamMap) {

    std::map<const CFGBlock*, std::set<std::string>> LocksetAtEntry;
    std::vector<const CFGBlock*> Worklist;

    const CFGBlock *Entry = &Cfg.getEntry();
    LocksetAtEntry[Entry] = InitialLocks;
    Worklist.push_back(Entry);

    while (!Worklist.empty()) {
        const CFGBlock *Block = Worklist.back();
        Worklist.pop_back();

        std::set<std::string> InSet = LocksetAtEntry[Block];
        std::set<std::string> OutSet = ProcessBlock(
            Block, InSet, Result, Context, CallStack, ParamMap);

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

    const CFGBlock *ExitBlock = &Cfg.getExit();
    auto It = LocksetAtEntry.find(ExitBlock);
    if (It != LocksetAtEntry.end()) {
        return It->second;
    }
    return InitialLocks;
}

static std::set<std::string> AnalyzeFunctionBody(
    const FunctionDecl *FD,
    ASTContext &Context,
    std::set<std::string> InitialLocks,
    std::vector<LockPair> &Result,
    std::set<const FunctionDecl*> &CallStack,
    const std::map<std::string, std::string> &ParamMap) {

    if (!FD->hasBody()) return InitialLocks;

    std::unique_ptr<CFG> Cfg = CFG::buildCFG(
        FD, FD->getBody(), &Context, CFG::BuildOptions());
    if (!Cfg) return InitialLocks;

    CallStack.insert(FD);
    std::set<std::string> OutLocks = ComputeLockPairs(
        *Cfg, InitialLocks, Result, Context, CallStack, ParamMap);
    CallStack.erase(FD);

    return OutLocks;
}

std::vector<LockPair> FindLockOrderPairs(FunctionDecl *FD, ASTContext &Context) {
    std::vector<LockPair> Result;
    std::set<const FunctionDecl*> CallStack;
    std::set<std::string> InitialLocks;
    std::map<std::string, std::string> EmptyParamMap;

    AnalyzeFunctionBody(FD, Context, InitialLocks, Result, CallStack, EmptyParamMap);

    return Result;
}