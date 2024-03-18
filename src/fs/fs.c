#include "fs.h"
#include "driver.h"
#include "block_manager.h"
#include "../libs/memory.h"
#include "../libs/libfuncs.h"

file_system fs;

static void init_filename(char *dst, char *name)
{
    for (int now = 0; now < 27 && name[now]; ++now)
        dst[now] = name[now];
    dst[27] = '\0';
}

char *make_filename(char *name)
{
    filename_t ret = malloc(sizeof(filename_t));
    init_filename(ret, name);
    return ret;
}

void init_file_system()
{
    read_sector(0, fs.sector0);
    fs.superblock = (super_block *)fs.sector0;
    // check magic
    read_sector(INODE_BLOCK_OFFSET, &(fs.root_dir_node));
    fs.curr_dir_node = &fs.root_dir_node;
}

/////////////////// BITSET /////////////////

static int turn_to_1(int block_st, int cnt)
{
    for (int i = 0; i < cnt; i++)
    {
        size_t *block = (size_t *)read_block(i + block_st);
        for (int j = 0; j < (SECTOR_SIZE >> 3); j++)
        {
            size_t now = block[j];
            if (now == -1)
                continue;
            for (int k = 0; k < 64; k++)
                if (now & (1 << k))
                    return (i << (SECTOR_SIZE_LOG2 + 3)) + (j << 6) + k;
        }
    }
    return -1;
}

static int allocate_inode_id()
{
    return turn_to_1(INODE_BITMAP_BLOCK_OFFSET, INODE_BITMAP_BLOCK_CNT);
}

static int allocate_datablock_id()
{
    return turn_to_1(DNODE_BITMAP_BLOCK_OFFSET, DNODE_BITMAP_BLOCK_CNT);
}

/////////////////// DISK INODE /////////////

disk_inode *get_inode(int id)
{
    int inode_block_id = id / INODE_PER_BLOCK;
    int block_id = INODE_BLOCK_OFFSET + inode_block_id;
    int inner_offset = id - inode_block_id * INODE_PER_BLOCK;
    disk_inode *disknode = partial_read_block(block_id, inner_offset * sizeof(disk_inode));
    return disknode;
}

void set_inode(int id, disk_inode *inode)
{
    int inode_block_id = id / INODE_PER_BLOCK;
    int block_id = INODE_BLOCK_OFFSET + inode_block_id;
    int inner_offset = id - inode_block_id * INODE_PER_BLOCK;
    partial_write_block(block_id, inner_offset * sizeof(disk_inode), inode, sizeof(disk_inode));
}

int allocate_inode(disk_inode *inode, int type)
{
    int node_id = allocate_inode_id();
    _memset(inode, 0, sizeof(disk_inode));
    inode->info.s.ftype = type;
    set_inode(node_id, inode);
    return node_id;
}

static void flush_inode(int id, disk_inode *inode)
{
    set_inode(id, inode);
    // TODO flush data page
}

int allocate_disk_block()
{
    int block_id = allocate_datablock_id();
    return block_id;
}

/////////////////// SNODE //////////////////

static void insert_subnode(system_inode *father, system_inode *son)
{
    list_push_back(&(father->subnodes), son);
}

static void remove_subnode(system_inode *father, system_inode *son)
{
    linked_list *subnodes = &(father->subnodes);
    for (list_node *now = subnodes->st; now; now = now->next)
    {
        if (now->v == son)
        {
            list_delete(subnodes, now);
            return;
        }
    }
}

static system_inode *make_snode(int id, char *name, system_inode *father)
{
    system_inode *ret = malloc(sizeof(system_inode));
    init_filename(ret->name, name);
    ret->inode_id = id;
    ret->ref_cnt = 1;
    ret->father = father;
    init_linked_list(&(ret->subnodes));
    if (father != NULL)
        insert_subnode(father, ret);
    return ret;
}

