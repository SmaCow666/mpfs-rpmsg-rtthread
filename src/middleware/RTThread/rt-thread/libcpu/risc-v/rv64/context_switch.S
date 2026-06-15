/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NOTE: Scaffold file for PolarFire SoC. Authoritative source:
 *   https://github.com/RT-Thread/rtthread-nano.git
 *   Path: libcpu/risc-v/rv64/context_switch.S
 *
 * RISC-V rv64 context switch assembly.
 * Hardware CLINT MTimer interrupt handler is TIMER_CMP_INT.
 */

#include "cpuport.h"

#if __riscv_xlen == 64
# define STORE    sd
# define LOAD     ld
# define REGBYTES 8
#else
# define STORE    sw
# define LOAD     lw
# define REGBYTES 4
#endif

#define CONTEXT_REG_COUNT 34
#define MSTATUS_PRV1 0x1880

.text
.align 4
.globl rt_hw_interrupt_disable
.globl rt_hw_interrupt_enable
.globl rt_hw_context_switch
.globl rt_hw_context_switch_to
.globl rt_hw_context_switch_interrupt

/* ============================================================
 * rt_hw_interrupt_disable()
 * Disable machine-mode interrupts (mie bit in mstatus).
 * Returns previous mstatus value.
 * ============================================================ */
rt_hw_interrupt_disable:
    csrr    a0, mstatus
    csrci   mstatus, 8
    ret

/* ============================================================
 * rt_hw_interrupt_enable(level)
 * Restore mstatus to given level (enabling/disabling mie bit).
 * ============================================================ */
rt_hw_interrupt_enable:
    csrw    mstatus, a0
    ret

/* ============================================================
 * rt_hw_context_switch(from, to)
 *   a0 = &from_thread->sp
 *   a1 = &to_thread->sp
 * Save current context, load target context.
 * ============================================================ */
rt_hw_context_switch:
    /* Disable interrupts during context switch */
    csrci   mstatus, 8

    /* Save context to current stack */
    addi    sp, sp, -REGBYTES * CONTEXT_REG_COUNT
    STORE   x1, 0x0(sp)
    STORE   x2, 1 * REGBYTES(sp)
    STORE   x3, 2 * REGBYTES(sp)
    STORE   x4, 3 * REGBYTES(sp)
    STORE   x5, 4 * REGBYTES(sp)
    STORE   x6, 5 * REGBYTES(sp)
    STORE   x7, 6 * REGBYTES(sp)
    STORE   x8, 7 * REGBYTES(sp)
    STORE   x9, 8 * REGBYTES(sp)
    STORE   x10, 9 * REGBYTES(sp)
    STORE   x11, 10 * REGBYTES(sp)
    STORE   x12, 11 * REGBYTES(sp)
    STORE   x13, 12 * REGBYTES(sp)
    STORE   x14, 13 * REGBYTES(sp)
    STORE   x15, 14 * REGBYTES(sp)
    STORE   x16, 15 * REGBYTES(sp)
    STORE   x17, 16 * REGBYTES(sp)
    STORE   x18, 17 * REGBYTES(sp)
    STORE   x19, 18 * REGBYTES(sp)
    STORE   x20, 19 * REGBYTES(sp)
    STORE   x21, 20 * REGBYTES(sp)
    STORE   x22, 21 * REGBYTES(sp)
    STORE   x23, 22 * REGBYTES(sp)
    STORE   x24, 23 * REGBYTES(sp)
    STORE   x25, 24 * REGBYTES(sp)
    STORE   x26, 25 * REGBYTES(sp)
    STORE   x27, 26 * REGBYTES(sp)
    STORE   x28, 27 * REGBYTES(sp)
    STORE   x29, 28 * REGBYTES(sp)
    STORE   x30, 29 * REGBYTES(sp)
    STORE   x31, 30 * REGBYTES(sp)
    /* Store mepc (return address to restored task) */
    csrr    t0, mepc
    STORE   t0, 31 * REGBYTES(sp)

    /* sp = from_thread->sp */
    STORE   sp, 0(a0)

    /* Load target thread sp */
    LOAD    sp, 0(a1)

    /* Restore context */
    LOAD    t0, 31 * REGBYTES(sp)
    csrw    mepc, t0
    /* Set machine mode */
    li      t0, MSTATUS_PRV1
    csrs    mstatus, t0

    LOAD    x1, 0x0(sp)
    LOAD    x4, 3 * REGBYTES(sp)
    LOAD    x5, 4 * REGBYTES(sp)
    LOAD    x6, 5 * REGBYTES(sp)
    LOAD    x7, 6 * REGBYTES(sp)
    LOAD    x8, 7 * REGBYTES(sp)
    LOAD    x9, 8 * REGBYTES(sp)
    LOAD    x10, 9 * REGBYTES(sp)
    LOAD    x11, 10 * REGBYTES(sp)
    LOAD    x12, 11 * REGBYTES(sp)
    LOAD    x13, 12 * REGBYTES(sp)
    LOAD    x14, 13 * REGBYTES(sp)
    LOAD    x15, 14 * REGBYTES(sp)
    LOAD    x16, 15 * REGBYTES(sp)
    LOAD    x17, 16 * REGBYTES(sp)
    LOAD    x18, 17 * REGBYTES(sp)
    LOAD    x19, 18 * REGBYTES(sp)
    LOAD    x20, 19 * REGBYTES(sp)
    LOAD    x21, 20 * REGBYTES(sp)
    LOAD    x22, 21 * REGBYTES(sp)
    LOAD    x23, 22 * REGBYTES(sp)
    LOAD    x24, 23 * REGBYTES(sp)
    LOAD    x25, 24 * REGBYTES(sp)
    LOAD    x26, 25 * REGBYTES(sp)
    LOAD    x27, 26 * REGBYTES(sp)
    LOAD    x28, 27 * REGBYTES(sp)
    LOAD    x29, 28 * REGBYTES(sp)
    LOAD    x30, 29 * REGBYTES(sp)
    LOAD    x31, 30 * REGBYTES(sp)

    addi    sp, sp, REGBYTES * CONTEXT_REG_COUNT
    mret

