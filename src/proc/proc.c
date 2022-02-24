#include "proc.h"
#include "../libs/elf.h"
#include "../memory/memory.h"
#include "../libs/ds.h"
#include "../trap/trap.h"

static process_control_block *current_pcb;
static uint32 proc_count;

static vector process_list;

void init_process_list()
{
    init_vector(&process_list, sizeof(process_control_block), 8);
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
    uint32 pid = ++proc_count;

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
    kernel_context->kernel_pgtable = get_kernel_pgtable();
    kernel_context->trap_handler = handle_trap;
    kernel_context->gprs[2] = USER_STACK_PAGE + USER_STACK_INIT_PAGENUM * PAGESIZE;

    trap_return(USER_CONTEXT_VADDR, CONV_SV39_PGTABLE(pcb->pagetable_root));
}