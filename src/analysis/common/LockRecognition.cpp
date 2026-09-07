#include "LockRecognition.h"

LockCallKind ClassifyLockCall(const std::string &FuncName) {
    bool IsWriteLock = (FuncName == "pthread_mutex_lock" || FuncName == "pthread_mutex_trylock" ||
                         FuncName == "pthread_spin_lock" || FuncName == "pthread_spin_trylock" ||
                         FuncName == "pthread_rwlock_wrlock" || FuncName == "pthread_rwlock_trywrlock");
    if (IsWriteLock) return LockCallKind::WriteLock;

    bool IsReadLock = (FuncName == "pthread_rwlock_rdlock" || FuncName == "pthread_rwlock_tryrdlock");
    if (IsReadLock) return LockCallKind::ReadLock;

    bool IsUnlock = (FuncName == "pthread_mutex_unlock" ||
                      FuncName == "pthread_spin_unlock" ||
                      FuncName == "pthread_rwlock_unlock");
    if (IsUnlock) return LockCallKind::Unlock;

    return LockCallKind::NotALock;
}

void ApplyLockCallToState(LockCallKind Kind, const std::string &MutexName, LockState &State) {
    if (Kind == LockCallKind::WriteLock) {
        State.May[MutexName] = LockKind::Write;
        State.Must[MutexName] = LockKind::Write;
    } else if (Kind == LockCallKind::ReadLock) {
        State.May[MutexName] = LockKind::Read;
        State.Must[MutexName] = LockKind::Read;
    } else if (Kind == LockCallKind::Unlock) {
        State.May.erase(MutexName);
        State.Must.erase(MutexName);
    }
}