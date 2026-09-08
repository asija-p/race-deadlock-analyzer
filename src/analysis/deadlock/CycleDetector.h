#ifndef CYCLEDETECTOR_H
#define CYCLEDETECTOR_H

#include "LockOrderAnalyzer.h"
#include <vector>
#include <string>

// Vraca listu ciklusa, gde je svaki ciklus lista LockPair zapisa (ne stringova)
std::vector<std::vector<LockPair>> FindCycles(const std::vector<LockPair> &Pairs);

#endif