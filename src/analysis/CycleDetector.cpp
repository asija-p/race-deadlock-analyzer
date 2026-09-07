#include "CycleDetector.h"
#include <map>
#include <set>
#include <algorithm>
#include <iterator>
#include <iostream>

static void DFS(
    const std::string &Node,
    const std::map<std::string, std::vector<LockPair>> &Graph,
    std::set<std::string> &Visited,
    std::set<std::string> &InProgress,
    std::vector<LockPair> &CurrentPath,
    std::vector<std::vector<LockPair>> &Cycles) {

    Visited.insert(Node);
    InProgress.insert(Node);

    auto It = Graph.find(Node);
    if (It != Graph.end()) {
        for (const LockPair &Edge : It->second) {
            const std::string &Neighbor = Edge.To;

            if (InProgress.count(Neighbor)) {
                std::vector<LockPair> Cycle;
                bool Started = false;
                for (const LockPair &P : CurrentPath) {
                    if (P.From == Neighbor) Started = true;
                    if (Started) Cycle.push_back(P);
                }
                Cycle.push_back(Edge);
                Cycles.push_back(Cycle);
            } else if (!Visited.count(Neighbor)) {
                CurrentPath.push_back(Edge);
                DFS(Neighbor, Graph, Visited, InProgress, CurrentPath, Cycles);
                CurrentPath.pop_back();
            }
        }
    }

    InProgress.erase(Node);
}

// Proverava da li SVE ivice ciklusa pripadaju ISTOJ niti (root funkciji).
// Ako da, ciklus je LAZAN ALARM - jedna nit ne moze biti u konfliktu
// sa samom sobom kroz sekvencijalne, ne-konkurentne pozive.
static bool AllSameThread(const std::vector<LockPair> &Cycle) {
    if (Cycle.empty()) return false;
    const std::string &FirstThread = Cycle[0].ThreadId;
    for (const auto &Edge : Cycle) {
        if (Edge.ThreadId != FirstThread) {
            return false;
        }
    }
    return true;
}

// Kroeningov all_concurrent kriterijum (Sec 6.2): za SVAKI PAR ivica u
// ciklusu, proveri da li dele bar jednu zajednicku Must-bravu koja STVARNO
// stiti (vidi ProtectingLockExists ispod). Ako i JEDAN par nema takvu bravu,
// ciklus ostaje (nije u potpunosti zasticen).
// NAPOMENA: ovo je parovi-po-parovima provera, ne globalni presek - to je
// tacno ono sto Kroeningova formula (∀ parova ivica) definise, i izbegava
// mesanje Must konteksta iz nepovezanih putanja izvrsavanja (npr. razlicitih
// grana rekurzije), koje bi globalni presek mogao pogresno da spoji.
//
// ISPRAVKA (bilo poznato ogranicenje): zajednicka brava stiti samo ako je
// BAR JEDNA strana drzi u Write modu. Ako obe strane drze istu rwlock bravu
// samo u Read modu, ta brava NE iskljucuje jedno izvrsavanje od drugog (dva
// citaoca mogu istovremeno drzati bravu), pa ne sme da "spase" ciklus od
// prijave - u suprotnom bismo propustili pravi deadlock.
static bool ProtectingLockExists(const LockPair &A, const LockPair &B) {
    for (const auto &EntryA : A.MustContextKinds) {
        auto ItB = B.MustContextKinds.find(EntryA.first);
        if (ItB == B.MustContextKinds.end()) continue;

        if (EntryA.second == LockKind::Write || ItB->second == LockKind::Write) {
            return true;
        }
    }
    return false;
}

static bool HasCommonLock(const std::vector<LockPair> &Cycle) {
    for (size_t i = 0; i < Cycle.size(); i++) {
        for (size_t j = i + 1; j < Cycle.size(); j++) {
            if (!ProtectingLockExists(Cycle[i], Cycle[j])) {
                return false;
            }
        }
    }
    return true;
}


// Za svaki deljeni cvor izmedju dve UZASTOPNE ivice ciklusa, proverava da li
// se te dve strane uopste MOGU sudariti. Na cvoru gde se Cycle[i] zavrsava
// (Cycle[i].To) i Cycle[i+1] pocinje (Cycle[i+1].From), Cycle[i] pokusava da
// AKVIRIRA taj cvor (mod = Cycle[i].ToKind) dok ga Cycle[i+1] vec DRZI
// (mod = Cycle[i+1].FromKind). Dva citaoca (Read/Read) se NIKAD ne sudaraju -
// pa ako je ijedan cvor u ciklusu bas takav (oba Read), ciklus fizicki ne moze
// da nastane kao pravi deadlock, cak i ako ostale provere (zajednicka brava,
// razlicita nit) prodju.
static bool AllNodesCanConflict(const std::vector<LockPair> &Cycle) {
    size_t N = Cycle.size();
    if (N == 0) return false;

    for (size_t i = 0; i < N; i++) {
        size_t Next = (i + 1) % N;
        bool BothRead = (Cycle[i].ToKind == LockKind::Read &&
                          Cycle[Next].FromKind == LockKind::Read);
        if (BothRead) {
            return false;
        }
    }
    return true;
}

static bool HasJoinPrecedence(const LockPair &A, const LockPair &B) {
    if (A.JoinedThreads.count(B.ThreadId)) return true;
    if (B.JoinedThreads.count(A.ThreadId)) return true;
    return false;
}

static bool AnyPairHasJoinPrecedence(const std::vector<LockPair> &Cycle) {
    for (size_t i = 0; i < Cycle.size(); i++) {
        for (size_t j = i + 1; j < Cycle.size(); j++) {
            if (HasJoinPrecedence(Cycle[i], Cycle[j])) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::vector<LockPair>> FindCycles(const std::vector<LockPair> &Pairs) {
    // Dedup SAMO na osnovu potpunog poklapanja (From, To, ContextLocks I
    // MustContextLocks) - ne spajamo (presecamo) Must vrednosti razlicitih
    // zapisa, jer bi to mesalo kontekste iz nepovezanih putanja izvrsavanja
    // (npr. razlicite grane rekurzije koje slucajno daju isti May par).
    std::set<LockPair> DedupSet;
    for (const LockPair &P : Pairs) {
        DedupSet.insert(P);
    }
    std::vector<LockPair> Deduped(DedupSet.begin(), DedupSet.end());

    std::map<std::string, std::vector<LockPair>> Graph;
    for (const LockPair &P : Deduped) {
        Graph[P.From].push_back(P);
    }

    std::set<std::string> Visited;
    std::set<std::string> InProgress;
    std::vector<LockPair> CurrentPath;
    std::vector<std::vector<LockPair>> AllCycles;

    for (const auto &Entry : Graph) {
        const std::string &Node = Entry.first;
        if (!Visited.count(Node)) {
            DFS(Node, Graph, Visited, InProgress, CurrentPath, AllCycles);
        }
    }

    std::vector<std::vector<LockPair>> RealCycles;
    for (const auto &Cycle : AllCycles) {
        if (AllSameThread(Cycle)) {
            continue;  // lazan alarm - ista nit, ne moze biti pravi deadlock
        }

        if (!HasCommonLock(Cycle) && AllNodesCanConflict(Cycle) && !AnyPairHasJoinPrecedence(Cycle)) {
            RealCycles.push_back(Cycle);
        }
    }

    return RealCycles;
}