static system_inode *get_snode(int id, char *name, system_inode *father)
{
    system_inode *ret = make_snode(id, name, father);
    memcpy(&(ret->disknode), get_inode(id), sizeof(disk_inode));
    return ret;
}

static void free_snode(system_inode *snode)
{
    system_inode *now = snode;
    system_inode *fa;
    while (1)
    {
        fa = now->father;
        if (fa != NULL)
        {
            flush_inode(now->inode_id, &(now->disknode));
            remove_subnode(fa, now);
            free(now);
            if ((fa->subnodes).count > 0 || fa->ref_cnt > 0)
                break;
            now = fa;
        }
        else
        {
            break;
        }
    }
}

/////////////////// Read/Write //////////////////

static int alloc_datablock_for_entry(fn_entry *entry)
{
    int block_id = allocate_datablock_id();
    if (block_id == -1)
        return -1;
    entry->s.valid = 1;
    entry->s.id = block_id;
}

static int locate_data_block(disk_inode *inode, size_t offset)
{
    offset >>= SECTOR_SIZE_LOG2;
    if (offset < DIRECT_CNT)
    {
        fn_entry *entry = &(inode->direct[offset]);
        if (!entry->s.valid)
            if (alloc_datablock_for_entry(entry) == -1)
                return -1;
        return entry->s.id;
    }
    offset -= DIRECT_CNT;
    if (offset < L1_INDIRECT_CNT * FN_ENTRY_PER_BLOCK)
    {
        int offset1 = offset / FN_ENTRY_PER_BLOCK;
        fn_entry *entry1 = &(inode->l1_indirect[offset1]);
        if (!entry1->s.valid)
            if (alloc_datablock_for_entry(entry1) == -1)
                return -1;
        int inner_offset = offset - offset1 * FN_ENTRY_PER_BLOCK;
        fn_entry *entry = (fn_entry *)partial_read_block(entry1->s.id, inner_offset * sizeof(fn_entry));
        if (!entry->s.valid)
            if (alloc_datablock_for_entry(entry) == -1)
                return -1;
        return entry->s.id;
    }
    offset -= L1_INDIRECT_CNT * FN_ENTRY_PER_BLOCK;
    if (offset < L2_INDIRECT_CNT * FN_ENTRY_PER_BLOCK * FN_ENTRY_PER_BLOCK)
    {
        int offset2 = offset / (FN_ENTRY_PER_BLOCK * FN_ENTRY_PER_BLOCK);
        fn_entry *entry2 = &(inode->l2_indirect[offset2]);
        if (!entry2->s.valid)
            if (alloc_datablock_for_entry(entry2) == -1)
                return -1;
        int offset1 = offset / FN_ENTRY_PER_BLOCK - offset2 * FN_ENTRY_PER_BLOCK;
        fn_entry *entry1 = (fn_entry *)partial_read_block(entry2->s.id, offset1 * sizeof(fn_entry));
        if (!entry1->s.valid)
            if (alloc_datablock_for_entry(entry1) == -1)
                return -1;
        int inner_offset = offset - offset2 * FN_ENTRY_PER_BLOCK * FN_ENTRY_PER_BLOCK - offset1 * FN_ENTRY_PER_BLOCK;
        fn_entry *entry = (fn_entry *)partial_read_block(entry1->s.id, inner_offset * sizeof(fn_entry));
        if (!entry->s.valid)
            if (alloc_datablock_for_entry(entry) == -1)
                return -1;
        return entry->s.id;
    }
    return -1;
};

static char *read_data_block_from_disk(disk_inode *inode, size_t offset)
{
    int block_id = locate_data_block(inode, offset);
    return read_block(block_id);
}

