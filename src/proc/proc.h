#ifndef PROC_H
#define PROC_H

#include "../riscv/riscv.h"
#include "../libs/elf.h"

typedef struct process_control_block
{
    uint32 pid;
    void *pagetable_root;
    rv64_context *kernel_context;
} process_control_block;

process_control_block *get_curr_pcb();

void init_process_list();

void exec_from_mem(elf_header *elf);

// void exec();

#endif