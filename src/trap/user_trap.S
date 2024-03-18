.macro SAVE reg, offset
    sd  \reg, \offset*8(sp)
.endm

.macro LOAD reg, offset
    ld  \reg, \offset*8(sp)
.endm

.section .text.trampoline

.globl _user_trap
_user_trap:
    csrrw   sp, sscratch, sp
    addi    sp, sp, -40*8
    SAVE    x1, 1
    csrrw   x1, sscratch, x0
    SAVE    x1, 2
    SAVE    x3, 3
    SAVE    x4, 4
    SAVE    x5, 5
    SAVE    x6, 6
    SAVE    x7, 7
    SAVE    x8, 8
    SAVE    x9, 9
    SAVE    x10, 10
    SAVE    x11, 11
    SAVE    x12, 12
    SAVE    x13, 13
    SAVE    x14, 14
    SAVE    x15, 15
    SAVE    x16, 16
    SAVE    x17, 17
    SAVE    x18, 18
    SAVE    x19, 19
    SAVE    x20, 20
    SAVE    x21, 21
    SAVE    x22, 22
    SAVE    x23, 23
    SAVE    x24, 24
    SAVE    x25, 25
    SAVE    x26, 26
    SAVE    x27, 27
    SAVE    x28, 28
    SAVE    x29, 29
    SAVE    x30, 30
    SAVE    x31, 31

    csrr    t0, sstatus
    csrr    t1, sepc
    csrr    a1, scause
    csrr    a2, stval

    SAVE    t0, 32
    SAVE    t1, 33
    SAVE    a1, 34
    SAVE    a2, 35
    SAVE    sp, 38
    
    LOAD    t0, 36
    LOAD    t1, 39
    LOAD    sp, 37
    
    mv      a0, sp
    addi    a1, x0, 1

    csrw    satp, t0
    sfence.vma

    jr      t1

.globl _user_restore
_user_restore:
    csrw    satp, a1
    sfence.vma
    
    addi    t0, a0, 40*8
    csrw    sscratch, t0
    mv      sp, a0
    
    LOAD    t0, 32
    LOAD    t1, 33
    LOAD    a1, 34
    LOAD    a2, 35

    csrw    sstatus, t0
    csrw    sepc, t1
    csrw    scause, a1
    csrw    stval, a2

    LOAD    x1, 1
    LOAD    x3, 3
    LOAD    x4, 4
    LOAD    x5, 5
    LOAD    x6, 6
    LOAD    x7, 7
    LOAD    x8, 8
    LOAD    x9, 9
    LOAD    x10, 10
    LOAD    x11, 11
    LOAD    x12, 12
    LOAD    x13, 13
    LOAD    x14, 14
    LOAD    x15, 15
    LOAD    x16, 16
    LOAD    x17, 17
    LOAD    x18, 18
    LOAD    x19, 19
    LOAD    x20, 20
    LOAD    x21, 21
    LOAD    x22, 22
    LOAD    x23, 23
    LOAD    x24, 24
    LOAD    x25, 25
    LOAD    x26, 26
    LOAD    x27, 27
    LOAD    x28, 28
    LOAD    x29, 29
    LOAD    x30, 30
    LOAD    x31, 31

    LOAD    sp, 2
    sret