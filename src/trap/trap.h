#ifndef TRAP_H
#define TRAP_H
#include "../riscv/riscv.h"
#include "../riscv/sbi.h"
#include "../libs/libfuncs.h"

void init_trap();

void handle_trap(rv64_context ctx);

void trap_return(void *user_context, void *user_pgtable);

#endif