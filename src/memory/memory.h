#ifndef MEMORY_H
#define MEMORY_H

#include "../libs/types.h"
#include "../riscv/riscv.h"

#define KERNBASE 0x80200000L
#define IDENTICAL_SEG_END (KERNBASE + 16L * 1024 * 1024)
#define PHYSTOP (KERNBASE + 128L * 1024 * 1024)
#define FREE_PAGE_COUNT ((PHYSTOP - IDENTICAL_SEG_END) >> 12)
#define PAGESIZE 0x1000L
#define PAGEMASK (~(PAGESIZE - 1))
#define MALLOC_INIT_PAGENUM 4

typedef struct pg_list
{
    struct pg_list *next;
} page_list;

void init_memory();

void *alloc_identical_page();
void free_identical_page(void *page);

void *alloc_free_page();
void free_free_page(void *page);

int map_page(sv39_pte *root, void *vaddr, void *paddr, uint8 flags);
void set_pgtable(void *table);

typedef struct empty_block
{
    size_t size;
    struct empty_block *next;
} empty_block;

#define EMPTY_BLOCK_SIZE sizeof(empty_block)

void init_allocator(void *heap_st, unsigned int n);
void *malloc(size_t size);
void free(void *addr);

#endif