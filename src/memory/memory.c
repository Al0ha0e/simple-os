#include "memory.h"
#include "../riscv/riscv.h"
#include "../libs/types.h"
#include "../libs/libfuncs.h"

static page_list *pglist_head;
static sv39_pte *root_pgtable;

extern char kernel_end[];
extern char kernel_text_end[];

static inline uint64 get_pgn_floor(void *addr)
{
    return (uint64)addr & PAGEMASK;
}

static inline uint64 get_pgn_ceil(void *addr)
{
    uint64 ret = (uint64)addr & PAGEMASK;
    if ((uint64)addr & (PAGESIZE - 1))
        return (void *)(ret + PAGESIZE);
    return (void *)ret;
}

void gen_page(void *paddr)
{
    page_list *newhead = (page_list *)paddr;
    newhead->next = pglist_head;
    pglist_head = newhead;
}

void gen_pages(void *st, void *en)
{
    uint64 now = st;
    while (1)
    {
        gen_page(now);
        now += PAGESIZE;
        if (now >= en)
            break;
    }
}

void init_pglist()
{
    printf("-----start init pglist-----\n");
    printf("pagesize %p pagemask %p\n", PAGESIZE, PAGEMASK);
    printf("kernel_en %p masked kernel_en %p\n", kernel_end, get_pgn_ceil(kernel_end));
    gen_pages(get_pgn_ceil(kernel_end), PHYSTOP);
    printf("pglist init ok from %p to %p\n", kernel_end, PHYSTOP);
}

void *alloc_phys_page()
{
    if (pglist_head)
    {
        page_list *ret = pglist_head;
        // printf("ALLOC page at %p\n", ret);
        pglist_head = pglist_head->next;
        ret->next = 0;
        memset(ret, 0, PAGESIZE);
        return ret;
    }
    return 0;
}

sv39_pte *find_pte(sv39_pte *root, void *vaddr)
{
    // printf("TRY FIND %p\n", vaddr);
    uint64 iaddr = (uint64)vaddr >> 12;
    sv39_pte *now = root;
    int i, sr = 18;
    for (i = 0; i < 2; i++)
    {
        now += (iaddr >> sr) & SV39_PNSEG_MASK;
        // printf("TEST %d vaddr %p pteaddr %p offset %p\n", 3 - i, vaddr, now, (iaddr >> sr) & SV39_PNSEG_MASK);
        // printf("PTE %p\n", *now);
        if (now->V)
        {
            now = now->ppn << 12;
        }
        else
        {
            sv39_pte *subpage = alloc_phys_page();
            if (!subpage)
                return 0;
            init_sv39pte(now, subpage, 0);
            // printf("pte %p\n", *now);
            now = subpage;
        }
        sr -= 9;
    }
    return now + ((iaddr >> sr) & SV39_PNSEG_MASK);
}

static inline int map_page(sv39_pte *root, void *vaddr, void *paddr, uint8 flags)
{
    sv39_pte *pte = find_pte(root, vaddr);
    if (!pte)
    {
        printf("bad vaddr %p paddr %p\n", vaddr, paddr);
        return -1;
    }

    init_sv39pte(pte, paddr, flags);
    return 0;
}

int map_pageseg(sv39_pte *root, void *vaddr, void *paddr, uint64 numpages, uint8 flags)
{
    int i;
    uint64 offset = 0;
    printf("try map page seg vaddr %p paddr %p numpages %d\n", vaddr, paddr, numpages);
    for (i = 0; i < numpages; i++)
    {
        if (map_page(root, vaddr + offset, paddr + offset, flags) == -1)
            return -1;
        offset += PAGESIZE;
    }
    return 0;
}

void *init_pgtable()
{
    printf("------start init page table------\n");
    root_pgtable = alloc_phys_page();
    printf("root_pagetable at %p\n", root_pgtable);
    printf("map exeseg kernbase at %p kernel_text_end at %p\n", KERNBASE, kernel_text_end);
    map_pageseg(root_pgtable, KERNBASE, KERNBASE, ((uint64)kernel_text_end - KERNBASE) >> 12, SV39_R | SV39_X);
    printf("PTE AT %p\n", find_pte(root_pgtable, KERNBASE));
    map_pageseg(root_pgtable, kernel_text_end, kernel_text_end, (PHYSTOP - (uint64)kernel_text_end) >> 12, SV39_R | SV39_W);
    printf("PTE AT %p\n", find_pte(kernel_text_end, KERNBASE));
    printf("page table init ok \n"); //%d %d\n", sb1, sb2);
    return root_pgtable;
}

void set_pgtable(void *table)
{
    w_satp(SATP_SV39 | ((uint64)table >> 12));
    sfence_vma();
    printf("SET PAGE TABLE %p\n", table);
}