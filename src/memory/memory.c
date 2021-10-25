#include "memory.h"
#include "../libs/types.h"
#include "../libs/libfuncs.h"

static page_list *pglist_head;

void init_pages(char *st, char *en)
{
    uint64 now = st;
    while (1)
    {
        gen_page(st);
        st += PAGESIZE;
        if (st >= en)
            break;
    }
}

void gen_page(char *paddr)
{
    memset(paddr, 0, PAGESIZE);
    page_list *newhead = (page_list *)paddr;
    newhead->next = pglist_head;
    pglist_head = newhead;
}