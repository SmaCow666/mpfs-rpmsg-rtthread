/*
 * Copyright (c) 2024, RT-Thread PolarFire SoC Port
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-06-15     codex        First board port for MPFS Icicle Kit
 */

#ifndef BOARD_H_
#define BOARD_H_

#include <stdint.h>

/* ==============================================================
 * Board definitions for PolarFire SoC Icicle Kit
 *
 * Mimics the layout of FreeRTOSConfig.h & demo_main.c definitions
 * for consistent SoC access.
 * ============================================================== */

/* ---------- CLINT (Core Local Interrupter) registers ---------- */
#define CLINT_MTIME           ((volatile uint64_t*)0x0200bff8)
#define CLINT_MTIMECMP_BASE   ((volatile uint64_t*)0x02004000)

/* ---------- System clock ---------- */
#define SYSCLK_FREQ           100000000UL    /* 100 MHz CPU clock */
#define CLINT_FREQ            1000000UL      /* 1 MHz CLINT tick  */

/* ---------- OS Tick configuration ---------- */
#define OS_TICK_RATE_HZ       1000

/* ---------- Heap section symbols (from linker script) ---------- */
extern unsigned char __heap_start;
extern unsigned char __heap_end;

/* ---------- UART ---------- */
extern uintptr_t g_mss_uart1_lo;
extern uintptr_t g_mss_uart3_lo;

/* ---------- Board initialization ---------- */
void rt_hw_board_init(void);

#endif /* BOARD_H_ */
