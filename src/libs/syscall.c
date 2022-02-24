#include "syscall.h"
#include "types.h"
#include "libfuncs.h"
#include "../proc/proc.h"
#include "../memory/memory.h"
#include "../riscv/sbi.h"

static void sys_write(uint64 fd, void *buffer, size_t count)
{
    process_control_block *pcb = get_curr_pcb();
    void *pgtable = pcb->pagetable_root;
    char *now = buffer;
    size_t cnt = 0;
    int det;
    for (
        now = convert_user_addr(pgtable, buffer);
        cnt < count;
        now = convert_user_addr(pgtable, (char *)buffer + cnt))
    {
        det = PAGESIZE - ((uint64)now & (PAGESIZE - 1));
        det = det < count - cnt ? det : count - cnt;
        for (int i = 0; i < det; i++)
            console_putchar(now[i]);
        cnt += det;
    }
    console_putchar('\n');
}

static void sys_exit(int32 state)
{
    while (1)
    {
    }
}

static void sys_sched_yield()
{
    static int sched_cnt = 0;
    printf("SCHED %d\n", sched_cnt++);
    if (sched_cnt == 5)
    {
        while (1)
        {
        }
    }
}

void handle_syscall(uint64 arg0, uint64 arg1, uint64 arg2, uint64 which)
{
    switch (which)
    {
    case SYSCALL_WRITE:
        sys_write(arg0, arg1, arg2);
        break;
    case SYSCALL_EXIT:
        sys_exit(arg0);
        break;
    case SYSCALL_SCHED_YIELD:
        sys_sched_yield();
        break;
    default:
        break;
    }
}