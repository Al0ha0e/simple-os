#ifndef DS_H
#define DS_H

#include "types.h"

typedef struct vector
{
    size_t item_size;
    unsigned int capacity;
    unsigned int count;
    void *buffer;
} vector;

void init_vector(vector *v, size_t item_size, unsigned int capacity);
void vector_push_back(vector *v, void *item);
void *vector_extend(vector *v);
void *vector_get_item(vector *v, unsigned int id);

#endif