#include "CFGUtils.h"
#include <set>
#include <vector>

bool IsBlockInLoop(const CFGBlock *Start) {
    std::set<const CFGBlock*> Visited;
    std::vector<const CFGBlock*> Worklist;
    for (const CFGBlock::AdjacentBlock &Succ : Start->succs()) {
        if (Succ.isReachable()) {
            Worklist.push_back(Succ.getReachableBlock());
        }
    }
    while (!Worklist.empty()) {
        const CFGBlock *B = Worklist.back();
        Worklist.pop_back();
        if (B == Start) return true;
        if (Visited.count(B)) continue;
        Visited.insert(B);
        for (const CFGBlock::AdjacentBlock &Succ : B->succs()) {
            if (Succ.isReachable()) {
                Worklist.push_back(Succ.getReachableBlock());
            }
        }
    }
    return false;
}