static void read_from_disk(disk_inode *inode, size_t offset, char *dist, size_t size)
{
    int in_block_offset = offset & (SECTOR_SIZE - 1);

    if (size + in_block_offset > SECTOR_SIZE)
    {
        size_t block_id = locate_data_block(inode, offset);
        size_t size1 = SECTOR_SIZE - in_block_offset;
        memcpy(dist, partial_read_block(block_id, in_block_offset), size1);
        dist += size1;
        offset += size1;
        size -= size1;
    }

    for (; size >= SECTOR_SIZE; size -= SECTOR_SIZE)
    {
        memcpy(dist, read_data_block_from_disk(inode, offset), SECTOR_SIZE);
        dist += SECTOR_SIZE;
        offset += SECTOR_SIZE;
    }

    if (size > 0)
        memcpy(dist, read_data_block_from_disk(inode, offset), size);
}

static void write_data_block_to_disk(disk_inode *inode, size_t offset, char *src)
{
    int block_id = locate_data_block(inode, offset);
    write_block(block_id, src);
}

static void write_to_disk(disk_inode *inode, size_t offset, char *src, size_t size)
{
    int in_block_offset = offset & (SECTOR_SIZE - 1);

    if (size + in_block_offset > SECTOR_SIZE)
    {
        size_t block_id = locate_data_block(inode, offset);
        size_t size1 = SECTOR_SIZE - in_block_offset;
        partial_write_block(block_id, in_block_offset, src, size1);
        src += size1;
        offset += size1;
        size -= size1;
    }

    for (; size >= SECTOR_SIZE; size -= SECTOR_SIZE)
    {
        write_data_block_to_disk(inode, offset, src);
        src += SECTOR_SIZE;
        offset += SECTOR_SIZE;
    }

    if (size > 0)
    {
        size_t block_id = locate_data_block(inode, offset);
        partial_write_block(block_id, 0, src, size);
    }
}

/////////////////// Dir File //////////////////

static int locate_in_dir(disk_inode *inode, char *path)
{
    int size = inode->size;
    int now = 0;
    index_entry *entry;
    for (int now = 0; now < size;)
    {
        entry = (index_entry *)read_data_block_from_disk(inode, now);
        for (int i = 0; i < INDEX_ENTRY_PER_BLOCK && now < size; i++)
        {
            if (_strcmp(path, entry->name) == 0)
                return entry->inode_id;
            now += sizeof(index_entry);
            entry++;
        }
    }
    return -1;
}

static int insert_to_dir(disk_inode *index_inode, const filename_t name, int inode_id)
{
    int entry_cnt = index_inode->size / sizeof(index_entry);
    if (entry_cnt == INDEX_ENTRY_PER_BLOCK)
        return -1;
    index_entry entry;
    entry.inode_id = inode_id;
    memcpy(entry.name, name, sizeof(filename_t));
    write_to_disk(index_inode, entry_cnt * sizeof(index_entry), &entry, sizeof(index_entry));
    return 0;
}

////////////// Process FDs /////////////////////

void init_proc_files(proc_files *pf)
{
    for (int i = 0; i < MAX_PROC_FD_CNT; i++)
        pf->fds[i] = i + 3;
    pf->avaliable_fd_cnt = MAX_PROC_FD_CNT;
    pf->used_fd_cnt = 0;
    _memset(pf->inodes, 0, sizeof(proc_inode *) * MAX_PROC_FD_CNT);
}

static int allocate_fd(proc_files *pf)
{
    int avaliable_fd_cnt = pf->avaliable_fd_cnt;
    if (avaliable_fd_cnt == 0)
        return -1;
    pf->avaliable_fd_cnt--;
    return pf->fds[MAX_PROC_FD_CNT - avaliable_fd_cnt];
}

static void free_fd(proc_files *pf, int fd)
{
    int avaliable_fd_cnt = pf->avaliable_fd_cnt;
    int r = MAX_PROC_FD_CNT - avaliable_fd_cnt;
    int i;
    for (i = 0; i < r; i++)
        if (pf->fds[i] == fd)
            break;
    if (i == r)
        return;
    pf->fds[i] = pf->fds[r - 1];
    pf->fds[r - 1] = fd;
    pf->avaliable_fd_cnt++;
}

