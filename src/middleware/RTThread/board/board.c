/*
 * Copyright (c) 2024, RT-Thread PolarFire SoC Port
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-06-15     codex        First board port for MPFS Icicle Kit
 */

#include <rtthread.h>
#include <rthw.h>
#include "board.h"

/* ==============================================================
 * Board-level initialization for PolarFire SoC Icicle Kit
 *
 * Responsibilities:
 *   1. Configure the system tick (CLINT MTimer)
 *   2. Initialize the console (UART)
 *   3. Set up the heap region
 * ============================================================== */

/* ---------- CLINT timer access (same pattern as FreeRTOS port) -- */
static volatile uint64_t * const mtime   = CLINT_MTIME;
static volatile uint64_t * const timecmp = CLINT_MTIMECMP_BASE;

/* ---------- OS Tick handler (called from timer ISR) ------------- */
void rt_hw_tick_handler(void)
{
    /* Set next timer interrupt: timecmp = mtime + tick_interval */
    *timecmp = *mtime + (CLINT_FREQ / OS_TICK_RATE_HZ);

    /* Notify the kernel of one tick passed */
    rt_tick_increase();
}

/* ---------- OS Tick timer initialization ----------------------- */
static void rt_hw_tick_init(void)
{
    /* Set the first timer compare value */
    *timecmp = *mtime + (CLINT_FREQ / OS_TICK_RATE_HZ);

    /* Enable machine timer interrupt (MTIE bit in mie) */
    __asm volatile("csrs mie, %0" :: "r"(0x80));
}

/* ---------- Board initialization entry ------------------------- */
void rt_hw_board_init(void)
{
    /* Initialize OS tick timer */
    rt_hw_tick_init();

    /* Initialize heap memory */
#ifdef RT_USING_HEAP
    rt_system_heap_init((void*)&__heap_start, (void*)&__heap_end);
#endif

    /* Initialize console (UART) */
#ifdef RT_USING_CONSOLE
    /* UART init is handled in application layer via MPFS HAL */
    /* rt_console_set_device("uart"); */
#endif

    /* Call components initialization (if enabled) */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}
