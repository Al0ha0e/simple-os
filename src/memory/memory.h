#ifndef MEMORY_H
#define MEMORY_H

#define KERNBASE 0x80200000L
#define PHYSTOP (KERNBASE + 128L * 1024 * 1024)
#define PAGESIZE 0x1000L
#define PAGEMASK (~(PAGESIZE - 1))

typedef struct pg_list
{
    struct pg_list *next;
} page_list;

void init_pglist();
void *alloc_phys_page();
void *init_pgtable();
void set_pgtable(void *table);
#endif