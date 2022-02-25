#include "libs/syscall.h"
#include "stdio.h"

char *c = "user";

void main()
{
    printf("pid %d\n", sys_getpid());
    uint64 pid = sys_fork();
    printf("pid %d %d\n", pid, sys_getpid());
    while (1)
    {
    }
}