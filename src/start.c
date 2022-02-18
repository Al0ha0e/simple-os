#include "riscv/riscv.h"
#include "libs/types.h"
#include "libs/libfuncs.h"
#include "riscv/sbi.h"
#include "memory/memory.h"
#include "trap/trap.h"
#include "libs/ds.h"

__attribute__((aligned(16))) char stack0[4096];

void start()
{
    printf("-----Start initialize\n");
    init_memory();
    init_trap();
    printf("-------Simple^OS initialize OK-------\n");
    printf("%p\n", r_time());
    set_timer(r_time() + 10000L);
    while (1)
    {
        // printf(">>> %p\n", r_time());
    }
    // uint64 mstatus = r_mstatus();
    // mstatus &= ~MSTATUS_MPP_MASK;
    // mstatus |= MSTATUS_MPP_S;
    // w_mstatus(mstatus);

    // asm volatile("mret");
    // shutdown();
}