/* ============================================================
 * rt_hw_context_switch_to(to)
 *   a0 = &to_thread->sp
 * Start the first thread: no context to save.
 * ============================================================ */
rt_hw_context_switch_to:
    /* Load target thread sp */
    LOAD    sp, 0(a0)
    /* Restore context and mret to the new thread */
    LOAD    t0, 31 * REGBYTES(sp)
    csrw    mepc, t0
    li      t0, MSTATUS_PRV1
    csrs    mstatus, t0

    LOAD    x1, 0x0(sp)
    LOAD    x4, 3 * REGBYTES(sp)
    LOAD    x5, 4 * REGBYTES(sp)
    LOAD    x6, 5 * REGBYTES(sp)
    LOAD    x7, 6 * REGBYTES(sp)
    LOAD    x8, 7 * REGBYTES(sp)
    LOAD    x9, 8 * REGBYTES(sp)
    LOAD    x10, 9 * REGBYTES(sp)
    LOAD    x11, 10 * REGBYTES(sp)
    LOAD    x12, 11 * REGBYTES(sp)
    LOAD    x13, 12 * REGBYTES(sp)
    LOAD    x14, 13 * REGBYTES(sp)
    LOAD    x15, 14 * REGBYTES(sp)
    LOAD    x16, 15 * REGBYTES(sp)
    LOAD    x17, 16 * REGBYTES(sp)
    LOAD    x18, 17 * REGBYTES(sp)
    LOAD    x19, 18 * REGBYTES(sp)
    LOAD    x20, 19 * REGBYTES(sp)
    LOAD    x21, 20 * REGBYTES(sp)
    LOAD    x22, 21 * REGBYTES(sp)
    LOAD    x23, 22 * REGBYTES(sp)
    LOAD    x24, 23 * REGBYTES(sp)
    LOAD    x25, 24 * REGBYTES(sp)
    LOAD    x26, 25 * REGBYTES(sp)
    LOAD    x27, 26 * REGBYTES(sp)
    LOAD    x28, 27 * REGBYTES(sp)
    LOAD    x29, 28 * REGBYTES(sp)
    LOAD    x30, 29 * REGBYTES(sp)
    LOAD    x31, 30 * REGBYTES(sp)

    addi    sp, sp, REGBYTES * CONTEXT_REG_COUNT
    mret

/* ============================================================
 * rt_hw_context_switch_interrupt(from, to)
 *   a0 = &from_thread->sp
 *   a1 = &to_thread->sp
 * Context switch called from interrupt context.
 * ============================================================ */
rt_hw_context_switch_interrupt:
    csrci   mstatus, 8
    STORE   sp, 0(a0)
    LOAD    sp, 0(a1)
    li      t0, MSTATUS_PRV1
    csrs    mstatus, t0
    ret
