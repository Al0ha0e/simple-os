#ifndef TIME_H
#define TIME_H

#include "types.h"

typedef enum timer_type
{
    TIMER_SCHEDULE
} timer_type;

typedef struct timer_info
{
    uint64 id;
    uint64 expire;
    timer_type type;
} timer_info;

uint64 set_timer(uint64 expire, timer_type type);

void remove_timer(uint64 id);

timer_info *timer_expire();

timer_info *get_nearest_timer();

void show_timers();

#endif