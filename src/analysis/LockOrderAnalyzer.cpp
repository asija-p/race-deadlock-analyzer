#include "LockOrderAnalyzer.h"
#include <clang/Analysis/CFG.h>
#include <iostream>
#include <set>
#include <map>
#include <algorithm>
#include <iterator>
#include <clang/Basic/SourceManager.h>
#include <clang/AST/RecursiveASTVisitor.h>

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
        // Normalizacija: (*p).field ima Base = UnaryOperator(Deref, p), dok
        // p->field ima Base = p direktno (isti semanticki objekat - polje
        // strukture na koju p pokazuje). Bez ovoga, (*p).field vraca razbijen
        // naziv "?.field" jer IgnoreParenImpCasts ne skida UnaryOperator, pa
        // DeclRefExpr ispod nikad nije pronadjen.
        if (auto *DerefOp = dyn_cast<UnaryOperator>(Base)) {
            if (DerefOp->getOpcode() == UO_Deref) {
                Base = DerefOp->getSubExpr()->IgnoreParenImpCasts();
            }
        }
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

struct LockState {
    std::map<std::string, LockKind> May;
    std::map<std::string, LockKind> Must;

    bool operator==(const LockState &Other) const {
        return May == Other.May && Must == Other.Must;
    }
    bool operator!=(const LockState &Other) const {
        return !(*this == Other);
    }
};

// Vraca skup imena brava iz May/Must mape (za popunjavanje LockPair::ContextLocks,
// koji ostaje set<string> - ne zanima nas mod za taj deo, samo koja su imena bila u igri).
static std::set<std::string> KeysOf(const std::map<std::string, LockKind> &M) {
    std::set<std::string> Keys;
    for (const auto &Entry : M) {
        Keys.insert(Entry.first);
    }
    return Keys;
}

// Spaja Source u Target (May-lockset, unija na granama CFG-a).
// Ako je ista brava prisutna na obe grane ali sa RAZLICITIM modom (npr. Read na
// jednoj grani, Write na drugoj), konzervativno je tretiramo kao Write - bolje
// lazni pozitiv nego da progutamo mogucu ekskluzivnu akviziciju.
static void MergeMayInto(std::map<std::string, LockKind> &Target,
                          const std::map<std::string, LockKind> &Source) {
    for (const auto &Entry : Source) {
        auto It = Target.find(Entry.first);
        if (It == Target.end()) {
            Target[Entry.first] = Entry.second;
        } else if (It->second != Entry.second) {
            It->second = LockKind::Write;
        }
    }
}

// Presek Must-lockset-a dve grane. Brava ostaje u preseku samo ako je SIGURNO
// drzana na obe grane. Ako je mod RAZLICIT izmedju grana, ne mozemo tvrditi
// da je SIGURNO Write (to bi bila neosnovana tvrdnja o zastiti koja moze
// sakriti pravi deadlock u HasCommonLock) - zato ovde konzervativno biramo
// Read, tj. "nije garantovano ekskluzivno". Ovo je NAMERNO suprotan smer od
// MergeMayInto (koji za May-lockset bira Write kod nesigurnosti) - odgovaraju
// na suprotna pitanja: May pita "da li se MOZE sudariti" (Write = gori slucaj),
// a Must pita "da li smo SIGURNO zasticeni" (Read = gori slucaj, tj. manje
// pouzdana zastita).
static std::map<std::string, LockKind> IntersectMust(
    const std::map<std::string, LockKind> &A,
    const std::map<std::string, LockKind> &B) {
    std::map<std::string, LockKind> Result;
    for (const auto &Entry : A) {
        auto It = B.find(Entry.first);
        if (It != B.end()) {
            Result[Entry.first] = (Entry.second == It->second) ? Entry.second : LockKind::Read;
        }
    }
    return Result;
}


struct CallContext {
    LockState State;
    std::map<std::string, std::string> ParamMap;

    bool operator==(const CallContext &Other) const {
        return State == Other.State && ParamMap == Other.ParamMap;
    }
    bool operator!=(const CallContext &Other) const {
        return !(*this == Other);
    }
};

using CallStackMap = std::map<const FunctionDecl*, CallContext>;

static LockState AnalyzeFunctionBody(
    const FunctionDecl *FD,
    ASTContext &Context,
    LockState InitialState,
    std::vector<LockPair> &Result,
    CallStackMap &CallStack,
    const std::map<std::string, std::string> &ParamMap,
    const std::string &ThreadId);

