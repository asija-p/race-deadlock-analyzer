#ifndef LOCKSTATE_H
#define LOCKSTATE_H

#include "LockKind.h"
#include <map>
#include <set>
#include <string>

// Stanje koje InterproceduralWalker nosi kroz CFG fixpoint i kroz
// interproceduralne pozive. Deljeno izmedju deadlock i race analize:
//
//  - May/Must: trenutni skup brava koje se "mogu" (May, unija preko grana)
//    odnosno "moraju" (Must, presek preko grana) drzati u datoj tacki.
//    Deadlocku treba May da uporedi "prethodno drzane" brave pri svakom
//    novom lock() pozivu; race analizi (Eraser-style lockset algoritam)
//    treba Must kao "lockset" koji vazi za svaki memory access.
//
//  - ThreadHandles/JoinedThreads: happens-before knjigovodstvo izgradjeno
//    iz pthread_create/pthread_join poziva. Generic je za obe analize -
//    obema treba da iskljuce parove (lock-ivica ili memory access) koji su
//    stvarno sekvencijalno poredjani preko fork/join, a ne stvarno konkurentni.
struct LockState {
    std::map<std::string, LockKind> May;
    std::map<std::string, LockKind> Must;
    std::map<std::string, std::string> ThreadHandles;  // "tid" ime -> ThreadId
    std::set<std::string> JoinedThreads;                // koje niti su SIGURNO gotove

    bool operator==(const LockState &Other) const {
        return May == Other.May && Must == Other.Must &&
               ThreadHandles == Other.ThreadHandles && JoinedThreads == Other.JoinedThreads;
    }
    bool operator!=(const LockState &Other) const {
        return !(*this == Other);
    }
};

// Vraca skup imena kljuceva mape (npr. imena trenutno drzanih brava).
std::set<std::string> KeysOf(const std::map<std::string, LockKind> &M);

// Merge na spoju CFG grana za May: unija po kljucu, a ako se dva ulaza
// razlikuju po LockKind-u (Read na jednoj grani, Write na drugoj), rezultat
// je konzervativno Write.
void MergeMayInto(std::map<std::string, LockKind> &Target,
                   const std::map<std::string, LockKind> &Source);

// Merge na spoju CFG grana za Must: presek po kljucu (brava mora biti
// drzana na SVIM granama da bi ostala u Must), a ako se LockKind razlikuje,
// rezultat je konzervativno Read (ne moze se garantovati Write na obe grane).
std::map<std::string, LockKind> IntersectMust(
    const std::map<std::string, LockKind> &A,
    const std::map<std::string, LockKind> &B);

// Presek zavrsenih (joined) niti na spoju grana - nit je "sigurno gotova"
// samo ako je to tacno na SVIM putanjama koje vode do te tacke.
std::set<std::string> IntersectJoinedThreads(
    const std::set<std::string> &A, const std::set<std::string> &B);

// Unija mape thread-handle-ova na spoju grana.
void MergeThreadHandlesInto(std::map<std::string, std::string> &Target,
                             const std::map<std::string, std::string> &Source);

#endif
