#ifndef MEMORYACCESS_H
#define MEMORYACCESS_H

#include "../common/LockKind.h"
#include <map>
#include <set>
#include <string>

// Jedan zapis "nit ThreadId je [Read/Write] pristupila promenljivoj
// VarName, dok je drzala lockset Locks". Analogno LockPair-u kod
// deadlocka, samo sto ovde nije ivica u grafu vec pojedinacni dogadjaj -
// RaceDetector kasnije uporedjuje SVAKI PAR dogadjaja (razlicite niti,
// bar jedan Write, prazan presek lockset-a, bez join precedence).
struct MemoryAccess {
    std::string VarName;
    bool IsWrite = false;
    std::map<std::string, LockKind> Lockset;   // State.Must u trenutku pristupa
    std::set<std::string> JoinedThreads;
    bool CreatedInLoop = false;
    std::string ThreadId;
    unsigned Line = 0;
};

#endif
