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

typedef struct list_node
{
    struct list_node *prev;
    struct list_node *next;
    void *v;
} list_node;

typedef struct linked_list
{
    list_node *st;
    list_node *en;
} linked_list;

void list_insert_front(linked_list *list, list_node *node);
void list_insert_back(linked_list *list, list_node *node);
list_node *list_push_front(linked_list *list, void *v);
list_node *list_push_back(linked_list *list, void *v);
void list_remove(linked_list *list, list_node *node);
void list_delete(linked_list *list, list_node *node);
void list_pop_front(linked_list *list);
void list_pop_back(linked_list *list);
void list_move_to_front();
void list_move_to_back();
#endif