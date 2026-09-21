#include "RaceDetector.h"
#include "../common/ConcurrencyExclusion.h"
#include <algorithm>
#include <iterator>
#include <string>
#include <map>
#include <string>

static bool VarNamesMayAlias(const std::string &A, const std::string &B) {
    if (A == B) return true;
    size_t PosA = A.find('[');
    size_t PosB = B.find('[');
    if (PosA == std::string::npos || PosB == std::string::npos) return false;
    std::string BaseA = A.substr(0, PosA);
    std::string BaseB = B.substr(0, PosB);
    if (BaseA != BaseB || BaseA == "?") return false;
    return A.substr(PosA) == "[?]" || B.substr(PosB) == "[?]";
}

static bool HasUnknownIndex(const std::string &Name) {
    return Name.find("[?]") != std::string::npos;
}

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

static std::string AccessKey(const MemoryAccess &M) {
    return M.VarName + "|" + std::to_string(M.Line) + "|" + M.ThreadId;
}

std::vector<RaceReport> FindRaces(const std::vector<MemoryAccess> &Accesses) {
    std::vector<RaceReport> Races;
        std::map<std::string, size_t> Seen;   // par (bez obzira na redosled) -> indeks u Races

    for (size_t i = 0; i < Accesses.size(); i++) {
        for (size_t j = i; j < Accesses.size(); j++) {
            const MemoryAccess &A = Accesses[i];
            const MemoryAccess &B = Accesses[j];

            if (!VarNamesMayAlias(A.VarName, B.VarName)) continue;
            if (!A.IsWrite && !B.IsWrite) continue;

            const bool SameThread = (A.ThreadId == B.ThreadId);
            if (SameThread && !A.CreatedInLoop) continue;

            if (IsMustProtected(A, B)) continue;
            if (!IsMayConcurrent(A, B)) continue;

            RaceSeverity Severity;
            if (HasUnknownIndex(A.VarName) || HasUnknownIndex(B.VarName)) {
                Severity = RaceSeverity::MayRace;
            } else if (SameThread) {
                Severity = RaceSeverity::MayRace;
            } else {
                bool DefinitelyConcurrent = IsMustConcurrent(A, B);
                bool DefinitelyUnprotected = !IsMayProtected(A, B);
                Severity = (DefinitelyConcurrent && DefinitelyUnprotected)
                               ? RaceSeverity::MustRace
                               : RaceSeverity::MayRace;
            }
            std::string KeyA = AccessKey(A);
            std::string KeyB = AccessKey(B);
            std::string Key = (KeyA < KeyB) ? KeyA + "#" + KeyB : KeyB + "#" + KeyA;
            auto Found = Seen.find(Key);
            if (Found != Seen.end()) {
                // Isti par vec prijavljen: zadrzi jaci nalaz.
                if (Severity == RaceSeverity::MustRace) {
                    Races[Found->second].Severity = RaceSeverity::MustRace;
                }
                continue;
            }
            Seen[Key] = Races.size();
            Races.push_back({A, B, Severity});
        }
    }

    return Races;
}