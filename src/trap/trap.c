#include "trap.h"

void init_trap()
{
    printf("------start init trap----------\n");
    w_stvec((uint64)_interrupt & (~3L));
    uint64 st = r_sstatus();
    //((rv64_sstatus *)((void *)&st))->sie = 1;
    w_sie(r_sie() | 1L << 5);
    w_sstatus(st | 1L << 1);
}

static uint64 prev = 0;

void handle_trap(rv64_context ctx, rv64_scause scause, uint64 stval)
{
    uint64 now = r_time();
    printf("INTERRUPT scause %p stval %p sepc %p det %d\n", scause, stval, ctx.sepc, now - prev);
    prev = now;
    set_timer(r_time() + 10000000L);
}