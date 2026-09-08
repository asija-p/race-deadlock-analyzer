#include "CycleDetector.h"
#include "../common/ConcurrencyExclusion.h"
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


static bool AllSameThread(const std::vector<LockPair> &Cycle) {
    if (Cycle.empty()) return false;
    const std::string &FirstThread = Cycle[0].ThreadId;
    for (const auto &Edge : Cycle) {
        if (Edge.ThreadId != FirstThread) {
            return false;
        }
    }
    if (Cycle[0].CreatedInLoop) {
        return false;
    }
    return true;
}


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
            if (ProtectingLockExists(Cycle[i], Cycle[j])) {
                return true;
            }
        }
    }
    return false;
}


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


static bool AnyPairIsDefinitelyExcluded(const std::vector<LockPair> &Cycle) {
    for (size_t i = 0; i < Cycle.size(); i++) {
        for (size_t j = i + 1; j < Cycle.size(); j++) {
            if (IsDefinitelyExcluded(Cycle[i], Cycle[j])) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::vector<LockPair>> FindCycles(const std::vector<LockPair> &Pairs) {
    // Dedup SAMO na osnovu potpunog poklapanja - ne spajamo (presecamo)
    // Must vrednosti razlicitih zapisa, jer bi to mesalo kontekste iz
    // nepovezanih putanja izvrsavanja.
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
            continue;
        }

        if (!HasCommonLock(Cycle) && AllNodesCanConflict(Cycle) && !AnyPairIsDefinitelyExcluded(Cycle)) {
            RealCycles.push_back(Cycle);
        }
    }

    return RealCycles;
}