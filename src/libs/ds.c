#include "ds.h"
#include "../memory/memory.h"
#include "../libs/libfuncs.h"

void init_vector(vector *v, size_t item_size, uint32 capacity)
{
    v->item_size = item_size;
    v->capacity = capacity;
    v->count = 0;
    v->buffer = malloc(item_size * capacity);
}

void push_back(vector *v, void *item)
{
    if (v->capacity == v->count)
    {
        void *new_buffer = malloc(v->capacity * v->item_size * 2);
        memcpy(new_buffer, v->buffer, v->capacity * v->item_size);
        free(v->buffer);
        v->buffer = new_buffer;
        v->capacity *= 2;
    }

    memcpy(((char *)v->buffer) + v->item_size * v->count, item, v->item_size);
    ++v->count;
}

void *get_item(vector *v, uint32 id)
{
    if (id >= v->count)
        return NULL;
    return ((char *)v->buffer) + v->item_size * id;
}