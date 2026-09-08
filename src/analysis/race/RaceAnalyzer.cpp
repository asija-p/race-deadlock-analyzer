#include "RaceAnalyzer.h"
#include "../common/ASTUtils.h"
#include "../common/InterproceduralWalker.h"
#include "../common/LockState.h"
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <map>
#include "../common/LockRecognition.h"

class RaceVisitor : public AnalysisVisitor {
public:
    explicit RaceVisitor(std::vector<MemoryAccess> &Result) : Result(Result) {}

    bool OnCallExpr(const CallExpr *Call, const FunctionDecl *,
                    const std::string &FuncName, LockState &State,
                    const CFGBlock *,
                    const std::map<std::string, std::string> &ParamMap,
                    const std::string &,
                    ASTContext &,
                    const std::set<std::string> &) override {

        LockCallKind Kind = ClassifyLockCall(FuncName);
        if (Kind == LockCallKind::NotALock) {
            return false;
        }

        if (Call->getNumArgs() == 0) return true;

        std::string RawName = ExtractVarName(Call->getArg(0));
        std::string MutexName = ResolveName(RawName, ParamMap);
        ApplyLockCallToState(Kind, MutexName, State);

        return true;
    }

    static bool IsThreadBookkeepingCall(const CallExpr *Call) {
        const FunctionDecl *Callee = Call->getDirectCallee();
        if (!Callee) return false;
        std::string Name = Callee->getNameAsString();
        if (Name == "pthread_create" || Name == "pthread_join") return true;
        return ClassifyLockCall(Name) != LockCallKind::NotALock;
    }

    static bool IsVariableAccessNode(const Stmt *S) {
        if (!S) return false;
        if (isa<DeclRefExpr>(S)) return true;
        if (isa<MemberExpr>(S)) return true;
        if (isa<ArraySubscriptExpr>(S)) return true;
        if (auto *U = dyn_cast<UnaryOperator>(S)) return U->getOpcode() == UO_Deref;
        return false;
    }

    static void CollectReads(const Stmt *S, const Expr *ExcludeExact,
                              std::vector<const Expr *> &Reads) {
        if (!S) return;

        if (auto *Call = dyn_cast<CallExpr>(S)) {
            if (IsThreadBookkeepingCall(Call)) return;
        }

        if (IsVariableAccessNode(S)) {
            if (S != ExcludeExact) {
                Reads.push_back(cast<Expr>(S));
            }
            // arr[i] - indeks se UVEK racuna kao citanje, cak i kad je
            // citav arr[i] izuzet jer je LHS dodele (adresa se i dalje
            // racuna citanjem indeksa).
            if (auto *ArrSub = dyn_cast<ArraySubscriptExpr>(S)) {
                CollectReads(ArrSub->getIdx(), ExcludeExact, Reads);
            }
            return;
        }

        for (const Stmt *Child : S->children()) {
            CollectReads(Child, ExcludeExact, Reads);
        }
    }

    void OnStmt(const Stmt *S, LockState &State, const CFGBlock *,
                const std::map<std::string, std::string> &ParamMap,
                const std::string &ThreadId, ASTContext &Context,
                const std::set<std::string> &CreatedInLoop) override {

        auto MakeAccess = [&](const Expr *AccessExpr, bool IsWrite) {
            if (!IsSharedAccess(AccessExpr)) return;

            std::string RawName = ExtractVarName(AccessExpr);
            std::string VarName = ResolveName(RawName, ParamMap);
            if (VarName == "?") return;

            MemoryAccess Access;
            Access.VarName = VarName;
            Access.IsWrite = IsWrite;
            Access.MustLockset = State.Must;
            Access.MayLockset = State.May;
            Access.JoinedThreads = State.JoinedThreads;
            Access.MustActiveThreads = State.MustActiveThreads;
            Access.MayActiveThreads = State.MayActiveThreads;
            Access.KnownThreadsAtThisPoint = State.KnownThreads;
            Access.CreatedInLoop = CreatedInLoop.count(ThreadId) > 0;
            Access.ThreadId = ThreadId;
            Access.Line = Context.getSourceManager().getSpellingLineNumber(AccessExpr->getBeginLoc());

            BlockBuffer.push_back(Access);
        };

        auto *BinOp = dyn_cast<BinaryOperator>(S);
        const Expr *WriteOnlyLHS = nullptr;

        if (BinOp && BinOp->isAssignmentOp()) {
            MakeAccess(BinOp->getLHS(), /*IsWrite=*/true);
            if (BinOp->getOpcode() == BO_Assign) {
                WriteOnlyLHS = BinOp->getLHS();
            }
        }

        std::vector<const Expr *> Reads;
        CollectReads(S, WriteOnlyLHS, Reads);
        for (const Expr *R : Reads) {
            MakeAccess(R, /*IsWrite=*/false);
        }
    }

    void BeginBlock(const CFGBlock *) override { BlockBuffer.clear(); }

    void EndBlock(const CFGBlock *Block) override {
        AccessesAtBlock[Block] = BlockBuffer;
    }

    void EnterNestedCall() override {
        SavedFrames.push_back({std::move(BlockBuffer), std::move(AccessesAtBlock)});
        BlockBuffer.clear();
        AccessesAtBlock.clear();
    }

    void ExitNestedCall() override {
        BlockBuffer = std::move(SavedFrames.back().first);
        AccessesAtBlock = std::move(SavedFrames.back().second);
        SavedFrames.pop_back();
    }

    void FlushCFG() override {
        std::vector<MemoryAccess> Flattened;
        for (const auto &Entry : AccessesAtBlock) {
            Flattened.insert(Flattened.end(), Entry.second.begin(), Entry.second.end());
        }
        AccessesAtBlock.clear();

        if (SavedFrames.empty()) {
            Result.insert(Result.end(), Flattened.begin(), Flattened.end());
        } else {
            SavedFrames.back().first.insert(
                SavedFrames.back().first.end(), Flattened.begin(), Flattened.end());
        }
    }

private:
    std::vector<MemoryAccess> &Result;
    std::vector<MemoryAccess> BlockBuffer;
    std::map<const CFGBlock *, std::vector<MemoryAccess>> AccessesAtBlock;
    std::vector<std::pair<std::vector<MemoryAccess>,
                           std::map<const CFGBlock *, std::vector<MemoryAccess>>>> SavedFrames;
};

std::vector<MemoryAccess> FindMemoryAccesses(FunctionDecl *FD, ASTContext &Context,
                                              std::set<std::string> &CreatedInLoop) {
    std::vector<MemoryAccess> Result;
    RaceVisitor Visitor(Result);

    CallStackMap CallStack;
    LockState InitialState;
    std::map<std::string, std::string> EmptyParamMap;

    std::string ThreadId = FD->getNameAsString();

    WalkFunction(FD, Context, InitialState, Visitor, CallStack, EmptyParamMap, ThreadId, CreatedInLoop);

    return Result;
}