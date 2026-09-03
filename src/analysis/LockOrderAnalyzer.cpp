#include "LockOrderAnalyzer.h"
#include <clang/Analysis/CFG.h>
#include <iostream>
#include <set>
#include <map>
#include <algorithm>
#include <iterator>

using namespace clang;

static std::string ExtractVarName(const Expr *Arg) {
    Arg = Arg->IgnoreParenImpCasts();
    if (auto *Unary = dyn_cast<UnaryOperator>(Arg)) {
        Arg = Unary->getSubExpr()->IgnoreParenImpCasts();
    }

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

    if (auto *Member = dyn_cast<MemberExpr>(Arg)) {
        std::string BaseName = "?";
        const Expr *Base = Member->getBase()->IgnoreParenImpCasts();
        if (auto *BaseRef = dyn_cast<DeclRefExpr>(Base)) {
            BaseName = BaseRef->getDecl()->getNameAsString();
        }
        std::string FieldName = Member->getMemberDecl()->getNameAsString();
        return BaseName + "." + FieldName;
    }

    if (auto *Ref = dyn_cast<DeclRefExpr>(Arg)) {
        return Ref->getDecl()->getNameAsString();
    }
    return "?";
}

static std::string ResolveName(const std::string &Name,
                                 const std::map<std::string, std::string> &ParamMap) {
    auto It = ParamMap.find(Name);
    if (It != ParamMap.end()) {
        return It->second;
    }
    return Name;
}

// LockState nosi OBA stanja zajedno kroz analizu:
// May  = sta je MOGLO biti zakljucano (union na spajanju grana)
// Must = sta je SIGURNO bilo zakljucano (presek na spajanju grana)
struct LockState {
    std::set<std::string> May;
    std::set<std::string> Must;

    bool operator==(const LockState &Other) const {
        return May == Other.May && Must == Other.Must;
    }
    bool operator!=(const LockState &Other) const {
        return !(*this == Other);
    }
};

// CallStack pamti KOJIM LockState-om smo POSLEDNJI PUT usli u svaku funkciju
// trenutno "na stack-u" - omogucava fixpoint pristup rekurziji.
using CallStackMap = std::map<const FunctionDecl*, LockState>;

static LockState AnalyzeFunctionBody(
    const FunctionDecl *FD,
    ASTContext &Context,
    LockState InitialState,
    std::vector<LockPair> &Result,
    CallStackMap &CallStack,
    const std::map<std::string, std::string> &ParamMap);

static LockState ProcessBlock(
    const CFGBlock *Block,
    LockState State,
    std::vector<LockPair> &Result,
    ASTContext &Context,
    CallStackMap &CallStack,
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

            std::string RawName = ExtractVarName(Call->getArg(0));
            std::string MutexName = ResolveName(RawName, ParamMap);

            if (FuncName == "pthread_mutex_lock" || FuncName == "pthread_mutex_trylock") {
                // Pravimo parove iz MAY skupa (sound - ne sme propustiti nijednu
                // mogucu ivicu), ali svaki par nosi i MUST kontekst za kasniji
                // common-locks filter.
                for (const std::string &Prev : State.May) {
                    LockPair P;
                    P.From = Prev;
                    P.To = MutexName;
                    P.ContextLocks = State.May;
                    P.MustContextLocks = State.Must;
                    Result.push_back(P);
                }
                State.May.insert(MutexName);
                State.Must.insert(MutexName);
            } else {
                State.May.erase(MutexName);
                State.Must.erase(MutexName);
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
                        if (ThreadDef && ThreadDef->hasBody()) {
                            LockState EmptyState;
                            auto It = CallStack.find(ThreadDef);
                            bool ShouldEnter = (It == CallStack.end()) ||
                                                (It->second != EmptyState);
                            if (ShouldEnter) {
                                std::map<std::string, std::string> EmptyParamMap;
                                AnalyzeFunctionBody(ThreadDef, Context, EmptyState, Result,
                                                     CallStack, EmptyParamMap);
                            }
                        }
                    }
                }
            }
            continue;
        }

        const FunctionDecl *Definition = Callee->getDefinition();
        if (Definition && Definition->hasBody()) {

            std::map<std::string, std::string> NewParamMap;
            unsigned NumParams = Definition->getNumParams();
            for (unsigned i = 0; i < Call->getNumArgs() && i < NumParams; i++) {
                std::string ArgName = ExtractVarName(Call->getArg(i));
                ArgName = ResolveName(ArgName, ParamMap);

                std::string ParamName = Definition->getParamDecl(i)->getNameAsString();
                if (ArgName != "?") {
                    NewParamMap[ParamName] = ArgName;
                }
            }

            auto It = CallStack.find(Definition);
            bool ShouldEnter = (It == CallStack.end()) ||
                                (It->second != State);

            if (ShouldEnter) {
                State = AnalyzeFunctionBody(
                    Definition, Context, State, Result, CallStack, NewParamMap);
            }
        }
    }

    return State;
}

