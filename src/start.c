#include "riscv/riscv.h"
#include "libs/types.h"
#include "libs/libfuncs.h"
#include "riscv/sbi.h"
#include "memory/memory.h"

__attribute__((aligned(16))) char stack0[4096];

void main();

void start()
{
    printf("-----Start initialize\n");
    init_pglist();
    sv39_pte *root = init_pgtable();
    set_pgtable(root);
    printf("-------Simple OS initialize OK-------\n");
    // uint64 mstatus = r_mstatus();
    // mstatus &= ~MSTATUS_MPP_MASK;
    // mstatus |= MSTATUS_MPP_S;
    // w_mstatus(mstatus);

    // asm volatile("mret");
    //shutdown();
}