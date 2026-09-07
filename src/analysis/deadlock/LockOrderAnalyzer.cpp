#include "LockOrderAnalyzer.h"
#include "../common/ASTUtils.h"
#include "../common/InterproceduralWalker.h"
#include "../common/LockState.h"
#include <map>

// Sve sto je ostalo u ovom fajlu je CISTO deadlock-specificno: prepoznavanje
// pthread_mutex/rwlock/spin lock i unlock poziva i generisanje LockPair
// ivica. Sav CFG fixpoint, interproceduralna rekurzija i pthread_create/
// pthread_join knjigovodstvo sad zivi u InterproceduralWalker-u (deljeno sa
// buducom race analizom).
class DeadlockVisitor : public AnalysisVisitor {
public:
    explicit DeadlockVisitor(std::vector<LockPair> &Result) : Result(Result) {}

    bool OnCallExpr(const CallExpr *Call, const FunctionDecl * /*Callee*/,
                     const std::string &FuncName, LockState &State,
                     const CFGBlock * /*Block*/,
                     const std::map<std::string, std::string> &ParamMap,
                     const std::string &ThreadId,
                     ASTContext & /*Context*/,
                     const std::set<std::string> &CreatedInLoop) override {

        bool IsWriteLock = (FuncName == "pthread_mutex_lock" || FuncName == "pthread_mutex_trylock" ||
                             FuncName == "pthread_spin_lock" || FuncName == "pthread_spin_trylock" ||
                             FuncName == "pthread_rwlock_wrlock" || FuncName == "pthread_rwlock_trywrlock");
        bool IsReadLock = (FuncName == "pthread_rwlock_rdlock" || FuncName == "pthread_rwlock_tryrdlock");
        bool IsUnlock = (FuncName == "pthread_mutex_unlock" ||
                          FuncName == "pthread_spin_unlock" ||
                          FuncName == "pthread_rwlock_unlock");

        if (!IsWriteLock && !IsReadLock && !IsUnlock) {
            // Nije lock/unlock poziv - nije nas posao, pusti Walkeru da
            // uradi generic interproceduralni ulazak (ako funkcija ima telo).
            return false;
        }

        if (Call->getNumArgs() == 0) return true;

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
                P.JoinedThreads = State.JoinedThreads;
                P.CreatedInLoop = CreatedInLoop.count(ThreadId) > 0;
                P.ThreadId = ThreadId;
                BlockBuffer.push_back(P);
            }
            State.May[MutexName] = NewKind;
            State.Must[MutexName] = NewKind;
        } else {
            State.May.erase(MutexName);
            State.Must.erase(MutexName);
        }

        // Lock/unlock su spoljne (libpthread) funkcije bez tela - obradjeno
        // je, Walker ne treba da pokusava interproceduralni ulazak.
        return true;
    }

    void BeginBlock(const CFGBlock * /*Block*/) override {
        BlockBuffer.clear();
    }

    void EndBlock(const CFGBlock *Block) override {
        // Isto ponasanje kao originalni PairsAtBlock[Block] = BlockPairs:
        // ako fixpoint ponovo obradi ovaj blok (jer mu se ulazno stanje
        // promenilo), rezultati iz PRETHODNE obrade se odbacuju - u
        // konacan Result ulazi samo poslednja obrada svakog bloka.
        PairsAtBlock[Block] = BlockBuffer;
    }

    void EnterNestedCall() override {
        SavedFrames.push_back({std::move(BlockBuffer), std::move(PairsAtBlock)});
        BlockBuffer.clear();
        PairsAtBlock.clear();
    }

    void ExitNestedCall() override {
        BlockBuffer = std::move(SavedFrames.back().first);
        PairsAtBlock = std::move(SavedFrames.back().second);
        SavedFrames.pop_back();
    }

    void FlushCFG() override {
        std::vector<LockPair> Flattened;
        for (const auto &Entry : PairsAtBlock) {
            Flattened.insert(Flattened.end(), Entry.second.begin(), Entry.second.end());
        }
        PairsAtBlock.clear();

        if (SavedFrames.empty()) {
            Result.insert(Result.end(), Flattened.begin(), Flattened.end());
        } else {
            SavedFrames.back().first.insert(
                SavedFrames.back().first.end(), Flattened.begin(), Flattened.end());
        }
    }

private:
    std::vector<LockPair> &Result;
    std::vector<LockPair> BlockBuffer;
    std::map<const CFGBlock *, std::vector<LockPair>> PairsAtBlock;
    std::vector<std::pair<std::vector<LockPair>,
                           std::map<const CFGBlock *, std::vector<LockPair>>>> SavedFrames;
};

std::vector<LockPair> FindLockOrderPairs(FunctionDecl *FD, ASTContext &Context,
                                          std::set<std::string> &CreatedInLoop) {
    std::vector<LockPair> Result;
    DeadlockVisitor Visitor(Result);

    CallStackMap CallStack;
    LockState InitialState;
    std::map<std::string, std::string> EmptyParamMap;

    std::string ThreadId = FD->getNameAsString();

    WalkFunction(FD, Context, InitialState, Visitor, CallStack, EmptyParamMap, ThreadId, CreatedInLoop);

    return Result;
}
