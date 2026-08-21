#include "CycleDetector.h"
#include <map>
#include <set>
#include <iostream>

static bool DFS(
    const std::string &Node,
    const std::map<std::string, std::vector<std::string>> &Graph,
    std::set<std::string> &Visited,
    std::set<std::string> &InProgress,
    std::vector<std::string> &CurrentPath,
    std::vector<std::vector<std::string>> &Cycles) {

    Visited.insert(Node);
    InProgress.insert(Node);
    CurrentPath.push_back(Node);

    // Graph.at(Node) moze da baci gresku ako Node nema susede - proveravamo prvo
    auto It = Graph.find(Node);
    if (It != Graph.end()) {
        for (const std::string &Neighbor : It->second) {
            if (InProgress.count(Neighbor)) {
                // CIKLUS! Napravi listu koja pokazuje ciklus
                std::vector<std::string> Cycle;
                Cycle.push_back(Neighbor);
                Cycles.push_back(Cycle);
            } else if (!Visited.count(Neighbor)) {
                DFS(Neighbor, Graph, Visited, InProgress, CurrentPath, Cycles);
            }
        }
    }

    CurrentPath.pop_back();
    InProgress.erase(Node);
    return false;
}

std::vector<std::vector<std::string>> FindCycles(const std::vector<LockPair> &Pairs) {
    // Gradimo graf iz parova
    std::map<std::string, std::vector<std::string>> Graph;
    for (const LockPair &P : Pairs) {
        Graph[P.first].push_back(P.second);
    }

    std::set<std::string> Visited;
    std::set<std::string> InProgress;
    std::vector<std::string> CurrentPath;
    std::vector<std::vector<std::string>> Cycles;

    // Pokrecemo DFS iz SVAKOG cvora (ne samo jednog) -
    // graf moze imati vise nepovezanih delova
    for (const auto &Entry : Graph) {
        const std::string &Node = Entry.first;
        if (!Visited.count(Node)) {
            DFS(Node, Graph, Visited, InProgress, CurrentPath, Cycles);
        }
    }

    return Cycles;
}