static LockState ProcessBlock(
    const CFGBlock *Block,
    LockState State,
    std::vector<LockPair> &Result,
    ASTContext &Context,
    CallStackMap &CallStack,
    const std::map<std::string, std::string> &ParamMap,
    const std::string &ThreadId) {

    for (const CFGElement &Elem : *Block) {
        auto CS = Elem.getAs<CFGStmt>();
        if (!CS) continue;
        const Stmt *S = CS->getStmt();
        auto *Call = dyn_cast<CallExpr>(S);
        if (!Call) continue;

        const FunctionDecl *Callee = Call->getDirectCallee();
        if (!Callee) continue;

        std::string FuncName = Callee->getNameAsString();

        // Write-mod brave: mutex i spinlock su UVEK ekskluzivni; rwlock wrlock isto.
        bool IsWriteLock = (FuncName == "pthread_mutex_lock" || FuncName == "pthread_mutex_trylock" ||
                             FuncName == "pthread_spin_lock" || FuncName == "pthread_spin_trylock" ||
                             FuncName == "pthread_rwlock_wrlock" || FuncName == "pthread_rwlock_trywrlock");
        // Read-mod: samo rwlock rdlock - deljena brava, ne sudara se sa drugim Read-om.
        bool IsReadLock = (FuncName == "pthread_rwlock_rdlock" || FuncName == "pthread_rwlock_tryrdlock");
        bool IsUnlock = (FuncName == "pthread_mutex_unlock" ||
                          FuncName == "pthread_spin_unlock" ||
                          FuncName == "pthread_rwlock_unlock");

        if (IsWriteLock || IsReadLock || IsUnlock) {
            if (Call->getNumArgs() == 0) continue;

            std::string RawName = ExtractVarName(Call->getArg(0));
            std::string MutexName = ResolveName(RawName, ParamMap);

            if (IsWriteLock || IsReadLock) {
                LockKind NewKind = IsWriteLock ? LockKind::Write : LockKind::Read;

                for (const auto &PrevEntry : State.May) {
                    LockPair P;
                    P.From = PrevEntry.first;
                    P.To = MutexName;
                    P.FromKind = PrevEntry.second;
                    P.ToKind = NewKind;
                    P.ContextLocks = KeysOf(State.May);
                    P.MustContextLocks = KeysOf(State.Must);
                    P.MustContextKinds = State.Must;
                    P.ThreadId = ThreadId;
                    Result.push_back(P);
                }
                State.May[MutexName] = NewKind;
                State.Must[MutexName] = NewKind;
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
                            std::map<std::string, std::string> EmptyParamMap;
                            CallContext EmptyContext{EmptyState, EmptyParamMap};
                            auto It = CallStack.find(ThreadDef);
                            bool ShouldEnter = (It == CallStack.end()) ||
                                                (It->second != EmptyContext);
                            if (ShouldEnter) {
                                // NOVO: ThreadId = MESTO poziva (linija), ne ime funkcije
                                unsigned Line = Context.getSourceManager()
                                                    .getSpellingLineNumber(Call->getBeginLoc());
                                std::string NewThreadId = "create_line_" + std::to_string(Line);

                                AnalyzeFunctionBody(ThreadDef, Context, EmptyState, Result,
                                                    CallStack, EmptyParamMap, NewThreadId);
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

            CallContext NewContext{State, NewParamMap};
            auto It = CallStack.find(Definition);
            bool ShouldEnter = (It == CallStack.end()) ||
                                (It->second != NewContext);

            if (ShouldEnter) {
                // Obican poziv - NASLEDJUJE isti ThreadId (deo je iste niti)
                State = AnalyzeFunctionBody(
                    Definition, Context, State, Result, CallStack, NewParamMap, ThreadId);
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
    const std::map<std::string, std::string> &ParamMap,
    const std::string &ThreadId) {

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
            Block, InState, Result, Context, CallStack, ParamMap, ThreadId);

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
    const std::map<std::string, std::string> &ParamMap,
    const std::string &ThreadId) {

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
    LockState OutState = ComputeLockPairs(
        *Cfg, InitialState, Result, Context, CallStack, ParamMap, ThreadId);

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

    std::string ThreadId = FD->getNameAsString();

    AnalyzeFunctionBody(FD, Context, InitialState, Result, CallStack, EmptyParamMap, ThreadId);

    return Result;
}