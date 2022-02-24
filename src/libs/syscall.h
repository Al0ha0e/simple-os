#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

#define SYSCALL_WRITE 64
#define SYSCALL_EXIT 93

void handle_syscall(uint64 arg0, uint64 arg1, uint64 arg2, uint64 which);

#endif