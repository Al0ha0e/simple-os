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

static void sys_getpid()
{
    process_control_block *pcb = get_curr_pcb();
    pcb->kernel_context->gprs[10] = pcb->pid;
}

static void sys_fork()
{
    proc_fork();
}

static void sys_exec() {}

void handle_syscall(uint64 arg0, uint64 arg1, uint64 arg2, uint64 which)
{
    switch (which)
    {
    case SYSCALL_WRITE:
        return sys_write(arg0, arg1, arg2);
    case SYSCALL_EXIT:
        return sys_exit(arg0);
    case SYSCALL_SCHED_YIELD:
        return sys_sched_yield();
    case SYSCALL_GETPID:
        return sys_getpid();
    case SYSCALL_FORK:
        return sys_fork();
    default:
        break;
    }
}