#include "RaceDetector.h"
#include "../common/ConcurrencyExclusion.h"
#include <algorithm>
#include <iterator>

//  RacerF still
static bool IsMayConcurrent(const MemoryAccess &A, const MemoryAccess &B) {
    return !IsDefinitelyExcluded(A, B);
}

// MustConcurrent(A,B): RacerF stil - simetricna provera preko MustActiveThreads
// (presek na granama grananja, ne unija). 
static bool IsMustConcurrent(const MemoryAccess &A, const MemoryAccess &B) {
    if (!IsMayConcurrent(A, B)) return false;

    bool AKnowsB = A.MustActiveThreads.count(B.ThreadId) > 0;
    bool BKnowsA = B.MustActiveThreads.count(A.ThreadId) > 0;

    bool AIsRoot = A.ThreadId.rfind("create_line_", 0) != 0;
    bool BIsRoot = B.ThreadId.rfind("create_line_", 0) != 0;

    if (!AIsRoot && !BIsRoot) {

        return AKnowsB || BKnowsA;
    }

    return AKnowsB && BKnowsA;
}

// MustProtected(A,B): dele bravu koju OBOJE SIGURNO drze.
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

// MayProtected(A,B): dele bravu koju OBOJE MOZDA drze.
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

            if (IsMustProtected(A, B)) continue;
            if (!IsMayConcurrent(A, B)) continue;

            bool DefinitelyConcurrent = IsMustConcurrent(A, B);
            bool DefinitelyUnprotected = !IsMayProtected(A, B);
            RaceSeverity Severity = (DefinitelyConcurrent && DefinitelyUnprotected)
                                         ? RaceSeverity::MustRace
                                         : RaceSeverity::MayRace;
            Races.push_back({A, B, Severity});
        }
    }

    return Races;
}