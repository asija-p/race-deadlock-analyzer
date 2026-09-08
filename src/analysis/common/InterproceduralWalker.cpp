#include "InterproceduralWalker.h"
#include "ASTUtils.h"
#include "CFGUtils.h"
#include <clang/Basic/SourceManager.h>
#include <vector>

static LockState ProcessBlock(
    const CFGBlock *Block,
    LockState State,
    AnalysisVisitor &Visitor,
    ASTContext &Context,
    CallStackMap &CallStack,
    const std::map<std::string, std::string> &ParamMap,
    const std::string &ThreadId,
    std::set<std::string> &CreatedInLoop) {

    Visitor.BeginBlock(Block);

    for (const CFGElement &Elem : *Block) {
        auto CS = Elem.getAs<CFGStmt>();
        if (!CS) continue;
        const Stmt *S = CS->getStmt();

        Visitor.OnStmt(S, State, Block, ParamMap, ThreadId, Context, CreatedInLoop);

        auto *Call = dyn_cast<CallExpr>(S);
        if (!Call) continue;

        const FunctionDecl *Callee = Call->getDirectCallee();
        if (!Callee) continue;

        std::string FuncName = Callee->getNameAsString();

        if (FuncName == "pthread_create") {
            if (Call->getNumArgs() >= 3) {
                unsigned Line = Context.getSourceManager()
                                    .getSpellingLineNumber(Call->getBeginLoc());
                std::string NewThreadId = "create_line_" + std::to_string(Line);

                if (IsBlockInLoop(Block)) {
                    CreatedInLoop.insert(NewThreadId);
                }

                std::string RawTidName = ExtractVarName(Call->getArg(0));
                std::string TidName = ResolveName(RawTidName, ParamMap);
                if (TidName != "?") {
                    State.ThreadHandles[TidName] = NewThreadId;
                }

                const Expr *ThreadArg = Call->getArg(2)->IgnoreParenImpCasts();
                if (auto *Cast = dyn_cast<CastExpr>(ThreadArg)) {
                    ThreadArg = Cast->getSubExpr()->IgnoreParenImpCasts();
                }
                if (auto *Ref = dyn_cast<DeclRefExpr>(ThreadArg)) {
                    if (auto *ThreadFD = dyn_cast<FunctionDecl>(Ref->getDecl())) {
                        const FunctionDecl *ThreadDef = ThreadFD->getDefinition();
                        if (ThreadDef && ThreadDef->hasBody()) {
                            LockState EmptyState;
                            EmptyState.ThreadHandles = State.ThreadHandles;
                            EmptyState.JoinedThreads = State.JoinedThreads;
                            EmptyState.MustActiveThreads = State.MustActiveThreads;
                            EmptyState.MustActiveThreads.insert(ThreadId);
                            EmptyState.MayActiveThreads = State.MayActiveThreads;
                            EmptyState.MayActiveThreads.insert(ThreadId);
                            EmptyState.KnownThreads = State.KnownThreads;
                            EmptyState.KnownThreads.insert(ThreadId);

                            State.MustActiveThreads.insert(NewThreadId);
                            State.MayActiveThreads.insert(NewThreadId);
                            State.KnownThreads.insert(NewThreadId);

                            std::map<std::string, std::string> EmptyParamMap;
                            CallContext EmptyContext{EmptyState, EmptyParamMap};
                            auto It = CallStack.find(ThreadDef);
                            bool ShouldEnter = (It == CallStack.end()) ||
                                                (It->second != EmptyContext);
                            if (ShouldEnter) {
                                Visitor.EnterNestedCall();
                                LockState ChildExitState = WalkFunction(
                                    ThreadDef, Context, EmptyState, Visitor,
                                    CallStack, EmptyParamMap, NewThreadId, CreatedInLoop);
                                Visitor.ExitNestedCall();

                                State.KnownThreads.insert(ChildExitState.KnownThreads.begin(),
                                                           ChildExitState.KnownThreads.end());
                            }
                        }
                    }
                }
            }
            continue;
        }

        if (FuncName == "pthread_join") {
            if (Call->getNumArgs() >= 1) {
                std::string RawTidName = ExtractVarName(Call->getArg(0));
                std::string TidName = ResolveName(RawTidName, ParamMap);
                auto HandleIt = State.ThreadHandles.find(TidName);
                if (HandleIt != State.ThreadHandles.end()) {
                    State.JoinedThreads.insert(HandleIt->second);

                    if (!CreatedInLoop.count(HandleIt->second)) {
                        State.MustActiveThreads.erase(HandleIt->second);
                        State.MayActiveThreads.erase(HandleIt->second);
                    }
                }
            }
            continue;
        }

        bool HandledByVisitor = Visitor.OnCallExpr(
            Call, Callee, FuncName, State, Block, ParamMap, ThreadId, Context, CreatedInLoop);
        if (HandledByVisitor) continue;

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

            CallContext NewContext{State, NewParamMap};
            auto It = CallStack.find(Definition);
            bool ShouldEnter = (It == CallStack.end()) ||
                                (It->second != NewContext);

            if (ShouldEnter) {
                Visitor.EnterNestedCall();
                State = WalkFunction(
                    Definition, Context, State, Visitor, CallStack, NewParamMap, ThreadId, CreatedInLoop);
                Visitor.ExitNestedCall();
            }
        }
    }

    Visitor.EndBlock(Block);

    return State;
}

