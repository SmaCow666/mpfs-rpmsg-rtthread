/*
 * Copyright (c) 2024, RT-Thread PolarFire SoC Port
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-06-15     codex        Rewritten to reuse MPFS HAL (SysTick_Config
 *                              + U54_1_sysTick_IRQHandler chain)
 *
 * 移植策略：最高可复用性方案
 *
 *   复用 MPFS HAL 的完整 Tick 链路，不自行操作 CLINT 寄存器：
 *     SysTick_Config() → handle_m_timer_interrupt()
 *     → U54_1_sysTick_IRQHandler() → rt_tick_increase()
 *
 *   HAL 自动完成：第一次 MTIMECMP 编程、每 Tick 的 MTIMECMP 重编程、
 *   MTIE 开关、mstatus.MIE 管理。
 *
 *   rt_kprintf() 底层通过 rt_hw_console_output() 输出，
 *   直接调用已编译的 MSS UART 裸机驱动。
 */

#include <rtthread.h>
#include <rthw.h>

/* MPFS HAL：CLINT 结构体、MTIMECMP 操作宏、mie 位定义 */
#include "mpfs_hal/mss_hal.h"

/* MSS UART 裸机驱动（已由平台层 Makefile 编译） */
#include "drivers/mss/mss_mmuart/mss_uart.h"

#include "board.h"

/* ============================================================
 * HAL Tick 链路 — 钩子点 ⭐
 *
 * 被 handle_m_timer_interrupt() 自动调用（mss_clint.c:105）。
 * handle_m_timer_interrupt 会自动完成：
 *   1. 关 MTIE（防重入）
 *   2. 调 U54_1_sysTick_IRQHandler()  ← 此行
 *   3. MTIMECMP = MTIME + g_systick_increment（自动重编程）
 *   4. 开 MTIE
 *
 * 移植者只需要在弱函数中填入 rt_tick_increase()。
 * ============================================================ */
void U54_1_sysTick_IRQHandler(void)
{
    rt_tick_increase();
}

/* ============================================================
 * rt_hw_board_init() — 板级初始化入口
 *
 * 被 RT-Thread 启动流程自动调用（在 rt_system_scheduler_start()
 * 之前，需要在应用层手动调用或者通过组件初始化链触发）。
 *
 * 步骤说明：
 *   1. Tick 初始化 → 用 SysTick_Config() 设置 CLINT 定时器
 *      注意：SysTick_Config() 内含 __enable_irq() 会打开 MIE。
 *      在单核场景下这是安全的：Scheduler 启动前无 Task 运行，
 *      Timer 中断即使触发也只是调用 rt_tick_increase() 做空计数。
 *      Scheduler 启动后 mret 会重新同步 MIE。
 *   2. 堆初始化 → 告知内核可用内存范围
 *   3. 控制台 → 绑定 UART 输出
 * ============================================================ */
void rt_hw_board_init(void)
{
    /* 1) 系统 Tick 初始化：复用 HAL 的 SysTick_Config()
     *
     * SysTick_Config 执行：
     *   a) 从 mss_sw_config.h 读取 HART1_TICK_RATE_MS
     *   b) 计算 g_systick_increment[hart_id]
     *   c) CLINT->MTIMECMP[hart_id] = CLINT->MTIME + increment
     *   d) set_csr(mie, MIP_MTIP)      -- 开 MTIE
     *   e) __enable_irq()               -- 开 MIE
     */
    SysTick_Config();

    /* 2) 系统堆初始化
     *
     * 堆起始 → 当前 BSS 段之后的内存（__bss_end）
     * 堆结束 → 在 _end 基础上追加 512KB
     * 此区域在链接脚本中未被显式使用，可以安全分配。
     */
    extern unsigned char __bss_end;
    extern unsigned char _end;
    rt_system_heap_init(
        (void *)&__bss_end,
        (void *)((unsigned long)&_end + 512UL * 1024UL));

    /* 3) 组件初始化（空函数，预留用户扩展） */
    rt_components_board_init();
}

/* ============================================================
 * rt_hw_console_output() — RT-Thread 控制台输出绑定
 *
 * rt_kprintf() 底层最终调用此函数输出字符。
 * 直接复用平台层已编译的 MSS UART 裸机驱动，无 RTOS 依赖。
 *
 * 注意：UART 外设时钟需要在 e51() 或 u54_1() 中使能：
 *   mss_config_clk_rst(MSS_PERIPH_MMUART0, hart_id, PERIPHERAL_ON);
 * ============================================================ */
void rt_hw_console_output(const char *str)
{
    /* 使用 MMUART0 作为控制台输出 */
    MSS_UART_polled_tx_string(&g_mss_uart0_lo, str);
}
