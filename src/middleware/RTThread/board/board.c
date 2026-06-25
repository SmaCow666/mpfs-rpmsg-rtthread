/*
 * Copyright (c) 2024, RT-Thread PolarFire SoC Port
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 *
 */

#include <rtthread.h>
#include <rthw.h>

/* MPFS HAL */
#include "mpfs_hal/mss_hal.h"
/* MSS UART bare-metal driver */
#include "drivers/mss/mss_mmuart/mss_uart.h"
#include "board.h"

/* ==================================================================
 * UART TX ring buffer + background print thread
 *
 * rt_hw_console_output() writes to a ring buffer then releases a
 * semaphore to wake the low-priority print thread. The print thread
 * drains the buffer and writes to UART so the caller never blocks.
 * ================================================================== */

#include "ringbuf.h"


/* TX ring buffer instance (background print thread) */
static char _tx_buf[CONSOLE_BUF_SIZE];
static struct ringbuf _tx_rb;

/* Wake-up semaphore: print thread blocks here until data arrives */
static rt_sem_t _tx_wake = RT_NULL;

static void _tx_init(void)
{
    ringbuf_init(&_tx_rb, _tx_buf, CONSOLE_BUF_SIZE);
}

/* ----------------------------------------------------------------
 * Background print thread entry (lowest priority)
 *
 * Wait on semaphore -> lock UART -> drain ring buffer -> unlock.
 * ---------------------------------------------------------------- */
void console_print_entry(void *parameter)
{
    static rt_mutex_t uart_lock = RT_NULL;
    char txbuf;

    (void)parameter;

    if (uart_lock == RT_NULL) {
        uart_lock = rt_mutex_create("uart", RT_IPC_FLAG_PRIO);
        if (uart_lock == RT_NULL) return;
    }

    while (1) {
        rt_sem_take(_tx_wake, RT_WAITING_FOREVER);

        rt_mutex_take(uart_lock, RT_WAITING_FOREVER);
        for (;;) {
            int ch = ringbuf_getc(&_tx_rb);
            if (ch < 0) break;
            txbuf = (char)ch;
            MSS_UART_polled_tx(&g_mss_uart1_lo, (const uint8_t *)&txbuf, 1);
        }
        rt_mutex_release(uart_lock);
    }
}

/* ----------------------------------------------------------------
 * Console init: create wake-up semaphore
 * ---------------------------------------------------------------- */
void rt_hw_console_init(void)
{
    _tx_init();
    _tx_wake = rt_sem_create("con", 0, RT_IPC_FLAG_PRIO);
    RT_ASSERT(_tx_wake != RT_NULL);
}

/* ============================================================
 * UART RX ring buffer + interrupt handler (for FinSH input)
 * ============================================================ */
#ifdef RT_USING_FINSH

#define RX_BUF_SIZE     64
static char _rx_buf[RX_BUF_SIZE];
static struct ringbuf _rx_rb;

static void _rx_init(void)
{
    ringbuf_init(&_rx_rb, _rx_buf, RX_BUF_SIZE);
}

/* UART RX interrupt callback (registered via MSS_UART_set_rx_handler).
 * Called by HAL"s PLIC external interrupt dispatch chain. */
static void _uart_rx_handler(mss_uart_instance_t *this_uart)
{
    rt_interrupt_enter();
    uint8_t byte;
    while (MSS_UART_get_rx(this_uart, &byte, 1) == 1) {
        ringbuf_putc(&_rx_rb, (char)byte);
    }
    rt_interrupt_leave();
}

#endif /* RT_USING_FINSH */

/* ============================================================
 * HAL Tick ISR chain
 * ============================================================ */
void U54_1_sysTick_IRQHandler(void)
{
    rt_interrupt_enter();
    rt_tick_increase();
    rt_interrupt_leave();
}

/* ============================================================
 * rt_hw_board_init() -- board-level init
 * ============================================================ */
void rt_hw_board_init(void)
{
    rt_uint8_t hart_id = (rt_uint8_t)read_csr(mhartid);

    /* Step 1: init UART console */
    #ifdef RPMSG_MASTER
        mss_config_clk_rst(MSS_PERIPH_MMUART1, (uint8_t)hart_id, PERIPHERAL_ON);
    #endif

    MSS_UART_init(&g_mss_uart1_lo, MSS_UART_115200_BAUD,
                  MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);

    PLIC_init();
    /* Step 2: init RT-Thread kernel */
    rt_system_scheduler_init();
    rt_system_timer_init();

    rt_thread_idle_init();
#ifdef RT_USING_TIMER_SOFT
    rt_system_timer_thread_init();
#endif

    /* Step 3: init SysTick */
    if (SysTick_Config() == 0U) {
        MSS_UART_polled_tx_string(&g_mss_uart1_lo,
            "\r\n*** SysTick_Config Success! ***\r\n");
    } else {
        MSS_UART_polled_tx_string(&g_mss_uart1_lo,
            "\r\n*** SysTick_Config Failed! ***\r\n");
        while (1);
    }

    /* Step 4: heap init */
    extern unsigned long __bss_end;
    extern unsigned char _end;
    rt_system_heap_init(
        (void *)&__bss_end,
        (void *)((unsigned long)&_end + 512UL * 1024UL));

    /* Step 5: board component init (reserved) */
    rt_components_board_init();

    /* Step 6: init console ring buffer + semaphore (after kernel/object system) */
    rt_hw_console_init();

#ifdef RT_USING_FINSH
    /* Step 7: init RX ring buffer + UART RX interrupt */
    _rx_init();

    /* Enable UART RX interrupt + PLIC routing */
    MSS_UART_set_rx_handler(&g_mss_uart1_lo, _uart_rx_handler, MSS_UART_FIFO_SINGLE_BYTE);
    MSS_UART_enable_local_irq(&g_mss_uart1_lo);
    // set_csr(mie, MIP_MEIP);
#endif /* RT_USING_FINSH */
}

/* ============================================================
 * rt_hw_console_output() -- RT-Thread console output
 * ============================================================ */
void rt_hw_console_output(const char *str)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    while (*str) {
        if (*str == '\n') {
            ringbuf_putc(&_tx_rb,'\r');
            ringbuf_putc(&_tx_rb,'\n');
        } else {
            ringbuf_putc(&_tx_rb,*str);
        }
        str++;
    }
    rt_hw_interrupt_enable(level);

    if (_tx_wake != RT_NULL) {
        rt_sem_release(_tx_wake);
    }

    // MSS_UART_polled_tx_string(&g_mss_uart1_lo, str);
}


/* ============================================================
 * rt_hw_console_getchar() -- RT-Thread FinSH input
 * ============================================================ */
#ifdef RT_USING_FINSH
char rt_hw_console_getchar(void)
{
    int ch = ringbuf_getc(&_rx_rb);
    if (ch < 0) {
        rt_thread_mdelay(10);
    }
    return ch;
}
#endif

