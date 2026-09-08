#ifndef LOCKSTATE_H
#define LOCKSTATE_H

#include "LockKind.h"
#include <map>
#include <set>
#include <string>

struct LockState {
    std::map<std::string, LockKind> May;
    std::map<std::string, LockKind> Must;
    std::map<std::string, std::string> ThreadHandles;
    std::set<std::string> JoinedThreads;
    std::set<std::string> MustActiveThreads;
    std::set<std::string> MayActiveThreads;
    std::set<std::string> KnownThreads;

    bool operator==(const LockState &Other) const {
        return May == Other.May && Must == Other.Must &&
               ThreadHandles == Other.ThreadHandles && JoinedThreads == Other.JoinedThreads &&
               MustActiveThreads == Other.MustActiveThreads &&
               MayActiveThreads == Other.MayActiveThreads &&
               KnownThreads == Other.KnownThreads;
    }
    bool operator!=(const LockState &Other) const {
        return !(*this == Other);
    }
};

std::set<std::string> KeysOf(const std::map<std::string, LockKind> &M);

void MergeMayInto(std::map<std::string, LockKind> &Target,
                   const std::map<std::string, LockKind> &Source);

std::map<std::string, LockKind> IntersectMust(
    const std::map<std::string, LockKind> &A,
    const std::map<std::string, LockKind> &B);

std::set<std::string> IntersectJoinedThreads(
    const std::set<std::string> &A, const std::set<std::string> &B);

void MergeThreadHandlesInto(std::map<std::string, std::string> &Target,
                             const std::map<std::string, std::string> &Source);

std::set<std::string> UnionActiveThreads(
    const std::set<std::string> &A, const std::set<std::string> &B);

std::set<std::string> IntersectActiveThreads(
    const std::set<std::string> &A, const std::set<std::string> &B);

#endif