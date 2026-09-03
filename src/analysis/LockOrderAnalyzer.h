#ifndef LOCKORDERANALYZER_H
#define LOCKORDERANALYZER_H

#include <clang/AST/Decl.h>
#include <clang/AST/ASTContext.h>
#include <vector>
#include <string>
#include <set>

using namespace clang;

// Umesto std::pair<string,string>, sad cuvamo i CEO lockset u trenutku nastanka.
// ContextLocks     = MAY-lockset (sta je MOGLO biti zakljucano, union na granama)
// MustContextLocks = MUST-lockset (sta je SIGURNO bilo zakljucano, presek na granama)
struct LockPair {
    std::string From;
    std::string To;
    std::set<std::string> ContextLocks;
    std::set<std::string> MustContextLocks;

    // Potrebno da bi LockPair mogao da ide u std::set (za dedup)
    bool operator<(const LockPair &Other) const {
        if (From != Other.From) return From < Other.From;
        if (To != Other.To) return To < Other.To;
        return ContextLocks < Other.ContextLocks;
    }
};

std::vector<LockPair> FindLockOrderPairs(FunctionDecl *FD, ASTContext &Context);

#endif