static LockState ComputeLockPairs(
    const CFG &Cfg,
    LockState InitialState,
    std::vector<LockPair> &Result,
    ASTContext &Context,
    CallStackMap &CallStack,
    const std::map<std::string, std::string> &ParamMap) {

    std::map<const CFGBlock*, LockState> StateAtEntry;
    std::map<const CFGBlock*, bool> Visited;
    std::vector<const CFGBlock*> Worklist;

    const CFGBlock *Entry = &Cfg.getEntry();
    StateAtEntry[Entry] = InitialState;
    Visited[Entry] = true;
    Worklist.push_back(Entry);

    while (!Worklist.empty()) {
        const CFGBlock *Block = Worklist.back();
        Worklist.pop_back();

        LockState InState = StateAtEntry[Block];
        LockState OutState = ProcessBlock(
            Block, InState, Result, Context, CallStack, ParamMap);

        for (const CFGBlock::AdjacentBlock &Succ : Block->succs()) {
            if (!Succ.isReachable()) continue;
            const CFGBlock *SuccBlock = Succ.getReachableBlock();

            LockState NewState;
            if (!Visited[SuccBlock]) {
                // Prvi put stizemo ovde - preuzmi stanje kakvo jeste
                NewState = OutState;
                Visited[SuccBlock] = true;
            } else {
                // Vec smo bili ovde - May: UNIJA, Must: PRESEK sa postojecim
                const LockState &Existing = StateAtEntry[SuccBlock];
                NewState.May = OutState.May;
                NewState.May.insert(Existing.May.begin(), Existing.May.end());

                std::set_intersection(
                    OutState.Must.begin(), OutState.Must.end(),
                    Existing.Must.begin(), Existing.Must.end(),
                    std::inserter(NewState.Must, NewState.Must.begin()));
            }

            if (StateAtEntry.find(SuccBlock) == StateAtEntry.end() ||
                NewState != StateAtEntry[SuccBlock]) {
                StateAtEntry[SuccBlock] = NewState;
                Worklist.push_back(SuccBlock);
            }
        }
    }

    const CFGBlock *ExitBlock = &Cfg.getExit();
    auto It = StateAtEntry.find(ExitBlock);
    if (It != StateAtEntry.end()) {
        return It->second;
    }
    return InitialState;
}

static LockState AnalyzeFunctionBody(
    const FunctionDecl *FD,
    ASTContext &Context,
    LockState InitialState,
    std::vector<LockPair> &Result,
    CallStackMap &CallStack,
    const std::map<std::string, std::string> &ParamMap) {

    if (!FD->hasBody()) return InitialState;

    std::unique_ptr<CFG> Cfg = CFG::buildCFG(
        FD, FD->getBody(), &Context, CFG::BuildOptions());
    if (!Cfg) return InitialState;

    bool HadPrevious = CallStack.count(FD) > 0;
    LockState PreviousValue;
    if (HadPrevious) {
        PreviousValue = CallStack[FD];
    }

    CallStack[FD] = InitialState;
    LockState OutState = ComputeLockPairs(
        *Cfg, InitialState, Result, Context, CallStack, ParamMap);

    if (HadPrevious) {
        CallStack[FD] = PreviousValue;
    } else {
        CallStack.erase(FD);
    }

    return OutState;
}

std::vector<LockPair> FindLockOrderPairs(FunctionDecl *FD, ASTContext &Context) {
    std::vector<LockPair> Result;
    CallStackMap CallStack;
    LockState InitialState;
    std::map<std::string, std::string> EmptyParamMap;

    AnalyzeFunctionBody(FD, Context, InitialState, Result, CallStack, EmptyParamMap);

    return Result;
}