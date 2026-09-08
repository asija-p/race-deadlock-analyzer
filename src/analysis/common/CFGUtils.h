#ifndef CFGUTILS_H
#define CFGUTILS_H

#include <clang/Analysis/CFG.h>

using namespace clang;

bool IsBlockInLoop(const CFGBlock *Start);

#endif
