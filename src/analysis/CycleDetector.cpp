#include "CycleDetector.h"
#include <map>
#include <set>
#include <iostream>

static void DFS(
    const std::string &Node,
    const std::map<std::string, std::vector<std::string>> &Graph,
    std::set<std::string> &Visited,
    std::set<std::string> &InProgress,
    std::vector<std::string> &CurrentPath,
    std::vector<std::vector<std::string>> &Cycles) {

    Visited.insert(Node);
    InProgress.insert(Node);
    CurrentPath.push_back(Node);

    auto It = Graph.find(Node);
    if (It != Graph.end()) {
        for (const std::string &Neighbor : It->second) {
            if (InProgress.count(Neighbor)) {
                // CIKLUS! Napravi put od Neighbor-a do kraja CurrentPath, pa nazad na Neighbor
                std::vector<std::string> Cycle;
                bool Started = false;
                for (const std::string &N : CurrentPath) {
                    if (N == Neighbor) Started = true;
                    if (Started) Cycle.push_back(N);
                }
                Cycle.push_back(Neighbor); // zatvori ciklus
                Cycles.push_back(Cycle);
            } else if (!Visited.count(Neighbor)) {
                DFS(Neighbor, Graph, Visited, InProgress, CurrentPath, Cycles);
            }
        }
    }

    CurrentPath.pop_back();
    InProgress.erase(Node);
}

std::vector<std::vector<std::string>> FindCycles(const std::vector<LockPair> &Pairs) {
    std::map<std::string, std::set<std::string>> GraphSet;  // set umesto vector, privremeno
    for (const LockPair &P : Pairs) {
        GraphSet[P.first].insert(P.second);
    }

    // konvertuj nazad u vector<string> za DFS (da ne moramo menjati potpis DFS-a)
    std::map<std::string, std::vector<std::string>> Graph;
    for (const auto &Entry : GraphSet) {
        Graph[Entry.first] = std::vector<std::string>(Entry.second.begin(), Entry.second.end());
    }

    std::set<std::string> Visited;
    std::set<std::string> InProgress;
    std::vector<std::string> CurrentPath;
    std::vector<std::vector<std::string>> Cycles;

    for (const auto &Entry : Graph) {
        const std::string &Node = Entry.first;
        if (!Visited.count(Node)) {
            DFS(Node, Graph, Visited, InProgress, CurrentPath, Cycles);
        }
    }

    return Cycles;
}