#include "time.h"
#include "ds.h"
#include "../memory/memory.h"
#include "../riscv/sbi.h"

linked_list timer_list;

static uint64 timer_id;

uint64 set_timer(uint64 expire, timer_type type)
{
    if (expire <= r_time())
        return 0;

    uint64 ret = ++timer_id;
    timer_info *info = malloc(sizeof(timer_info));
    info->id = ret;
    info->type = type;
    info->expire = expire;
    list_node *prev = timer_list.st;
    if (!prev || expire < ((timer_info *)prev->v)->expire)
    {
        list_push_front(&timer_list, info);
        sbi_set_timer(expire);
        return ret;
    }
    for (list_node *now = prev->next;
         now && ((timer_info *)(now->v))->expire < info->expire;
         prev = now, now = now->next)
        ;
    list_push_after(&timer_list, prev, info);

    return ret;
}

void remove_timer(uint64 id)
{
    for (list_node *now = timer_list.st; now; now = now->next)
    {
        timer_info *info = (timer_info *)now->v;
        if (info->id == id)
        {
            if (now == timer_list.st)
            {
                if (now->next)
                    sbi_set_timer(((timer_info *)now->next->v)->expire);
                else
                    sbi_set_timer(~0L);
            }
            list_delete(&timer_list, now);
            free(info);
            break;
        }
    }
}

timer_info *timer_expire()
{
    return list_pop_front(&timer_list);
}

timer_info *get_nearest_timer()
{
    if (timer_list.st)
        return timer_list.st->v;
    return NULL;
}