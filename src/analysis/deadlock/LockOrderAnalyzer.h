#ifndef LOCKORDERANALYZER_H
#define LOCKORDERANALYZER_H

#include "../common/LockKind.h"
#include <clang/AST/Decl.h>
#include <clang/AST/ASTContext.h>
#include <vector>
#include <string>
#include <set>
#include <map>

using namespace clang;

// Predstavlja jednu ivicu u grafu redosleda brava: "dok se drzi From,
// akvirira se To". Deadlock-specificno - race analiza ovo ne koristi.
struct LockPair {
    std::string From;
    std::string To;
    LockKind FromKind = LockKind::Write;
    LockKind ToKind = LockKind::Write;
    std::set<std::string> ContextLocks;
    std::set<std::string> MustContextLocks;
    std::map<std::string, LockKind> MustContextKinds;
    std::set<std::string> JoinedThreads;
    bool CreatedInLoop = false;
    std::set<std::string> KnownThreadsAtThisPoint;   // NOVO
    std::string ThreadId;

    bool operator<(const LockPair &Other) const {
        if (From != Other.From) return From < Other.From;
        if (To != Other.To) return To < Other.To;
        if (FromKind != Other.FromKind) return FromKind < Other.FromKind;
        if (ToKind != Other.ToKind) return ToKind < Other.ToKind;
        if (ContextLocks != Other.ContextLocks) return ContextLocks < Other.ContextLocks;
        if (MustContextLocks != Other.MustContextLocks) return MustContextLocks < Other.MustContextLocks;
        if (MustContextKinds != Other.MustContextKinds) return MustContextKinds < Other.MustContextKinds;
        if (JoinedThreads != Other.JoinedThreads) return JoinedThreads < Other.JoinedThreads;
        if (CreatedInLoop != Other.CreatedInLoop) return CreatedInLoop < Other.CreatedInLoop;
        if (KnownThreadsAtThisPoint != Other.KnownThreadsAtThisPoint) return KnownThreadsAtThisPoint < Other.KnownThreadsAtThisPoint;
        return ThreadId < Other.ThreadId;
    }
};

std::vector<LockPair> FindLockOrderPairs(FunctionDecl *FD, ASTContext &Context,
                                          std::set<std::string> &CreatedInLoop);

#endif
