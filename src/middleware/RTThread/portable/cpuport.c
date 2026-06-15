/*
 * Copyright (c) 2024, RT-Thread PolarFire SoC Port
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-06-15     codex        First port: PolarFire SoC CPU port
 *
 * cpuport.c - Board-specific CPU port extensions.
 *
 * This file provides board-level port hooks for the PolarFire SoC,
 * analogous to FreeRTOS port.c. The generic RISC-V cpuport.c lives
 * in rt-thread/libcpu/risc-v/rv64/cpuport.c (via git subtree).
 *
 * Responsibilities:
 *   - Stack initialization for RT-Thread threads
 *   - System tick timer setup (CLINT MTimer)
 *   - Interrupt controller (PLIC) integration
 */

#include <rtthread.h>
#include <rthw.h>
#include "board.h"
#include "rtt_port.h"

/* ==============================================================
 * RT-Thread Thread Stack Initialization
 *
 * Sets up the stack frame so that rt_hw_context_switch_to() can
 * start the thread. Stack layou matches RISC-V rv64 ABI:
 *   0(sp):   ra
 *   1(sp):   tp (x4)
 *   2(sp):   gp (x3) - reserved
 *   3(sp):   t0 (x5)
 *   ...
 *   9(sp):   a0 (x10) = parameter
 *   ...
 *   31(sp):  mepc = entry
 * ============================================================== */

rt_uint8_t *rt_hw_stack_init(void *tentry,
                              void *parameter,
                              rt_uint8_t *stack_addr,
                              void *texit)
{
    rt_uint64_t *stk;

    /* Align stack to 8-byte boundary */
    stk = (rt_uint64_t *)(((rt_uint64_t)stack_addr + 7) & ~7);
    /* Move down to make room for 34 saved registers */
    stk -= 34;

    /* Machine return address (program counter) */
    stk[31] = (rt_uint64_t)tentry;

    /* Machine status register value:
     * Set MPP bits to 0b11 (machine mode) and MPIE (0x80) */
    stk[32] = 0x0000000000001880UL;

    /* Save parameter in a0 (x10) register slot */
    stk[10] = (rt_uint64_t)parameter;

    /* Save exit function in ra (x1) register slot */
    stk[1]  = (rt_uint64_t)texit;

    /* Save thread pointer (x4/tp) - use sp itself as tp for now */
    stk[4]  = (rt_uint64_t)stk;

    /* Return the new stack pointer */
    return (rt_uint8_t *)stk;
}

/* ==============================================================
 * System Tick Setup
 *
 * Uses CLINT MTimer (same as FreeRTOS port).
 * The tick interrupt handler is board.c:rt_hw_tick_handler().
 * ============================================================== */

void rt_hw_tick_init(void) __attribute__((weak, alias("rt_hw_board_tick_init")));
