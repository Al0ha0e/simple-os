#include "libs/syscall.h"

char *c = "user";

void main()
{
    while (1)
    {
        sys_write(1, c, 4);
        sys_sched_yield();
    }
}