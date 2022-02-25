#include "syscall.h"

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

uint64 sys_getpid()
{
    return syscall(0, 0, 0, SYSCALL_GETPID);
}

uint64 sys_fork()
{
    return syscall(0, 0, 0, SYSCALL_FORK);
}