#include "trap.h"
#include "../memory/memory.h"
#include "../proc/proc.h"
#include "../libs/syscall.h"
#include "../libs/time.h"

extern void _user_trap();
extern void _user_restore();

void init_trap()
{
    printf("------start init trap----------\n");
    w_stvec(TRAMPOLINE_PAGE & (~3L));
    uint64 st = r_sstatus();
    //((rv64_sstatus *)((void *)&st))->sie = 1;
    w_sie(r_sie() | 1L << 5);
    w_sstatus(st | 1L << 1);
}

static uint64 prev = 0;

void handle_trap(rv64_context ctx)
{
    rv64_scause scause = *((rv64_scause *)&ctx.scause);
    uint64 stval = ctx.stval;
    if (scause.interrupt)
    {
        printf("INTERRUPT scause %p stval %p sepc %p\n", scause, stval, ctx.sepc);
        switch (scause.exception_code)
        {
        case 5:
            uint64 now = r_time();
            prev = now;
            for (timer_info *info = get_nearest_timer(); info && info->expire <= now; info = get_nearest_timer())
            {
                timer_expire();
                if (info->type == TIMER_SCHEDULE)
                    proc_schedule(1);
            }
            break;
        default:
            break;
        }
    }
    else
    {
        // printf("EXCEPTION scause %p stval %p sepc %p\n", scause, stval, ctx.sepc);
        switch (scause.exception_code)
        {
        case 8:
            // printf("SYSCALL %d\n", ctx.gprs[17]);
            handle_syscall(ctx.gprs[10], ctx.gprs[11], ctx.gprs[12], ctx.gprs[17]);
            ctx.sepc += 4;
            break;

        default:
            break;
        }
    }
    trap_return(ctx.user_context, CONV_SV39_PGTABLE(get_curr_pcb()->pagetable_root));
}

void trap_return(void *user_context, void *user_pgtable)
{
    asm volatile("jr %0" ::"r"(TRAMPOLINE_PAGE + ((char *)_user_restore) - ((char *)_user_trap)));
}