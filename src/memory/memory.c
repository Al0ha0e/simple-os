#include "memory.h"
#include "../libs/types.h"
#include "../libs/libfuncs.h"
#include "../libs/elf.h"
#include "../libs/ds.h"

static page_list *identical_pglist;
static sv39_pte *kernel_pgtable_root;
static uint32 free_pages[FREE_PAGE_COUNT];
static uint32 free_page_top;
static linked_list user_addrseg_list;

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

addr_seg *init_addr_seg(addr_seg *seg)
{
    init_vector(&seg->kaddrs, sizeof(void *), 4);
    seg->ref_cnt = 1;
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

static void map_user_addrseg(void *root, addr_seg_ref *seg_ref, void *vaddr, void *content, size_t size, uint8 flags)
{
    seg_ref->flags = flags;
    addr_seg *seg = malloc(sizeof(addr_seg));
    init_addr_seg(seg);
    seg->st_vaddr = vaddr;
    seg->page_cnt = size / PAGESIZE;
    seg_ref->seg = list_push_back(&user_addrseg_list, seg);
    for (size_t offset = 0; offset < size; offset += PAGESIZE)
    {
        char *dst = (char *)vaddr + offset;
        char *kernel_dst = alloc_identical_page();
        map_page(root, dst, kernel_dst, flags);
        vector_push_back(&seg->kaddrs, &kernel_dst);
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

void init_userproc_addr_space(void *user_pgtable_root, elf_header *elf, vector *seg_refs)
{
    program_header *progh = ((char *)elf) + elf->phoff;
    uint8 flags;
    addr_seg_ref *seg_ref;
    for (int i = 0; i < elf->phnum; i++)
    {
        flags = SV39_U | (progh->flags.E << 3) | (progh->flags.W << 2) | (progh->flags.R << 1);
        seg_ref = vector_extend(seg_refs);
        map_user_addrseg(user_pgtable_root, seg_ref, progh->vaddr, ((char *)elf) + progh->off, progh->memsz, flags);
        progh = progh + 1;
    }
    seg_ref = vector_extend(seg_refs);
    map_user_addrseg(user_pgtable_root, seg_ref, USER_STACK_PAGE, NULL, USER_STACK_INIT_PAGENUM * PAGESIZE, SV39_U | SV39_R | SV39_W);
}

void dispose_userproc_addr_space(vector *seg_refs)
{
    for (int i = 0; i < seg_refs->count; i++)
    {
        addr_seg_ref *seg_ref = vector_get_item(seg_refs, i);
        addr_seg *seg = (addr_seg *)seg_ref->seg->v;
        seg->ref_cnt--;
        if (!seg->ref_cnt)
        {
            for (int j = 0; j < seg->kaddrs.count; j++)
            {
                void *kaddr = *((void **)seg->kaddrs.buffer + j);
                free_identical_page(kaddr);
            }
            dispose_vector(&(seg->kaddrs));
            list_delete(&user_addrseg_list, seg_ref->seg);
        }
    }
    dispose_vector(seg_refs);
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

static void copy_user_addrseg(void *dst_pgtable_root, addr_seg_ref *dst, addr_seg_ref *src, uint8 flags)
{
    dst->flags = flags;
    char w = flags & SV39_W;
    addr_seg *src_seg = (addr_seg *)(src->seg->v);
    vector *src_kaddrs = &src_seg->kaddrs;
    char *vaddr = src_seg->st_vaddr;
    if (!w)
    {
        src_seg->ref_cnt++;
        dst->seg = src->seg;
        for (int i = 0; i < src_kaddrs->count; i++)
        {
            map_page(dst_pgtable_root, vaddr, *(void **)vector_get_item(src_kaddrs, i), flags);
            vaddr += PAGESIZE;
        }
        return;
    }

    addr_seg *dst_seg = malloc(sizeof(addr_seg));
    init_addr_seg(dst_seg);
    dst_seg->st_vaddr = src_seg->st_vaddr;
    dst_seg->page_cnt = src_seg->page_cnt;
    vector *dst_kaddrs = &dst_seg->kaddrs;
    for (int i = 0; i < src_kaddrs->count; i++)
    {
        void *dst_kaddr = alloc_identical_page();
        vector_push_back(dst_kaddrs, &dst_kaddr);
        map_page(dst_pgtable_root, vaddr, dst_kaddr, flags);
        memcpy(dst_kaddr, *(void **)vector_get_item(src_kaddrs, i), PAGESIZE);
        vaddr += PAGESIZE;
    }
    dst->seg = list_push_back(&user_addrseg_list, dst_seg);
}

void copy_userproc_addr_space(void *dst_pgtable_root, vector *dst_addr_space, vector *src_addr_space, int readonly)
{
    addr_seg_ref *dst;
    addr_seg_ref *src;
    uint8 flags;
    for (int i = 0; i < src_addr_space->count; i++)
    {
        dst = vector_extend(dst_addr_space);
        src = vector_get_item(src_addr_space, i);
        flags = src->flags;
        if (readonly)
            flags &= ~(SV39_W);
        copy_user_addrseg(dst_pgtable_root, dst, src, flags);
    }
}