#ifndef PROC_H
#define PROC_H

#include "../riscv/riscv.h"
#include "../libs/elf.h"
#include "../libs/ds.h"

#define MAX_PROCESS_NUM 64
#define TIME_SLICE (QEMU_CLOCK_FREQ / 1L)

typedef struct process_control_block
{
    uint32 pid;
    uint64 timer;
    void *pagetable_root;
    rv64_context *kernel_context;
    vector addr_space;
} process_control_block;

process_control_block *get_curr_pcb();

void init_process_list();

void exec_from_mem(elf_header *elf);

void proc_fork();

void proc_schedule(int expire);

// void exec();

#endif