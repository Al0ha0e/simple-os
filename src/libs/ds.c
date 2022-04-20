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

void vector_push_back(vector *v, void *item)
{
    memcpy(vector_extend(v), item, v->item_size);
}

void *vector_extend(vector *v)
{
    if (v->capacity == v->count)
    {
        void *new_buffer = malloc(v->capacity * v->item_size * 2);
        memcpy(new_buffer, v->buffer, v->capacity * v->item_size);
        free(v->buffer);
        v->buffer = new_buffer;
        v->capacity *= 2;
    }
    void *ret = ((char *)v->buffer) + v->item_size * v->count;
    ++v->count;
    return ret;
}

void *vector_get_item(vector *v, uint32 id)
{
    if (id >= v->count)
        return NULL;
    return ((char *)v->buffer) + v->item_size * id;
}

static list_node *make_list_node(void *v)
{
    list_node *ret = malloc(sizeof(list_node));
    ret->prev = ret->next = NULL;
    ret->v = v;
    return ret;
}

void list_insert_front(linked_list *list, list_node *node)
{
    if (!list->st)
    {
        list->st = list->en = node;
        return;
    }
    node->next = list->st;
    list->st->prev = node;
    list->st = node;
}

void list_insert_back(linked_list *list, list_node *node)
{
    if (!list->en)
    {
        list->st = list->en = node;
        return;
    }
    node->prev = list->en;
    list->en->next = node;
    list->en = node;
}

list_node *list_push_front(linked_list *list, void *v)
{
    list_node *ret = make_list_node(v);
    list_insert_front(list, ret);
    return ret;
}

list_node *list_push_back(linked_list *list, void *v)
{
    list_node *ret = make_list_node(v);
    list_insert_back(list, ret);
    return ret;
}

void list_remove(linked_list *list, list_node *node)
{
    if (node == list->st)
    {
        if (node == list->en)
            list->st = list->en = NULL;
        else
        {
            list->st = node->next;
            list->st->prev = NULL;
        }
    }
    else if (node == list->en)
    {
        list->en = node->prev;
        list->en->next = NULL;
    }
    else
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    node->prev = node->next = NULL;
}

void list_delete(linked_list *list, list_node *node)
{
    list_remove(list, node);
    free(node);
}

void list_pop_front(linked_list *list)
{
    if (list->st)
        list_delete(list, list->st);
}

void list_pop_back(linked_list *list)
{
    if (list->en)
        list_delete(list, list->en);
}

void list_move_to_front(linked_list *list, list_node *node)
{
    if (node == list->st)
        return;
    list_remove(list, node);
    list_insert_front(list, node);
}

void list_move_to_back(linked_list *list, list_node *node)
{
    if (node == list->en)
        return;
    list_remove(list, node);
    list_insert_back(list, node);
}