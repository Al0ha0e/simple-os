#include "proc.h"
#include "../libs/elf.h"
#include "../memory/memory.h"
#include "../libs/ds.h"
#include "../trap/trap.h"

static process_control_block *current_pcb;

static vector process_list;
static uint32 process_ids[MAX_PROCESS_NUM];
static uint32 process_ids_top;

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
    init_vector(&process_list, sizeof(process_control_block), 8);
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
    void *pagetable_root,
    rv64_context *kernel_context)
{
    pcb->pid = pid;
    pcb->pagetable_root = pagetable_root;
    pcb->kernel_context = kernel_context;
    init_vector(&pcb->addr_space, sizeof(memory_seg), 4);
    return pcb;
}

void exec_from_mem(elf_header *elf)
{
    uint32 pid = alloc_pid();

    process_control_block *pcb = vector_extend(&process_list);
    init_pcb(
        pcb,
        pid,
        init_userproc_pgtable(init_user_context(pid)),
        GET_KERNEL_USER_CONTEXT_ADDR(pid));

    init_userproc_addr_space(pcb->pagetable_root, elf, &pcb->addr_space);

    current_pcb = pcb;

    rv64_context *kernel_context = pcb->kernel_context;
    kernel_context->sepc = elf->entry;
    kernel_context->sstatus = r_sstatus(); //| (1L << 8);
    kernel_context->kernel_context = kernel_context;
    kernel_context->kernel_pgtable = CONV_SV39_PGTABLE(get_kernel_pgtable());
    kernel_context->trap_handler = handle_trap;
    kernel_context->gprs[2] = USER_STACK_PAGE + USER_STACK_INIT_PAGENUM * PAGESIZE;

    trap_return(USER_CONTEXT_VADDR, CONV_SV39_PGTABLE(pcb->pagetable_root));
}

void fork()
{
    process_control_block *ori = current_pcb;
    uint32 pid = alloc_pid();

    process_control_block *pcb = vector_extend(&process_list);
    init_pcb(
        pcb,
        pid,
        init_userproc_pgtable(init_user_context(pid)),
        GET_KERNEL_USER_CONTEXT_ADDR(pid));
    memcpy(GET_KERNEL_USER_CONTEXT_ADDR(pid), GET_KERNEL_USER_CONTEXT_ADDR(ori->pid), sizeof(rv64_context));
    copy_userproc_addr_space(pcb->pagetable_root, &(pcb->addr_space), &(ori->addr_space));

    ori->kernel_context->gprs[10] = pid;

    rv64_context *kernel_context = pcb->kernel_context;
    kernel_context->kernel_context = kernel_context;
    kernel_context->gprs[10] = 0;
    kernel_context->sepc += 4;
    current_pcb = pcb;
}