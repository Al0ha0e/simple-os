#ifndef MEMORY_H
#define MEMORY_H

#include "../libs/types.h"
#include "../libs/ds.h"
#include "../libs/elf.h"
#include "../riscv/riscv.h"

#define KERNBASE 0x80200000L
#define IDENTICAL_SEG_END (KERNBASE + 128L * 1024 * 1024)
#define PHYSTOP (KERNBASE + 192L * 1024 * 1024)
#define PAGESIZE 0x1000L
#define PAGESHIFT 12
#define FREE_PAGE_COUNT ((PHYSTOP - IDENTICAL_SEG_END) >> PAGESHIFT)
#define PAGEMASK (~(PAGESIZE - 1))
#define MALLOC_INIT_PAGENUM 4
#define TRAMPOLINE_PAGE 0xFFFFFFFFFFFFF000L
#define USER_CONTEXT_PAGE 0xFFFFFFFFFFFFE000L
#define USER_CONTEXT_VADDR (void *)(TRAMPOLINE_PAGE - sizeof(rv64_context))
#define USER_STACK_PAGE 0x0000000100000000L
#define USER_STACK_INIT_PAGENUM 1

#define GET_KERNEL_USER_CONTEXT_PAGE(x) (TRAMPOLINE_PAGE - x * PAGESIZE * 2)
#define GET_KERNEL_USER_CONTEXT_ADDR(x) (TRAMPOLINE_PAGE - x * PAGESIZE * 2 + PAGESIZE - sizeof(rv64_context))
#define CONV_SV39_PGTABLE(x) (SATP_SV39 | ((uint64)x >> PAGESHIFT))

typedef struct pg_list
{
    struct pg_list *next;
} page_list;

typedef struct empty_block
{
    size_t size;
    struct empty_block *next;
} empty_block;

#define EMPTY_BLOCK_SIZE sizeof(empty_block)

typedef struct addr_seg
{
    void *st_vaddr;
    size_t page_cnt;
    vector kaddrs;
    uint ref_cnt;
} addr_seg;

typedef struct addr_seg_ref
{
    list_node *seg;
    uint8 flags;
} addr_seg_ref;

addr_seg *init_addr_seg(addr_seg *seg);

void init_memory();

void *alloc_identical_page();
void free_identical_page(void *page);

void *alloc_free_page();
void free_free_page(void *page);

void *get_kernel_pgtable();

int map_page(sv39_pte *root, void *vaddr, void *paddr, uint8 flags);
void set_pgtable(void *table);
void dispose_pgtable(sv39_pte *root);

void init_allocator(void *heap_st, uint32 n);
void *malloc(size_t size);
void free(void *addr);

void *init_user_context(uint32 pid);
void *init_userproc_pgtable(void *ctx_page);
void init_userproc_addr_space(void *user_pgtable_root, elf_header *elf, vector *segs);
void dispose_userproc_addr_space(vector *segs);

void *convert_user_addr(void *user_pgtable, void *addr);

void copy_userproc_addr_space(void *dst_pgtable_root, vector *dst_addr_space, vector *src_addr_space, int readonly);
#endif