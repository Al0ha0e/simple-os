#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

static inline uint64 syscall(uint64 arg0, uint64 arg1, uint64 arg2, uint64 which)
{
    uint64 ret;
    asm volatile("mv a7, a3\n\t"
                 "ecall\n\t"
                 "mv %0, a0"
                 : "=r"(ret)::"memory");
    return ret;
}

#define SYSCALL_WRITE 64
#define SYSCALL_EXIT 93
#define SYSCALL_SCHED_YIELD 124

uint64 sys_write(uint64 fd, void *buffer, size_t count)
{
    return syscall(fd, (uint64)buffer, count, SYSCALL_WRITE);
}

uint64 sys_exit(int32 state)
{
    return syscall(state, 0, 0, SYSCALL_EXIT);
}

uint64 sys_sched_yield()
{
    return syscall(0, 0, 0, SYSCALL_SCHED_YIELD);
}

#endif