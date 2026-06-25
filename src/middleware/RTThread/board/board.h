/*
 * Copyright (c) 2024, RT-Thread PolarFire SoC Port
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-06-15             Restructured to reuse MPFS HAL CLINT
 */

#ifndef BOARD_H_
#define BOARD_H_

#include <stdint.h>

/* System clock */
#define CLINT_FREQ            1000000UL

/* Peripheral instances */
extern mss_uart_instance_t  g_mss_uart0_lo;
extern mss_uart_instance_t  g_mss_uart1_lo;

/* Background console print thread */
#define CONSOLE_BUF_SIZE        512
#define CONSOLE_PRINT_PRIO      RT_THREAD_PRIORITY_MAX - 1
#define CONSOLE_PRINT_STACK     2048

void rt_hw_console_init(void);
void console_print_entry(void *parameter);
void rt_hw_board_init(void);

#endif /* BOARD_H_ */
