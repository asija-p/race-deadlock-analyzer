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

// Azurira State.May/State.Must u skladu sa Kind (lock dodaje MutexName sa
// odgovarajucim LockKind-om, unlock ga uklanja). Poziva se POSLE eventualnog
// generisanja LockPair-a (koji mora da vidi STARO stanje, pre ove izmene).
void ApplyLockCallToState(LockCallKind Kind, const std::string &MutexName, LockState &State);

#endif