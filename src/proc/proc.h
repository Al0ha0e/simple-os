#ifndef PROC_H
#define PROC_H

#include "../riscv/riscv.h"
#include "../libs/elf.h"
#include "../libs/ds.h"
#include "../libs/types.h"

#define MAX_PROCESS_NUM 64
#define TIME_SLICE (QEMU_CLOCK_FREQ / 1L)

typedef enum process_state
{
    PROC_RUNNING,
    PROC_READY,
    PROC_WAITING,
    PROC_ZOMBIE,
    PROC_DYING
} process_state;

typedef struct process_control_block
{
    uint32 pid;
    process_state state;
    uint64 timer;
    uint64 exit_code;

    void *pagetable_root;
    rv64_context *kernel_context;
    vector addr_space;
} process_control_block;

process_control_block *get_curr_pcb();

void init_process_list();

void exec_from_mem(elf_header *elf);

void proc_fork();

void proc_schedule(int expire);

void proc_exit(int32 exit_code);

// void exec();

#endif