#include "libs/syscall.h"

char c[] = "hello";

void main()
{
    sys_write(1, c, 6);
    sys_exit(0);
}