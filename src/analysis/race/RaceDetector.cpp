#include "RaceDetector.h"
#include <algorithm>
#include <iterator>

// Ista provera kao HasJoinPrecedence u CycleDetector.cpp, samo za
// MemoryAccess umesto LockPair.
static bool HasJoinPrecedence(const MemoryAccess &A, const MemoryAccess &B) {
    if (A.JoinedThreads.count(B.ThreadId) && !B.CreatedInLoop) return true;
    if (B.JoinedThreads.count(A.ThreadId) && !A.CreatedInLoop) return true;
    return false;
}

// MustConcurrent(A,B): SIGURNO su bile obe aktivne u isto vreme. Mora vaziti
// u OBA pravca (RacerF Sec 6.2: "...and vice versa") - asimetrija u samo
// jednom pravcu nije dovoljna da tvrdimo punu sigurnost.
static bool IsMustConcurrent(const MemoryAccess &A, const MemoryAccess &B) {
    return A.MustActiveThreads.count(B.ThreadId) > 0 &&
           B.MustActiveThreads.count(A.ThreadId) > 0;
}

// MayConcurrent(A,B): MOZDA su bile obe aktivne u isto vreme - isto tako
// simetricno, samo sa sirim (May) skupovima.
static bool IsMayConcurrent(const MemoryAccess &A, const MemoryAccess &B) {
    return A.MayActiveThreads.count(B.ThreadId) > 0 &&
           B.MayActiveThreads.count(A.ThreadId) > 0;
}

static bool IsMustProtected(const MemoryAccess &A, const MemoryAccess &B) {
    for (const auto &EntryA : A.MustLockset) {
        auto ItB = B.MustLockset.find(EntryA.first);
        if (ItB == B.MustLockset.end()) continue;
        if (EntryA.second == LockKind::Write || ItB->second == LockKind::Write) {
            return true;
        }
    }
    return false;
}

static bool IsMayProtected(const MemoryAccess &A, const MemoryAccess &B) {
    for (const auto &EntryA : A.MayLockset) {
        auto ItB = B.MayLockset.find(EntryA.first);
        if (ItB == B.MayLockset.end()) continue;
        if (EntryA.second == LockKind::Write || ItB->second == LockKind::Write) {
            return true;
        }
    }
    return false;
}

std::vector<RaceReport> FindRaces(const std::vector<MemoryAccess> &Accesses) {
    std::vector<RaceReport> Races;

    for (size_t i = 0; i < Accesses.size(); i++) {
        for (size_t j = i + 1; j < Accesses.size(); j++) {
            const MemoryAccess &A = Accesses[i];
            const MemoryAccess &B = Accesses[j];

            if (A.VarName != B.VarName) continue;
            if (A.ThreadId == B.ThreadId) continue;
            if (!A.IsWrite && !B.IsWrite) continue;
            if (HasJoinPrecedence(A, B)) continue;

            // NE MayProtected je zajednicka "kapija" za oba nivoa - ako
            // POSTOJI bilo koja putanja sa zajednickom bravom, ne mozemo
            // tvrditi ni MustRace ni MayRace (videti diskusiju: MustProtected
            // povlaci MayProtected, pa je provera na MayProtected dovoljna
            // i za oba slucaja).
            if (IsMayProtected(A, B)) continue;

            if (IsMustConcurrent(A, B)) {
                Races.push_back({A, B, RaceSeverity::MustRace});
            } else if (IsMayConcurrent(A, B)) {
                Races.push_back({A, B, RaceSeverity::MayRace});
            }
            // Ako ni MustConcurrent ni MayConcurrent - sigurno nisu mogli
            // biti konkurentni, nema race-a bez obzira na zastitu.
        }
    }

    return Races;
}