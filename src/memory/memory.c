#include "memory.h"
#include "../libs/types.h"
#include "../libs/libfuncs.h"

static page_list *identical_pglist;
sv39_pte *kernel_pgtable_root;
static uint32 free_pages[FREE_PAGE_COUNT];
static uint32 free_page_top;

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

static void init_identical_pglist()
{
    printf("-----start init pglist-----\n");
    printf("pagesize %p pagemask %p\n", PAGESIZE, PAGEMASK);
    printf("kernel_en %p masked kernel_en %p\n", kernel_end, get_pgn_ceil(kernel_end));
    uint64 now = get_pgn_ceil(kernel_end);
    for (; now < IDENTICAL_SEG_END; now += PAGESIZE)
    {
        page_list *newhead = (page_list *)now;
        newhead->next = identical_pglist;
        identical_pglist = newhead;
    }
    printf("identical pglist init ok from %p to %p\n", kernel_end, IDENTICAL_SEG_END);
}

void *alloc_identical_page()
{
    if (identical_pglist)
    {
        page_list *ret = identical_pglist;
        // printf("ALLOC page at %p\n", ret);
        identical_pglist = identical_pglist->next;
        ret->next = 0;
        memset(ret, 0, PAGESIZE);
        return ret;
    }
    return 0;
}

void free_identical_page(void *page)
{
    page_list *pg = page;
    pg->next = identical_pglist;
    identical_pglist = pg;
}

static void init_free_pages()
{
    for (int i = 0; i < FREE_PAGE_COUNT; i++)
        free_pages[i] = (IDENTICAL_SEG_END >> 12) + i;
    free_page_top = FREE_PAGE_COUNT;
}

void *alloc_free_page()
{
    if (!free_page_top)
        return NULL;
    return ((uint64)free_pages[--free_page_top]) << 12;
}

void free_free_page(void *page)
{
    free_pages[free_page_top++] = (uint64)page >> 12;
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
            sv39_pte *subpage = alloc_identical_page();
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

int map_page(sv39_pte *root, void *vaddr, void *paddr, uint8 flags)
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

static void *init_kernel_pgtable()
{
    printf("------start init page table------\n");
    kernel_pgtable_root = alloc_identical_page();
    printf("root_pagetable at %p\n", kernel_pgtable_root);
    printf("map exeseg kernbase at %p kernel_text_end at %p\n", KERNBASE, kernel_text_end);
    map_pageseg(kernel_pgtable_root, KERNBASE, KERNBASE, ((uint64)kernel_text_end - KERNBASE) >> 12, SV39_R | SV39_X);
    printf("PTE AT %p\n", find_pte(kernel_pgtable_root, KERNBASE));
    map_pageseg(kernel_pgtable_root, kernel_text_end, kernel_text_end, (IDENTICAL_SEG_END - (uint64)kernel_text_end) >> 12, SV39_R | SV39_W);
    printf("&&&&& %d %d\n", (PHYSTOP - IDENTICAL_SEG_END) >> 12, (PHYSTOP - (uint64)kernel_text_end) >> 12);
    printf("PTE AT %p\n", find_pte(kernel_pgtable_root, kernel_text_end));
    printf("page table init ok \n"); //%d %d\n", sb1, sb2);
    return kernel_pgtable_root;
}

void set_pgtable(void *table)
{
    w_satp(SATP_SV39 | ((uint64)table >> 12));
    sfence_vma();
    printf("SET PAGE TABLE %p\n", table);
}

void init_memory()
{
    init_identical_pglist();
    init_free_pages();
    sv39_pte *root = init_kernel_pgtable();
    set_pgtable(root);
    init_allocator(IDENTICAL_SEG_END, MALLOC_INIT_PAGENUM);
}