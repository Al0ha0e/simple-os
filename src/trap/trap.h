#ifndef TRAP_H
#define TRAP_H
#include "../riscv/riscv.h"
#include "../riscv/sbi.h"
#include "../libs/libfuncs.h"

extern void _interrupt();

void init_trap();

void handle_trap(rv64_context ctx, rv64_scause scause, uint64 stval);

#endif