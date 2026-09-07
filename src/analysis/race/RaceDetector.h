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

// Vraca sve parove MemoryAccess zapisa koji predstavljaju potencijalni race,
// klasifikovane po ozbiljnosti (MustRace = sigurno konkurentni i sigurno
// nezasticeni; MayRace = neizvesno u bar jednoj od te dve dimenzije).
std::vector<RaceReport> FindRaces(const std::vector<MemoryAccess> &Accesses);

#endif