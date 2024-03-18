#ifndef BLOCK_MANAGER_H
#define BLOCK_MANAGER_H

#define SECTOR_SIZE 512
#define SECTOR_SIZE_LOG2 9
#define BM_CAPACITY 64
#define BM_CAPACITY_LOG2 6

void init_block_manager();

char *read_block(int id);
void write_block(int id, char *src);
void flush(int id);

char *partial_read_block(int id, int offset);
void partial_write_block(int id, int offset, char *src, int cnt);

#endif