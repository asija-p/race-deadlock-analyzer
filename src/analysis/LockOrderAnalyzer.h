#ifndef LOCKORDERANALYZER_H
#define LOCKORDERANALYZER_H

#include <clang/AST/Decl.h>
#include <clang/AST/ASTContext.h>
#include <vector>
#include <string>
#include <set>
#include <map>

using namespace clang;

enum class LockKind {
    Read,
    Write
};

struct LockPair {
    std::string From;
    std::string To;
    LockKind FromKind = LockKind::Write;
    LockKind ToKind = LockKind::Write;
    std::set<std::string> ContextLocks;
    std::set<std::string> MustContextLocks;
    std::map<std::string, LockKind> MustContextKinds;
    std::set<std::string> JoinedThreads;
    bool CreatedInLoop = false;   // NOVO - da li ThreadId ove ivice moze predstavljati VISE niti
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
        return ThreadId < Other.ThreadId;
    }
};

std::vector<LockPair> FindLockOrderPairs(FunctionDecl *FD, ASTContext &Context,
                                          std::set<std::string> &CreatedInLoop);

#endif