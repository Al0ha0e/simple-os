#ifndef RISCV_H
#define RISCV_H

#include "../libs/types.h"

// Machine Status Register, mstatus

#define MSTATUS_MPP_MASK (3L << 11) // previous mode.
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3) // machine-mode interrupt enable.

static inline uint64 r_mstatus()
{
    uint64 ret;
    asm volatile("csrr %0, mstatus"
                 : "=r"(ret));
    return ret;
}

static inline void w_mstatus(uint64 x)
{
    asm volatile("csrw mstatus, %0"
                 :
                 : "r"(x));
}

static inline uint64 sbi_call(uint64 arg0,uint64 arg1,uint64 arg2,uint64 which){
    uint64 ret;
    asm volatile("mv a7, a3\n\t"
                "ecall\n\t"
                "mv %0, a0":"=r"(ret)::"memory");
    // asm volatile("ecall"
    //     : "=r"(ret));
    //     //: "{x10}" (arg0), "{x11}" (arg1), "{x12}" (arg2), "{x17}" (which)
    //     //: "memory");     // 如果汇编可能改变内存，则需要加入 memory 选项
    //     // : "volatile");
    return ret;
}

#endif