#include "proc.h"
#include "../libs/elf.h"
#include "../libs/time.h"
#include "../memory/memory.h"
#include "../libs/ds.h"
#include "../trap/trap.h"

static process_control_block *current_pcb;

static process_control_block process_list[MAX_PROCESS_NUM];
static uint32 process_ids[MAX_PROCESS_NUM];
static uint32 process_ids_top;
static linked_list ready_list;

static uint32 alloc_pid()
{
    return process_ids[--process_ids_top];
}
static void free_pid(uint32 pid)
{
    process_ids[process_ids_top++] = pid;
}

void init_process_list()
{
    process_ids_top = MAX_PROCESS_NUM;
    for (int i = 0; i < MAX_PROCESS_NUM; i++)
        process_ids[i] = i;
}

process_control_block *get_curr_pcb()
{
    return current_pcb;
}

static process_control_block *init_pcb(
    process_control_block *pcb,
    uint32 pid,
    process_state state,
    void *pagetable_root,
    rv64_context *kernel_context)
{
    pcb->pid = pid;
    pcb->state = state;
    pcb->pagetable_root = pagetable_root;
    pcb->kernel_context = kernel_context;
    init_vector(&pcb->addr_space, sizeof(addr_seg_ref), 4);
    return pcb;
}

void exec_from_mem(elf_header *elf)
{
    uint32 pid = alloc_pid();

    process_control_block *pcb = process_list + pid;
    init_pcb(
        pcb,
        pid,
        PROC_RUNNING,
        init_userproc_pgtable(init_user_context(pid)),
        GET_KERNEL_USER_CONTEXT_ADDR(pid));

    init_userproc_addr_space(pcb->pagetable_root, elf, &pcb->addr_space);

    current_pcb = pcb;

    rv64_context *kernel_context = pcb->kernel_context;
    kernel_context->sstatus = r_sstatus(); //| (1L << 8);
    kernel_context->sepc = elf->entry;
    kernel_context->scause = r_scause();
    kernel_context->stval = r_stval();
    kernel_context->kernel_pgtable = CONV_SV39_PGTABLE(get_kernel_pgtable());
    kernel_context->kernel_context = kernel_context;
    kernel_context->user_context = USER_CONTEXT_VADDR - sizeof(rv64_context);
    kernel_context->trap_handler = handle_trap;
    kernel_context->gprs[2] = USER_STACK_PAGE + USER_STACK_INIT_PAGENUM * PAGESIZE;

    list_push_back(&ready_list, pcb);
    pcb->timer = set_timer(r_time() + TIME_SLICE, TIMER_SCHEDULE);
    user_trap_return(kernel_context->user_context, CONV_SV39_PGTABLE(pcb->pagetable_root));
}

void proc_fork()
{
    process_control_block *ori = current_pcb;
    uint32 pid = alloc_pid();

    process_control_block *pcb = &process_list[pid];
    init_pcb(
        pcb,
        pid,
        PROC_READY,
        init_userproc_pgtable(init_user_context(pid)),
        GET_KERNEL_USER_CONTEXT_ADDR(pid));
    memcpy(GET_KERNEL_USER_CONTEXT_ADDR(pid), GET_KERNEL_USER_CONTEXT_ADDR(ori->pid), sizeof(rv64_context));
    copy_userproc_addr_space(pcb->pagetable_root, &(pcb->addr_space), &(ori->addr_space), 0);

    ori->kernel_context->gprs[10] = pid;

    rv64_context *kernel_context = pcb->kernel_context;
    kernel_context->kernel_context = kernel_context;
    kernel_context->gprs[10] = 0;
    kernel_context->sepc += 4;
    // current_pcb = pcb;

    list_push_front(&ready_list, pcb);
}

void proc_schedule(int expire)
{
    if (!expire)
        remove_timer(current_pcb->timer);
    printf("SCHED %d st %d en %d\n",
           current_pcb->pid,
           ((process_control_block *)(ready_list.st->v))->pid,
           ((process_control_block *)(ready_list.en->v))->pid);
    current_pcb = ready_list.st->v;
    current_pcb->timer = set_timer(r_time() + TIME_SLICE, TIMER_SCHEDULE);
    list_move_to_back(&ready_list, ready_list.st);
}

void proc_exit(int32 exit_code)
{
    current_pcb->exit_code = exit_code;
    dispose_pgtable(current_pcb->pagetable_root);
    dispose_userproc_addr_space(&current_pcb->addr_space);
    current_pcb->state = PROC_ZOMBIE;
    // TODO waitpid
    printf("EXIT pid %d st %d en %d code %d\n",
           current_pcb->pid,
           ((process_control_block *)(ready_list.st->v))->pid,
           ((process_control_block *)(ready_list.en->v))->pid,
           exit_code);
    list_pop_back(&ready_list);
    printf("EXIT2 st %d en %d\n",
           ((process_control_block *)(ready_list.st->v))->pid,
           ((process_control_block *)(ready_list.en->v))->pid);
    proc_schedule(0);
}