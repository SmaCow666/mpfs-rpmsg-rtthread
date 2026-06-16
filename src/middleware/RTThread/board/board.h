/*
 * Copyright (c) 2024, RT-Thread PolarFire SoC Port
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-06-15     codex        Restructured to reuse MPFS HAL CLINT
 */

#ifndef BOARD_H_
#define BOARD_H_

/* ==============================================================
 * Board header for PolarFire SoC Icicle Kit
 *
 * 复用策略：直接使用 MPFS HAL 的 CLINT 结构体和寄存器定义。
 * 不自行定义 CLINT 地址宏，避免与 HAL 冲突。
 * ============================================================== */

#include <stdint.h>
// #include "src/platform/drivers/mss/mss_mmuart/mss_uart.h"

/* ---------- CLINT 时钟 ---------- */
#define CLINT_FREQ            1000000UL      /* 1 MHz CLINT tick */
#define OS_TICK_RATE_HZ       1000           /* RT-Thread system tick rate */

/* ---------- UART 实例 ---------- */
extern mss_uart_instance_t  g_mss_uart0_lo;
extern mss_uart_instance_t  g_mss_uart1_lo;

/* ---------- 引导接口 ---------- */
void rt_hw_board_init(void);

#endif /* BOARD_H_ */
