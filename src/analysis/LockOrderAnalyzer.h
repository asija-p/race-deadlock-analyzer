#ifndef LOCKORDERANALYZER_H
#define LOCKORDERANALYZER_H

#include <clang/AST/Decl.h>
#include <clang/AST/ASTContext.h>
#include <vector>
#include <string>
#include <set>
#include <map>

using namespace clang;

// Mod u kom je brava drzana/akvirirana.
// Write = ekskluzivno (mutex, spinlock, rwlock wrlock).
// Read  = deljeno (rwlock rdlock) - dva Read-a se NE sudaraju medjusobno.
enum class LockKind {
    Read,
    Write
};

// Umesto std::pair<string,string>, sad cuvamo i CEO lockset u trenutku nastanka.
// ContextLocks     = MAY-lockset (sta je MOGLO biti zakljucano, union na granama)
// MustContextLocks = MUST-lockset (sta je SIGURNO bilo zakljucano, presek na granama)
struct LockPair {
    std::string From;
    std::string To;
    LockKind FromKind = LockKind::Write;
    LockKind ToKind = LockKind::Write;
    std::set<std::string> ContextLocks;
    std::set<std::string> MustContextLocks;
    std::map<std::string, LockKind> MustContextKinds;
    std::set<std::string> JoinedThreads;   // NOVO - koje niti su SIGURNO vec join-ovane
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
        return ThreadId < Other.ThreadId;
    }
};

std::vector<LockPair> FindLockOrderPairs(FunctionDecl *FD, ASTContext &Context);

#endif