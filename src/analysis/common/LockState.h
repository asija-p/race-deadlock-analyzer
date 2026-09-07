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
//
//  - ThreadHandles/JoinedThreads: happens-before knjigovodstvo izgradjeno
//    iz pthread_create/pthread_join poziva.
//
//  - MustActiveThreads/MayActiveThreads: koje niti su SIGURNO (Must) odnosno
//    MOZDA (May) trenutno aktivne "zajedno sa mnom", iz MOJE perspektive.
//    Dete NASLEDJUJE oba skupa od roditelja u trenutku pthread_create (plus
//    roditeljev ThreadId ulazi u oba), i NIKAD ih sam ne invalidira - dete
//    zna da je roditelj aktivan tokom CELOG svog zivota, jer pthread_join
//    na strani roditelja fizicki ne moze da se zavrsi pre nego sto dete
//    zavrsi (vidi diskusiju uz ovaj commit). Invalidacija (uklanjanje
//    deteta iz ovih skupova) se desava SAMO na strani roditelja, pri
//    pthread_join, analogno unlock-u za brave.
struct LockState {
    std::map<std::string, LockKind> May;
    std::map<std::string, LockKind> Must;
    std::map<std::string, std::string> ThreadHandles;  // "tid" ime -> ThreadId
    std::set<std::string> JoinedThreads;                // koje niti su SIGURNO gotove
    std::set<std::string> MustActiveThreads;            // NOVO
    std::set<std::string> MayActiveThreads;             // NOVO

    bool operator==(const LockState &Other) const {
        return May == Other.May && Must == Other.Must &&
               ThreadHandles == Other.ThreadHandles && JoinedThreads == Other.JoinedThreads &&
               MustActiveThreads == Other.MustActiveThreads &&
               MayActiveThreads == Other.MayActiveThreads;
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

// Unija skupova aktivnih niti na spoju grana (May-stil - MOZDA aktivan ako
// je aktivan na BAR JEDNOJ grani).
std::set<std::string> UnionActiveThreads(
    const std::set<std::string> &A, const std::set<std::string> &B);

// Presek skupova aktivnih niti na spoju grana (Must-stil - SIGURNO aktivan
// samo ako je aktivan na SVIM granama).
std::set<std::string> IntersectActiveThreads(
    const std::set<std::string> &A, const std::set<std::string> &B);

#endif