#ifndef CYCLEDETECTOR_H
#define CYCLEDETECTOR_H

#include "LockOrderAnalyzer.h"
#include <vector>
#include <string>

// Vraca listu ciklusa koje pronadje (svaki ciklus je lista imena mutexa po redu)
std::vector<std::vector<std::string>> FindCycles(const std::vector<LockPair> &Pairs);

#endif