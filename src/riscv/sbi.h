#ifndef SBI_H
#define SBI_H

#include "riscv.h"

#define SBI_SET_TIMER 0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
#define SBI_SHUTDOWN 8

static inline void sbi_set_timer(uint64 duration)
{
    sbi_call(duration, 0, 0, SBI_SET_TIMER);
}

static inline void sbi_console_putchar(uint64 c)
{
    sbi_call(c, 0, 0, SBI_CONSOLE_PUTCHAR);
}

static inline void sbi_shutdown()
{
    sbi_call(0, 0, 0, SBI_SHUTDOWN);
}

#endif