#ifndef MEMORY_H
#define MEMORY_H

#define KERNBASE 0x80200000L
#define PHYSTOP (KERNBASE + 128 * 1024 * 1024)
#define PAGESIZE 0x1000L

typedef struct pg_list
{
    struct pg_list *next;
} page_list;

#endif