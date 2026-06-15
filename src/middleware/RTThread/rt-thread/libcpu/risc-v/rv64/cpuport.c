/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NOTE: Scaffold file for PolarFire SoC. Authoritative source:
 *   https://github.com/RT-Thread/rtthread-nano.git
 *   Path: libcpu/risc-v/rv64/cpuport.c
 */

#include <rtthread.h>
#include <rthw.h>
#include "cpuport.h"

/* ============================================================
 * rt_hw_stack_init()
 * Initialize the stack of a new thread.
 * Stack frame layout (34 x 8 bytes for rv64):
 *   [0]  ra   (x1)   -> texit
 *   [1]  sp   (x2)
 *   [2]  gp   (x3)
 *   [3]  tp   (x4)
 *   [4]  t0   (x5)
 *   ...
 *   [10] a0   (x10)  -> parameter
 *   [11] a1   (x11)
 *   ...
 *   [31] mepc        -> tentry
 *   [32] mstatus     -> 0x1880 (MPP=3, MPIE)
 * ============================================================ */
rt_uint8_t *rt_hw_stack_init(void *tentry,
                              void *parameter,
                              rt_uint8_t *stack_addr,
                              void *texit)
{
    rt_uint64_t *stk;

    /* Align to 8-byte boundary, then reserve space for 34 registers */
    stk = (rt_uint64_t *)(((rt_uint64_t)stack_addr + 7) & ~7);
    stk -= 34;

    /* ra -> thread exit function */
    stk[0]  = (rt_uint64_t)texit;
    /* tp (x4) -> thread pointer points to its own TCB */
    stk[3]  = (rt_uint64_t)0;
    /* a0 (x10) -> parameter */
    stk[10] = (rt_uint64_t)parameter;
    /* mepc -> thread entry */
    stk[31] = (rt_uint64_t)tentry;
    /* mstatus -> machine mode, interrupts enabled after mret */
    stk[32] = (rt_uint64_t)0x0000000000001880UL;

    return (rt_uint8_t *)stk;
}
