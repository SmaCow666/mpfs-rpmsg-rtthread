/*
 * Copyright (c) 2024, RT-Thread PolarFire SoC Port
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * rtt_demo.c — RT-Thread Nano minimal startup test
 *
 * 启动测试流程：
 *   1. 初始化 UART 控制台（MMUART0）
 *   2. 调用 rt_hw_board_init() → SysTick_Config() + 堆初始化
 *   3. 关闭 MIE（SysTick_Config 过早开了它）
 *   4. 初始化调度器和定时器子系统
 *   5. 创建两个测试线程交替打印
 *   6. 启动调度器（不返回）
 *
 * 调用位置：u54_1.c 中的 start_rtt_demo()
 * 替代对象：start_demo() [FreeRTOS]
 */

#include <rtthread.h>

/* MPFS HAL：外设时钟、PLIC */
#include "mpfs_hal/mss_hal.h"

/* MSS UART 裸机驱动 */
#include "drivers/mss/mss_mmuart/mss_uart.h"

#include "board.h"

/* ============================================================
 * 测试线程 1 — 每 1 秒打印一次
 * ============================================================ */
static void thread1_entry(void *parameter)
{
    int count = 0;
    while (1)
    {
        rt_kprintf("[RTT] thread1: count=%d  tick=%d\n",
                   count++, rt_tick_get());
        rt_thread_mdelay(1000);
    }
}

/* ============================================================
 * 测试线程 2 — 每 0.5 秒打印一次
 * ============================================================ */
static void thread2_entry(void *parameter)
{
    while (1)
    {
        rt_kprintf("[RTT] thread2: alive  tick=%d\n", rt_tick_get());
        rt_thread_mdelay(500);
    }
}

/* ============================================================
 * start_rtt_demo() — RT-Thread Nano 启动测试入口
 *
 * 需要在 u54_1.c 中替换 start_demo() 调用为此函数。
 * ============================================================ */
void start_rtt_demo(void)
{
    rt_thread_t tid1;
    rt_thread_t tid2;
    rt_base_t   level;

    /* ---- Step 1: 初始化 UART 控制台 ---- */
    /* 使能 MMUART0 外设时钟（如尚未使能） */
    mss_config_clk_rst(MSS_PERIPH_MMUART0,
                       (uint8_t)read_csr(mhartid),
                       PERIPHERAL_ON);

    /* 初始化 MMUART0：115200-8N1 */
    MSS_UART_init(&g_mss_uart0_lo,
                  MSS_UART_115200_BAUD,
                  MSS_UART_DATA_8_BITS |
                  MSS_UART_NO_PARITY  |
                  MSS_UART_ONE_STOP_BIT);

    /* 初始化 PLIC（默认配置即可） */
    PLIC_init();

    /* ---- Step 2: 板级初始化 ---- */
    rt_hw_board_init();

    /* ---- Step 3: 关闭 MIE（SysTick_Config 内部调了 __enable_irq） ---- */
    level = rt_hw_interrupt_disable();

    /* ---- Step 4: 初始化调度器和定时器 ---- */
    rt_system_scheduler_init();
    rt_system_timer_init();

    /* ---- Step 5: 创建测试线程 ---- */
    tid1 = rt_thread_create("t1",
                            thread1_entry,
                            RT_NULL,
                            1024,
                            10,
                            10);
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);

    tid2 = rt_thread_create("t2",
                            thread2_entry,
                            RT_NULL,
                            1024,
                            15,
                            10);
    if (tid2 != RT_NULL)
        rt_thread_startup(tid2);

    /* ---- Step 6: 恢复中断状态 + 启动调度器 ---- */
    rt_hw_interrupt_enable(level);

    rt_kprintf("[RTT] Scheduler starting...\n");

    rt_system_scheduler_start();

    /* 不应到达此处 */
    while (1);
}
