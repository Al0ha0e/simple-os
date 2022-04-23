#include "memory.h"
#include "../libs/types.h"
#include "../libs/libfuncs.h"
#include "../libs/elf.h"

static page_list *identical_pglist;
static sv39_pte *kernel_pgtable_root;
static uint32 free_pages[FREE_PAGE_COUNT];
static uint32 free_page_top;

extern char kernel_end[];
extern char kernel_text_end[];
extern char kernel_rodata_end[];
extern char kernel_data_end[];
extern char trampoline_start[];

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

memory_seg *init_memory_seg(memory_seg *seg)
{
    init_vector(&seg->user_kernel, sizeof(user_kernel_addr_mapping), 4);
    return seg;
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

void *get_kernel_pgtable()
{
    return kernel_pgtable_root;
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

static void dispose_pglevel(sv39_pte *now, int level)
{
    sv39_pte *nxt = now;
    for (int i = 0; i < PAGESIZE / sizeof(sv39_pte); i++, nxt++)
    {
        if (nxt->V)
            level == 3 ? dispose_pglevel(nxt->ppn << 12, 2)
                       : free_identical_page(nxt->ppn << 12);
    }
    free_identical_page(now);
}

void dispose_pgtable(sv39_pte *root)
{
    dispose_pglevel(root, 3);
}

static void *init_kernel_pgtable()
{
    printf("------start init page table------\n");
    kernel_pgtable_root = alloc_identical_page();
    printf("root_pagetable at %p\n", kernel_pgtable_root);
    printf("kernbase:\t%p\nkernel_text_end:\t%p\nkernel_rodata_end:\t%p\nkernel_data_end:\t%p\nkernel_end:\t%p\n",
           KERNBASE,
           kernel_text_end,
           kernel_rodata_end,
           kernel_data_end,
           kernel_end);
    map_pageseg(kernel_pgtable_root, KERNBASE, KERNBASE, ((uint64)kernel_text_end - KERNBASE) >> 12, SV39_R | SV39_X);
    map_pageseg(kernel_pgtable_root, kernel_text_end, kernel_text_end, ((uint64)kernel_rodata_end - (uint64)kernel_text_end) >> 12, SV39_R);
    map_pageseg(kernel_pgtable_root, kernel_rodata_end, kernel_rodata_end, (IDENTICAL_SEG_END - (uint64)kernel_rodata_end) >> 12, SV39_R | SV39_W);
    map_page(kernel_pgtable_root, TRAMPOLINE_PAGE, trampoline_start, SV39_X | SV39_R);

    printf("page table init ok \n");
    return kernel_pgtable_root;
}

static void map_user_pageseg(void *root, memory_seg *seg, void *vaddr, void *content, size_t size, uint8 flags)
{
    seg->flags = flags;
    seg->st_vaddr = vaddr;
    user_kernel_addr_mapping *pair;
    for (size_t offset = 0; offset < size; offset += PAGESIZE)
    {
        char *dst = (char *)vaddr + offset;
        char *kernel_dst = alloc_identical_page();
        map_page(root, dst, kernel_dst, flags);
        pair = vector_extend(&seg->user_kernel);
        pair->uaddr = dst;
        pair->kaddr = kernel_dst;
        if (content)
            memcpy(kernel_dst, (char *)content + offset, PAGESIZE <= size - offset ? PAGESIZE : size - offset);
    }
}

void *init_userproc_pgtable(void *ctx_page)
{
    void *user_pgtable_root = alloc_identical_page();
    map_page(user_pgtable_root, TRAMPOLINE_PAGE, trampoline_start, SV39_X | SV39_R);
    map_page(user_pgtable_root, USER_CONTEXT_PAGE, ctx_page, SV39_R | SV39_W);
    return user_pgtable_root;
}

void init_userproc_addr_space(void *user_pgtable_root, elf_header *elf, vector *segs)
{
    program_header *progh = ((char *)elf) + elf->phoff;
    uint8 flags;
    memory_seg *seg;
    for (int i = 0; i < elf->phnum; i++)
    {
        flags = SV39_U | (progh->flags.E << 3) | (progh->flags.W << 2) | (progh->flags.R << 1);
        seg = init_memory_seg(vector_extend(segs));
        map_user_pageseg(user_pgtable_root, seg, progh->vaddr, ((char *)elf) + progh->off, progh->memsz, flags);
        progh = progh + 1;
    }
    seg = init_memory_seg(vector_extend(segs));
    map_user_pageseg(user_pgtable_root, seg, USER_STACK_PAGE, NULL, USER_STACK_INIT_PAGENUM * PAGESIZE, SV39_U | SV39_R | SV39_W);
}

void dispose_userproc_addr_space(vector *segs)
{
    for (int i = 0; i < segs->count; i++)
    {
        memory_seg *seg = vector_get_item(segs, i);
        for (int j = 0; j < seg->user_kernel.count; j++)
        {
            user_kernel_addr_mapping *mapping = vector_get_item(&(seg->user_kernel), j);
            free_identical_page(mapping->kaddr);
        }
        dispose_vector(&(seg->user_kernel));
    }
    dispose_vector(segs);
}

void *init_user_context(uint32 pid)
{
    void *ret = alloc_free_page();
    map_page(kernel_pgtable_root, GET_KERNEL_USER_CONTEXT_PAGE(pid), ret, SV39_R | SV39_W);
    return ret;
}

void set_pgtable(void *table)
{
    w_satp(CONV_SV39_PGTABLE(table));
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

void *convert_user_addr(void *user_pgtable, void *addr)
{
    sv39_pte *pte = find_pte(user_pgtable, addr);
    char *ret = pte->ppn << 12;
    ret += (uint64)addr & (PAGESIZE - 1);
    return ret;
}

static void copy_user_pageseg(void *dst_pgtable_root, memory_seg *dst, memory_seg *src)
{
    dst->st_vaddr = src->st_vaddr;
    dst->flags = src->flags;
    vector *src_pairs = &(src->user_kernel);
    vector *dst_pairs = &(dst->user_kernel);
    for (int i = 0; i < src_pairs->count; i++)
    {
        user_kernel_addr_mapping *dst_pair = vector_extend(dst_pairs);
        user_kernel_addr_mapping *src_pair = vector_get_item(src_pairs, i);
        dst_pair->uaddr = src_pair->uaddr;
        dst_pair->kaddr = alloc_identical_page();
        map_page(dst_pgtable_root, dst_pair->uaddr, dst_pair->kaddr, dst->flags);
        memcpy(dst_pair->kaddr, src_pair->kaddr, PAGESIZE);
    }
}

void copy_userproc_addr_space(void *dst_pgtable_root, vector *dst_addr_space, vector *src_addr_space)
{
    memory_seg *dst;
    for (int i = 0; i < src_addr_space->count; i++)
    {
        dst = init_memory_seg(vector_extend(dst_addr_space));
        copy_user_pageseg(dst_pgtable_root, dst, vector_get_item(src_addr_space, i));
    }
}