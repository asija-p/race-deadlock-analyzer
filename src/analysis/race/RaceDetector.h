#ifndef RACEDETECTOR_H
#define RACEDETECTOR_H

#include "MemoryAccess.h"
#include <vector>

enum class RaceSeverity {
    MustRace,
    MayRace
};

struct RaceReport {
    MemoryAccess A;
    MemoryAccess B;
    RaceSeverity Severity;
};

std::vector<RaceReport> FindRaces(const std::vector<MemoryAccess> &Accesses);

#endif