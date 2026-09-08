#ifndef LOCKRECOGNITION_H
#define LOCKRECOGNITION_H

#include "LockState.h"
#include <string>

// Klasifikuje da li je FuncName neki od podrzanih pthread lock/unlock
// poziva (mutex, spinlock, rwlock). Deljeno izmedju deadlock i race
// analize - JEDINO mesto gde se ova lista imena odrzava, da se ne bi
// dve kopije razisle kad se doda novi podrzan poziv.
enum class LockCallKind {
    NotALock,
    WriteLock,
    ReadLock,
    Unlock
};

LockCallKind ClassifyLockCall(const std::string &FuncName);

void ApplyLockCallToState(LockCallKind Kind, const std::string &MutexName, LockState &State);

#endif