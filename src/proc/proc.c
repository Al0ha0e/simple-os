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

void exec_from_mem(elf_header *elf)
{
    uint32 pid = ++proc_count;
    void *ctx_page = set_user_context(pid);
    printf("A\n");
    void *user_pgtable_root = init_userproc_pgtable(elf, ctx_page);
    rv64_context *kernel_context = GET_KERNEL_USER_CONTEXT_ADDR(pid);

    printf("B\n");

    process_control_block pcb;
    pcb.pid = pid;
    pcb.pagetable_root = user_pgtable_root;
    pcb.kernel_context = kernel_context;
    vector_push_back(&process_list, &pcb);
    current_pcb = vector_get_item(&process_list, pid - 1);

    printf("C\n");

    kernel_context->sepc = elf->entry;
    kernel_context->sstatus = r_sstatus(); //| (1L << 8);
    kernel_context->kernel_context = kernel_context;
    kernel_context->kernel_pgtable = get_kernel_pgtable();
    kernel_context->trap_handler = handle_trap;
    kernel_context->gprs[2] = USER_STACK_PAGE + USER_STACK_INIT_PAGENUM * PAGESIZE;

    printf("READY FOR RETURN\n");
    trap_return(USER_CONTEXT_VADDR, CONV_SV39_PGTABLE(user_pgtable_root));
}