static LockState ComputeFixpoint(
    const CFG &Cfg,
    LockState InitialState,
    AnalysisVisitor &Visitor,
    ASTContext &Context,
    CallStackMap &CallStack,
    const std::map<std::string, std::string> &ParamMap,
    const std::string &ThreadId,
    std::set<std::string> &CreatedInLoop) {

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
            Block, InState, Visitor, Context, CallStack, ParamMap, ThreadId, CreatedInLoop);

        for (const CFGBlock::AdjacentBlock &Succ : Block->succs()) {
            if (!Succ.isReachable()) continue;
            const CFGBlock *SuccBlock = Succ.getReachableBlock();

            LockState NewState;
            if (!Visited[SuccBlock]) {
                NewState = OutState;
                Visited[SuccBlock] = true;
            } else {
                const LockState &Existing = StateAtEntry[SuccBlock];
                NewState.May = OutState.May;
                MergeMayInto(NewState.May, Existing.May);
                NewState.Must = IntersectMust(OutState.Must, Existing.Must);
                NewState.ThreadHandles = OutState.ThreadHandles;
                MergeThreadHandlesInto(NewState.ThreadHandles, Existing.ThreadHandles);
                NewState.JoinedThreads = IntersectJoinedThreads(OutState.JoinedThreads, Existing.JoinedThreads);
                NewState.MustActiveThreads = IntersectActiveThreads(OutState.MustActiveThreads, Existing.MustActiveThreads);
                NewState.MayActiveThreads = UnionActiveThreads(OutState.MayActiveThreads, Existing.MayActiveThreads);
                NewState.KnownThreads = UnionActiveThreads(OutState.KnownThreads, Existing.KnownThreads);
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

LockState WalkFunction(const FunctionDecl *FD, ASTContext &Context,
                        LockState InitialState, AnalysisVisitor &Visitor,
                        CallStackMap &CallStack,
                        const std::map<std::string, std::string> &ParamMap,
                        const std::string &ThreadId,
                        std::set<std::string> &CreatedInLoop) {

    if (!FD->hasBody()) return InitialState;

    std::unique_ptr<CFG> Cfg = CFG::buildCFG(
        FD, FD->getBody(), &Context, CFG::BuildOptions());
    if (!Cfg) return InitialState;

    bool HadPrevious = CallStack.count(FD) > 0;
    CallContext PreviousValue;
    if (HadPrevious) {
        PreviousValue = CallStack[FD];
    }

    CallStack[FD] = CallContext{InitialState, ParamMap};
    LockState OutState = ComputeFixpoint(
        *Cfg, InitialState, Visitor, Context, CallStack, ParamMap, ThreadId, CreatedInLoop);
    Visitor.FlushCFG();

    if (HadPrevious) {
        CallStack[FD] = PreviousValue;
    } else {
        CallStack.erase(FD);
    }

    return OutState;
}