#ifndef MEMORYACCESS_H
#define MEMORYACCESS_H

#include "../common/LockKind.h"
#include <map>
#include <set>
#include <string>

struct MemoryAccess {
    std::string VarName;
    bool IsWrite = false;
    std::map<std::string, LockKind> MustLockset;   // State.Must u trenutku pristupa (preimenovano iz Lockset)
    std::map<std::string, LockKind> MayLockset;     // NOVO - State.May u trenutku pristupa
    std::set<std::string> JoinedThreads;
    bool CreatedInLoop = false;
    std::string ThreadId;
    unsigned Line = 0;
    std::set<std::string> MustActiveThreads;
    std::set<std::string> MayActiveThreads;
};

#endif