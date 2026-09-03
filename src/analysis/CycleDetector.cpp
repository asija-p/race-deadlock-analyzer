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
                // CIKLUS - sakupi pun put kao LockPair zapise
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

// Proverava da li SVI parovi ivica u ciklusu dele bar jedan zajednicki lock.
// Ako da - ciklus je lazan alarm (zasticen common lock-om).
static bool HasCommonLock(const std::vector<LockPair> &Cycle) {
    for (size_t i = 0; i < Cycle.size(); i++) {
        for (size_t j = i + 1; j < Cycle.size(); j++) {
            std::set<std::string> Intersection;
            std::set_intersection(
                Cycle[i].MustContextLocks.begin(), Cycle[i].MustContextLocks.end(),
                Cycle[j].MustContextLocks.begin(), Cycle[j].MustContextLocks.end(),
                std::inserter(Intersection, Intersection.begin()));

            if (Intersection.empty()) {
                return false;
            }
        }
    }
    return true;
}

std::vector<std::vector<LockPair>> FindCycles(const std::vector<LockPair> &Pairs) {
    std::map<std::string, std::set<LockPair>> GraphSet;
    for (const LockPair &P : Pairs) {
        GraphSet[P.From].insert(P);
    }

    std::map<std::string, std::vector<LockPair>> Graph;
    for (const auto &Entry : GraphSet) {
        Graph[Entry.first] = std::vector<LockPair>(Entry.second.begin(), Entry.second.end());
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

    // Filtriraj - izbaci cikluse koji su zasticeni zajednickim lockom
    std::vector<std::vector<LockPair>> RealCycles;
    for (const auto &Cycle : AllCycles) {
        if (!HasCommonLock(Cycle)) {
            RealCycles.push_back(Cycle);
        }
    }

    return RealCycles;
}