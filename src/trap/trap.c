#include "trap.h"
#include "../memory/memory.h"
#include "../proc/proc.h"
#include "../libs/syscall.h"
#include "../libs/time.h"

extern void _user_trap();
extern void _user_restore();
extern void _kernel_trap();
extern void _kernel_restore();

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

static void kernel_trap_return(void *kernel_context)
{
    asm volatile("jr %0" ::"r"(_kernel_restore));
}

void user_trap_return(void *user_context, void *user_pgtable)
{
    asm volatile("jr %0" ::"r"(TRAMPOLINE_PAGE + ((char *)_user_restore) - ((char *)_user_trap)));
}

void handle_trap(rv64_context ctx, int from_user)
{
    int a;
    *(&a) = from_user;
    rv64_context *tmpctx = &ctx;
    process_control_block *tmppcb = get_curr_pcb();
    tmppcb->kernel_context = tmpctx;
    rv64_scause scause = *((rv64_scause *)&tmpctx->scause);
    uint64 stval = tmpctx->stval;

    if (scause.interrupt)
    {
        printf("INTERRUPT time %p scause %p stval %p sepc %p from_user %d\n", r_time(), scause, stval, tmpctx->sepc, a);
        printf("KCONTEXT %p UCONTEXT %p\n", tmpctx->kernel_context, tmpctx->user_context);
        switch (scause.exception_code)
        {
        case 5:
            uint64 now = r_time();
            prev = now;
            for (timer_info *info = get_nearest_timer(); info && info->expire <= now; info = get_nearest_timer())
            {
                timer_expire();
                if (info->type == TIMER_SCHEDULE)
                {
                    if (a)
                        proc_schedule(1);
                    else
                        tmppcb->timer = set_timer(r_time() + TIME_SLICE, TIMER_SCHEDULE);
                }
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
            // w_sstatus(r_sstatus() | 2L);
            handle_syscall(tmpctx->gprs[10], tmpctx->gprs[11], tmpctx->gprs[12], tmpctx->gprs[17]);
            tmpctx->sepc += 4;
            break;

        default:
            break;
        }
    }
    w_sstatus(r_sstatus() & ~(2L));
    process_control_block *currpcb = get_curr_pcb();
    if (tmppcb->pid != currpcb->pid)
    {
        rv64_context *currctx = currpcb->kernel_context;
        if (!(currctx->sstatus & (1L << 8)))
        {
            w_stvec(TRAMPOLINE_PAGE & (~3L));
            user_trap_return(currctx->user_context, CONV_SV39_PGTABLE(currpcb->pagetable_root));
        }
        kernel_trap_return(currctx);
    }

    tmppcb->kernel_context = tmpctx;
    if (a)
    {
        w_stvec(TRAMPOLINE_PAGE & (~3L));
        user_trap_return(tmpctx->user_context, CONV_SV39_PGTABLE(tmppcb->pagetable_root));
    }
    kernel_trap_return(tmpctx);
}
