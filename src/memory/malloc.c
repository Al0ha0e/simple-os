#include "memory.h"
#include "../riscv/riscv.h"

static empty_block *freelist = NULL;
static void *heap_st;
static void *heap_en;

static void get_pages(void *st, uint32 n)
{
    for (int i = 0; i < n; i++)
    {
        void *paddr = alloc_free_page();
        map_page(get_kernel_pgtable(), st + i * PAGESIZE, paddr, SV39_R | SV39_W);
    }
}

void init_allocator(void *st, uint32 n)
{
    get_pages(st, n);
    heap_st = freelist = st;
    heap_en = ((char *)st) + n * PAGESIZE;
    freelist->next = NULL;
    freelist->size = n * PAGESIZE - EMPTY_BLOCK_SIZE;
}

static uint32 get_pgcnt(size_t size)
{
    size_t rem = size & (PAGESIZE - 1);
    uint32 ret = (size >> PAGESHIFT) + (rem ? 1 : 0);
}

static void *extend_heap(uint32 n)
{
    void *ret = heap_en;
    get_pages(ret, n);
    heap_en = ((char *)heap_en) + n * PAGESIZE;
    return ret;
}

static void *alloc_in_block(empty_block *now, empty_block *prev, size_t rsize)
{
    size_t *rst;
    if (rsize >= now->size)
    {
        rsize = now->size + EMPTY_BLOCK_SIZE;
        prev ? (prev->next = now->next) : (freelist = now->next);
        rst = (size_t *)now;
    }
    else
    {
        now->size -= rsize;
        rst = (size_t *)(((char *)(now + 1)) + now->size);
    }
    *rst = rsize;
    return (void *)(rst + 1);
}

void *malloc(size_t size)
{
    empty_block *now = NULL;
    empty_block *prev = NULL;
    size_t rsize = size + sizeof(size_t);
    rsize = rsize >= EMPTY_BLOCK_SIZE ? rsize : EMPTY_BLOCK_SIZE;

    for (now = freelist; now; prev = now, now = now->next)
        if (now->size + EMPTY_BLOCK_SIZE >= rsize)
            return alloc_in_block(now, prev, rsize);

    uint32 pgcnt = get_pgcnt(rsize);
    now = (empty_block *)extend_heap(pgcnt);

    prev ? (prev->next = now) : (freelist = now);

    now->next = NULL;
    now->size = pgcnt * PAGESIZE - EMPTY_BLOCK_SIZE;
    return alloc_in_block(now, prev, rsize);
}

static void combine_block(empty_block *prev, empty_block *now)
{
    if (now != ((char *)(prev + 1)) + prev->size)
        return;
    prev->next = now->next;
    prev->size += now->size + EMPTY_BLOCK_SIZE;
}

void free(void *addr)
{
    size_t *st = ((size_t *)addr) - 1;
    size_t rsize = *st;
    void *en = ((char *)st) + rsize;
    empty_block *block = st;
    block->size = rsize - EMPTY_BLOCK_SIZE;

    if (!freelist || en <= freelist)
    {
        block->next = freelist;
        freelist = block;
        if (block->next)
            combine_block(block, block->next);
        return;
    }

    empty_block *now = NULL;

    for (now = freelist; now->next; now = now->next)
    {
        if (en <= now->next)
        {
            block->next = now->next;
            now->next = block;
            combine_block(block, block->next);
            combine_block(now, block);
            return;
        }
    }

    block->next = now->next;
    now->next = block;
    combine_block(now, block);
}