#include "riscv/riscv.h"
#include "libs/types.h"
#include "libs/libfuncs.h"
#include "riscv/sbi.h"
#include "memory/memory.h"
#include "trap/trap.h"
#include "libs/ds.h"
#include "libs/elf.h"
#include "proc/proc.h"

__attribute__((aligned(16))) char stack0[4096];

extern char app_0_start[];
extern char app_0_end[];

void start()
{
    printf("-----Start initialize\n");
    init_memory();
    init_trap();
    init_process_list();
    printf("-------Simple^OS initialize OK-------\n");
    printf("%p\n", r_time());
    elf_header *elfh = ((elf_header *)app_0_start);
    exec_from_mem(elfh);

    // uint64 mstatus = r_mstatus();
    // mstatus &= ~MSTATUS_MPP_MASK;
    // mstatus |= MSTATUS_MPP_S;
    // w_mstatus(mstatus);

    // asm volatile("mret");
    // sbi_shutdown();
}