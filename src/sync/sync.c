#include "sync.h"

void init_spinlock(spinlock *lock)
{
    *lock = 0;
}

void spinlock_acquire(spinlock *lock)
{
    while (__sync_lock_test_and_set(lock, 1) != 0)
        ;
    __sync_synchronize();
}

void spinlock_release(spinlock *lock)
{
    __sync_synchronize();
    __sync_lock_release(lock);
}