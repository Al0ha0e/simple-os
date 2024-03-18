#ifndef FS_H
#define FS_H

#include "block_manager.h"
#include "../libs/ds.h"

typedef char filename_t[28];

char *make_filename(char *name);

#define DIRECT_CNT 20
#define L1_INDIRECT_CNT 8
#define L2_INDIRECT_CNT 2

enum file_type
{
    INDEX_FILE,
    DATA_FILE
};

typedef struct inner_fn_info
{
    int ftype;
} inner_fn_info;

typedef union fn_info
{
    inner_fn_info s;
    int i;
} fn_info;

typedef struct inner_fn_entry
{
    unsigned int valid : 1;
    unsigned int id : 31;
} inner_fn_entry;

typedef union fn_entry
{
    inner_fn_entry s;
    int i;
} fn_entry;

typedef struct disk_inode
{
    int size;
    fn_info info;
    fn_entry direct[DIRECT_CNT];
    fn_entry l1_indirect[L1_INDIRECT_CNT];
    fn_entry l2_indirect[L2_INDIRECT_CNT];

} disk_inode;

#define INODE_BITMAP_BLOCK_CNT 1
#define MAX_FILE_CNT (INODE_BITMAP_BLOCK_CNT << (SECTOR_SIZE_LOG2 + 3))
#define INODE_BLOCK_CNT ((MAX_FILE_CNT * sizeof(disk_inode)) >> SECTOR_SIZE_LOG2)

#define FN_ENTRY_PER_BLOCK (SECTOR_SIZE / sizeof(fn_entry))
#define MAX_DATABLOCK_CNT (MAX_FILE_CNT * (DIRECT_CNT + L1_INDIRECT_CNT * FN_ENTRY_PER_BLOCK + L2_INDIRECT_CNT * FN_ENTRY_PER_BLOCK * FN_ENTRY_PER_BLOCK))
#define DNODE_BITMAP_BLOCK_CNT (MAX_DATABLOCK_CNT >> (SECTOR_SIZE_LOG2 + 3))

#define INODE_BITMAP_BLOCK_OFFSET 1
#define DNODE_BITMAP_BLOCK_OFFSET (INODE_BITMAP_BLOCK_OFFSET + INODE_BITMAP_BLOCK_CNT)
#define INODE_BLOCK_OFFSET (DNODE_BITMAP_BLOCK_OFFSET + DNODE_BITMAP_BLOCK_CNT)
#define DNODE_BLOCK_OFFSET (INODE_BLOCK_OFFSET + INODE_BLOCK_CNT)

#define INODE_PER_BLOCK (SECTOR_SIZE / sizeof(disk_inode))

typedef struct index_entry
{
    filename_t name;
    int inode_id;
} index_entry;

#define INDEX_ENTRY_PER_BLOCK (SECTOR_SIZE / sizeof(index_entry))

typedef struct super_block
{
    int magic_num;
    int block_cnt;
    int data_block_cnt;
    int file_cnt;
} super_block;

typedef struct system_inode
{
    filename_t name;
    int inode_id;
    int ref_cnt;
    disk_inode disknode;
    system_inode *father;
    linked_list subnodes;

} system_inode;

typedef struct file_system
{
    char sector0[SECTOR_SIZE];
    super_block *superblock;
    system_inode root_dir_node;
    system_inode *curr_dir_node;
} file_system;

void init_file_system();

enum file_mode
{
    RDONLY = 0,
    WRONLY = 1 << 0,
    RDWR = 1 << 1,
    CREATE = 1 << 9,
    TRUNC = 1 << 10
};

typedef struct proc_inode
{
    int fd;
    int access_mode;
    size_t offset;
    system_inode *inode;
} proc_inode;

#define MAX_PROC_FD_CNT 32

typedef struct proc_files
{
    int avaliable_fd_cnt;
    int used_fd_cnt;
    int fds[MAX_PROC_FD_CNT];
    proc_inode inodes[MAX_PROC_FD_CNT];
} proc_files;

void init_proc_files(proc_files *pf);

int fs_open(proc_files *pf, const char *path, int oflag);
int fs_close(proc_files *pf, int fd);
int fs_lseek(proc_files *pf, int fd, size_t offset, int whence);
size_t fs_read(proc_files *pf, int fd, char *buf, size_t nbytes);
size_t fs_write(proc_files *pf, int fd, char *buf, size_t nbytes);

#endif