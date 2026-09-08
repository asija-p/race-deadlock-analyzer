#include "LockState.h"
#include <algorithm>
#include <iterator>

std::set<std::string> KeysOf(const std::map<std::string, LockKind> &M) {
    std::set<std::string> Keys;
    for (const auto &Entry : M) {
        Keys.insert(Entry.first);
    }
    return Keys;
}

void MergeMayInto(std::map<std::string, LockKind> &Target,
                   const std::map<std::string, LockKind> &Source) {
    for (const auto &Entry : Source) {
        auto It = Target.find(Entry.first);
        if (It == Target.end()) {
            Target[Entry.first] = Entry.second;
        } else if (It->second != Entry.second) {
            It->second = LockKind::Write;
        }
    }
}

std::map<std::string, LockKind> IntersectMust(
    const std::map<std::string, LockKind> &A,
    const std::map<std::string, LockKind> &B) {
    std::map<std::string, LockKind> Result;
    for (const auto &Entry : A) {
        auto It = B.find(Entry.first);
        if (It != B.end()) {
            Result[Entry.first] = (Entry.second == It->second) ? Entry.second : LockKind::Read;
        }
    }
    return Result;
}

std::set<std::string> IntersectJoinedThreads(
    const std::set<std::string> &A, const std::set<std::string> &B) {
    std::set<std::string> Result;
    std::set_intersection(A.begin(), A.end(), B.begin(), B.end(),
                           std::inserter(Result, Result.begin()));
    return Result;
}

void MergeThreadHandlesInto(std::map<std::string, std::string> &Target,
                             const std::map<std::string, std::string> &Source) {
    for (const auto &Entry : Source) {
        auto It = Target.find(Entry.first);
        if (It == Target.end()) {
            Target[Entry.first] = Entry.second;
        } else if (It->second != Entry.second) {

            Target.erase(It);
        }
    }
}

std::set<std::string> UnionActiveThreads(
    const std::set<std::string> &A, const std::set<std::string> &B) {
    std::set<std::string> Result = A;
    Result.insert(B.begin(), B.end());
    return Result;
}

std::set<std::string> IntersectActiveThreads(
    const std::set<std::string> &A, const std::set<std::string> &B) {
    std::set<std::string> Result;
    std::set_intersection(A.begin(), A.end(), B.begin(), B.end(),
                           std::inserter(Result, Result.begin()));
    return Result;
}