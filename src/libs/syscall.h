#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

#define SYSCALL_WRITE 64
#define SYSCALL_EXIT 93
#define SYSCALL_SCHED_YIELD 124

void handle_syscall(uint64 arg0, uint64 arg1, uint64 arg2, uint64 which);

#endif