static proc_inode *fd_to_pnode(proc_files *pf, int fd)
{
    for (int i = 0; i < pf->used_fd_cnt; i++)
        if (pf->inodes[i].fd == fd)
            return &(pf->inodes[i]);
    return NULL;
}

static proc_inode *name_to_pnode(proc_files *pf, const char *name)
{
    for (int i = 0; i < pf->used_fd_cnt; i++)
        if (_strcmp(name, pf->inodes[i].inode->name))
            return &(pf->inodes[i]);
    return NULL;
}

static system_inode *inner_create(const filename_t name, int type)
{
    system_inode *snode = make_snode(0, name, fs.curr_dir_node);
    int inode_id = allocate_inode(&(snode->disknode), type);
    if (inode_id == -1)
        return NULL;

    snode->inode_id = inode_id;
    insert_to_dir(fs.curr_dir_node, name, inode_id);
    return snode;
}

static system_inode *inner_open(const char *path, int oflag)
{
    linked_list *subnodes = &(fs.curr_dir_node->subnodes);
    for (list_node *now = subnodes->st; now; now = now->next)
    {
        system_inode *subnode = (system_inode *)(now->v);
        if (_strcmp(path, subnode->name) == 0)
        {
            subnode->ref_cnt++;
            return subnode;
        }
    }

    system_inode *snode = NULL;
    int inode_id = locate_in_dir(&(fs.curr_dir_node->disknode), path);
    if (inode_id == -1) // create
    {
        snode = inner_create(path, DATA_FILE);
    }
    else
    {
        snode = get_snode(inode_id, path, fs.curr_dir_node);
    }
    return snode;
}

int fs_open(proc_files *pf, const char *path, int oflag)
{
    proc_inode *pnode = name_to_pnode(pf, path);
    return pnode->fd;

    int fd = allocate_fd(pf);
    if (fd == -1)
        return -1;

    system_inode *snode = inner_open(path, oflag);
    if (snode == NULL)
        return -1;

    int pos = pf->used_fd_cnt - 1;
    proc_inode *pnode = &(pf->inodes[pos]);
    pnode->fd = fd;
    pnode->access_mode = oflag;
    pnode->inode = snode;
    return fd;
}

static int inner_close(system_inode *snode)
{
    --(snode->ref_cnt);
    if (snode->ref_cnt > 0)
        return 0;
    free_snode(snode);
    return 0;
}

int fs_close(proc_files *pf, int fd)
{
    proc_inode *pnode = fd_to_pnode(pf, fd);
    if (pnode == NULL)
        return -1;
    free_fd(pf, fd);
    return inner_close(pnode->inode);
}

int fs_lseek(proc_files *pf, int fd, size_t offset, int whence)
{
    proc_inode *pnode = fd_to_pnode(pf, fd);
    if (pnode == NULL)
        return -1;
    // TODO whence
    pnode->offset = offset;
    if (offset >= pnode->inode->disknode.size)
        pnode->inode->disknode.size = offset + 1;
    return 0;
}

size_t fs_read(proc_files *pf, int fd, char *buf, size_t nbytes)
{
    proc_inode *pnode = fd_to_inode(pf, fd);
    if (pnode == NULL)
        return -1;
    // TODO check flag
    disk_inode *inode = &(pnode->inode->disknode);
    size_t maxlen = inode->size - pnode->offset;
    if (maxlen == 0)
        return 0;

    size_t size = nbytes > maxlen ? maxlen : nbytes;
    read_from_disk(inode, pnode->offset, buf, size);
    pnode->offset += size;
    return size;
}

size_t fs_write(proc_files *pf, int fd, char *buf, size_t nbytes)
{
    proc_inode *pnode = fd_to_inode(pf, fd);
    if (pnode == NULL)
        return -1;
    // TODO check flag
    disk_inode *inode = &(pnode->inode->disknode);
    if (pnode->offset + nbytes > inode->size)
        inode->size = pnode->offset + nbytes;
    write_to_disk(inode, pnode->offset, buf, nbytes);
    pnode->offset += nbytes;
    return 0;
}