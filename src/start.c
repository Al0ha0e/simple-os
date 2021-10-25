#include "riscv/riscv.h"
#include "libs/types.h"
#include "riscv/sbi.h"

__attribute__((aligned(16))) char stack0[4096];

void main();

void start()
{
    // uint64 mstatus = r_mstatus();
    // mstatus &= ~MSTATUS_MPP_MASK;
    // mstatus |= MSTATUS_MPP_S;
    // w_mstatus(mstatus);

    // asm volatile("mret");
    console_putchar('S');
    console_putchar('O');
    console_putchar('S');
    console_putchar('\n');
    //shutdown();
}