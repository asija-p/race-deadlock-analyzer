#include "RaceAnalyzer.h"
#include "../common/ASTUtils.h"
#include "../common/InterproceduralWalker.h"
#include "../common/LockState.h"
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <map>
#include "../common/LockRecognition.h"

// SKELET - pokazuje MEHANIZAM prikljucenja na deljeni InterproceduralWalker,
// ne konacan algoritam. Ono sto realno jos treba doraditi (namerno
// ostavljeno kao TODO, jer je to vec pitanje race-detekcione politike a ne
// arhitekture):
//
//  1. Sta se racuna kao "deljena" promenljiva? Ovde se belezi SVAKA
//     pisana/citana promenljiva. Trebalo bi filtrirati na globalne/staticke
//     promenljive i pristupe preko pokazivaca/parametara (StorageClass,
//     da li je Decl lokalni VarDecl bez adrese uzete van funkcije...) -
//     cisto lokalne stek promenljive ne mogu izazvati race.
//  2. IsWrite se ovde odredjuje samo iz BinaryOperator sa '=' - ne hvata
//     slozene slucajeve (compound assignment +=, ++, pass-by-reference u
//     pozivu funkcije koja pise kroz pokazivac).
//  3. RaceDetector (analogno CycleDetector-u) treba da uporedjuje SVAKI PAR
//     MemoryAccess zapisa: razlicit ThreadId, bar jedan IsWrite, prazan
//     presek Lockset mapa (kljucevi), i da iskoristi JoinedThreads/
//     CreatedInLoop na isti nacin kao HasJoinPrecedence kod deadlocka.
class RaceVisitor : public AnalysisVisitor {
public:
    explicit RaceVisitor(std::vector<MemoryAccess> &Result) : Result(Result) {}

    // Race analizi ne trebaju posebni pozivi funkcija (osim mozda atomic_*
    // primitiva u buducnosti) - pusti Walkeru generic interproceduralni ulazak.
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

    void OnStmt(const Stmt *S, LockState &State, const CFGBlock *,
                const std::map<std::string, std::string> &ParamMap,
                const std::string &ThreadId, ASTContext &Context,
                const std::set<std::string> &CreatedInLoop) override {
        auto *BinOp = dyn_cast<BinaryOperator>(S);
        if (!BinOp || !BinOp->isAssignmentOp()) return;

        // TODO(1): filtrirati na stvarno deljene promenljive.
        std::string RawName = ExtractVarName(BinOp->getLHS());
        std::string VarName = ResolveName(RawName, ParamMap);
        if (VarName == "?") return;

        MemoryAccess Access;
        Access.VarName = VarName;
        Access.IsWrite = true;
        Access.MustLockset = State.Must;
        Access.MayLockset = State.May;
        Access.JoinedThreads = State.JoinedThreads;
        Access.MustActiveThreads = State.MustActiveThreads;
        Access.MayActiveThreads = State.MayActiveThreads;
        Access.CreatedInLoop = CreatedInLoop.count(ThreadId) > 0;
        Access.ThreadId = ThreadId;
        Access.Line = Context.getSourceManager().getSpellingLineNumber(S->getBeginLoc());

        BlockBuffer.push_back(Access);
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