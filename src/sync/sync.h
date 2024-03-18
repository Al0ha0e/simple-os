#ifndef SYNC_H
#define SYNC_H

#include "../libs/types.h"

typedef uint spinlock;

void init_spinlock(spinlock *lock);
void spinlock_acquire(spinlock *lock);
void spinlock_release(spinlock *lock);

#endif