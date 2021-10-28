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

static inline void w_satp(uint64 x)
{
    asm volatile("csrw satp, %0"
                 :
                 : "r"(x));
}

// flush the TLB.
static inline void sfence_vma()
{
    // the zero, zero means flush all TLB entries.
    asm volatile("sfence.vma zero, zero");
}

static inline uint64 sbi_call(uint64 arg0, uint64 arg1, uint64 arg2, uint64 which)
{
    uint64 ret;
    asm volatile("mv a7, a3\n\t"
                 "ecall\n\t"
                 "mv %0, a0"
                 : "=r"(ret)::"memory");
    // asm volatile("ecall"
    //     : "=r"(ret));
    //     //: "{x10}" (arg0), "{x11}" (arg1), "{x12}" (arg2), "{x17}" (which)
    //     //: "memory");     // 如果汇编可能改变内存，则需要加入 memory 选项
    //     // : "volatile");
    return ret;
}

#define SATP_SV39 (8L << 60)
#define SV39_PNSEG_MASK ((1L << 9) - 1)

typedef struct sv39pte
{
    uint8 V : 1;
    uint8 R : 1;
    uint8 W : 1;
    uint8 X : 1;
    uint8 U : 1;
    uint8 G : 1;
    uint8 A : 1;
    uint8 D : 1;
    uint8 rsw : 2;
    uint64 ppn : 44;
    uint16 reserved : 10;
} sv39_pte;

#define SV39_X (1 << 3)
#define SV39_W (1 << 2)
#define SV39_R (1 << 1)

static inline void set_sv39pte_ppn(sv39_pte *pte, void *ppn)
{
    pte->ppn = (uint64)ppn >> 12;
}
static inline void set_sv39pte_D(sv39_pte *pte, uint8 flag) { pte->D = flag; }
static inline void set_sv39pte_A(sv39_pte *pte, uint8 flag) { pte->A = flag; }
static inline void set_sv39pte_G(sv39_pte *pte, uint8 flag) { pte->G = flag; }
static inline void set_sv39pte_U(sv39_pte *pte, uint8 flag) { pte->U = flag; }
static inline void set_sv39pte_X(sv39_pte *pte, uint8 flag) { pte->X = flag; }
static inline void set_sv39pte_W(sv39_pte *pte, uint8 flag) { pte->W = flag; }
static inline void set_sv39pte_R(sv39_pte *pte, uint8 flag) { pte->R = flag; }
static inline void set_sv39pte_V(sv39_pte *pte, uint8 flag) { pte->V = flag; }
static inline void init_sv39pte(sv39_pte *pte, void *ppn, uint8 flags)
{
    *((uint64 *)pte) = (uint64)flags | 1;
    set_sv39pte_ppn(pte, ppn);
}
#endif