#include "block_manager.h"
#include "driver.h"
#include "../libs/libfuncs.h"

typedef struct _inner_block_info
{
    unsigned int occupied : 1;
    unsigned int dirty : 1;
    unsigned int next : 30;
} _inner_block_info;

typedef union inner_block_info
{
    _inner_block_info s;
    unsigned int i;
} inner_block_info;

typedef struct block_info
{
    int id;
    inner_block_info info;
} block_info;

static int free_block_cnt;
static int occupied_head;
static int occupied_tail;
static char block_pool[SECTOR_SIZE * BM_CAPACITY];
static char occu_bitmap[BM_CAPACITY >> 3];
static block_info infos[BM_CAPACITY];

void init_block_manager()
{
    free_block_cnt = BM_CAPACITY;
    occupied_head = occupied_tail = -1;
}

static int find_block(int id)
{
    for (int i = 0; i < BM_CAPACITY; i++)
        if (infos[i].info.s.occupied && infos[i].id == id)
            return i;

    return -1;
}

static int alloc_block()
{
    int ret;
    if (free_block_cnt)
    {
        --free_block_cnt;
        for (int i = 0; i < BM_CAPACITY; i++)
        {
            if (!infos[i].info.s.occupied)
            {
                ret = i;
                break;
            }
        }
    }
    else
    {
        ret = occupied_head;
        occupied_head = infos[occupied_head].info.s.next;
        if (infos[ret].info.s.dirty)
            write_sector(infos[ret].id, block_pool + (ret << BM_CAPACITY_LOG2));
    }

    infos[ret].info.i = -1;
    infos[ret].info.s.dirty = 0;
    if (occupied_tail == -1)
    {
        occupied_head = occupied_tail = ret;
    }
    else
    {
        infos[occupied_tail].info.s.next = ret;
        occupied_tail = ret;
    }
    _memset(ret, 0, SECTOR_SIZE);
    return ret;
}

char *read_block(int id)
{
    int bid = find_block(id);
    if (bid != -1)
        return block_pool + (bid << BM_CAPACITY_LOG2);
    bid = alloc_block();
    infos[bid].id = id;
    read_sector(id, block_pool + (bid << BM_CAPACITY_LOG2));
}

void write_block(int id, char *src)
{
    int bid = find_block(id);
    if (bid == -1)
    {
        bid = alloc_block();
        infos[bid].id = id;
    }
    infos[bid].info.s.dirty = 1;
    memcpy(block_pool + (bid << BM_CAPACITY_LOG2), src, SECTOR_SIZE);
}

void flush(int id)
{
    int bid = find_block(id);
    if (bid == -1 || !infos[bid].info.s.dirty)
        return;
    write_sector(id, block_pool + (bid << BM_CAPACITY_LOG2));
    infos[bid].info.s.dirty = 0;
}

char *partial_read_block(int id, int offset)
{
    return read_block(id) + offset;
}

void partial_write_block(int id, int offset, char *src, int cnt)
{
    char *dst = partial_read_block(id, offset);
    memcpy(dst, src, cnt);
}