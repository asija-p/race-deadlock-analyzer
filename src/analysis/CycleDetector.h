#ifndef CYCLEDETECTOR_H
#define CYCLEDETECTOR_H

#include "LockOrderAnalyzer.h"
#include <vector>
#include <string>

// Sad vraca listu ciklusa, gde je svaki ciklus lista LockPair zapisa (ne stringova)
std::vector<std::vector<LockPair>> FindCycles(const std::vector<LockPair> &Pairs);

#endif