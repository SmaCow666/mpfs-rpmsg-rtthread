#include <rtthread.h>
#include <rthw.h>
#include "mpfs_hal/mss_hal.h"
#include "drivers/mss/mss_mmuart/mss_uart.h"
#include "board.h"

static void thread1_entry(void *parameter)
{
    int count = 0;
    while (1) {
        rt_kprintf("[RTT] thread1: count=%d  tick=%lu\n",
                   count++, (unsigned long)rt_tick_get());
        rt_thread_mdelay(1000);
    }
}

static void thread2_entry(void *parameter)
{
    while (1) {
        rt_kprintf("[RTT] thread2: alive  tick=%lu\n",
                   (unsigned long)rt_tick_get());
        rt_thread_mdelay(500);
    }
}

void start_rtt_demo(void)
{
    rt_thread_t tid1, tid2;
    rt_uint8_t hart_id = (rt_uint8_t)read_csr(mhartid);

    /* Step 1: Init MMUART1 console */
    mss_config_clk_rst(MSS_PERIPH_MMUART1, hart_id, PERIPHERAL_ON);
    MSS_UART_init(&g_mss_uart1_lo, MSS_UART_115200_BAUD,
                  MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);
    MSS_UART_polled_tx_string(&g_mss_uart1_lo,
        "\r\n*** RT-Thread Nano MPFS250T Demo ***\r\n");

    PLIC_init();
    rt_hw_board_init();

    /* Step 2: Close MIE (SysTick_Config called __enable_irq) */
    (void)rt_hw_interrupt_disable();

    /* Step 3: Init scheduler + timer */
    rt_system_scheduler_init();
    rt_system_timer_init();

    /* Step 4: Create test threads */
    tid1 = rt_thread_create("t1", thread1_entry, RT_NULL, 2048, 10, 10);
    if (tid1 != RT_NULL) rt_thread_startup(tid1);

    tid2 = rt_thread_create("t2", thread2_entry, RT_NULL, 2048, 15, 10);
    if (tid2 != RT_NULL) rt_thread_startup(tid2);

    /* Step 5: Start scheduler (never returns) */
    rt_kprintf("[RTT] Starting scheduler...\r\n");
    rt_system_scheduler_start();
    while (1);
}