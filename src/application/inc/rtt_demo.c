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
        if(count > 4) {
            rt_kprintf("[RTT] thread1: exiting after 5 counts\n");
            break;
        }
    }
    /* Exit thread1 */
    rt_kprintf("[RTT] thread1: exiting\n");
    rt_thread_delete(rt_thread_self());
}

static void thread2_entry(void *parameter)
{
    int count = 0;
    while (1) {
        rt_kprintf("[RTT] thread2: alive  tick=%lu\n",
                   (unsigned long)rt_tick_get());
        rt_thread_mdelay(500);
        if(count++ > 4) {
            rt_kprintf("[RTT] thread2: exiting after 5 counts\n");
            break;
        }
    }
    /* Exit thread2 */
    rt_kprintf("[RTT] thread2: exiting\n");
    rt_thread_delete(rt_thread_self());
}

static void thread_test_entry(void)
{
    rt_thread_t tid1, tid2;

    /* Create test threads */
    tid1 = rt_thread_create("t1", thread1_entry, RT_NULL, 4096, 10, 10);
    if (tid1 != RT_NULL) rt_thread_startup(tid1);

    tid2 = rt_thread_create("t2", thread2_entry, RT_NULL, 4096, 15, 10);
    if (tid2 != RT_NULL) rt_thread_startup(tid2);
}

void start_rtt_demo(void)
{
    rt_thread_t tid1, tid2, tprint;

    /* Step 1: board init (starts tick) */
    rt_hw_board_init();

    /* Step 2: create background print thread (lowest priority) */
    tprint = rt_thread_create("con",
                               console_print_entry, RT_NULL,
                               CONSOLE_PRINT_STACK,
                               CONSOLE_PRINT_PRIO, 10);
    if (tprint != RT_NULL) rt_thread_startup(tprint);
    // 请注意：涉及到打印的线程函数必须在该步之后！！

    /* Step 3: create test threads */
    // thread_test_entry(); // PASS

    /* Step 4: init all registered components (FinSH etc.) */
    rt_components_init();
    // finsh_system_init();

    /* Step 5: start scheduler */
    rt_system_scheduler_start();
    // rt_kprintf("[RTT] Starting scheduler...\r\n");
    rt_show_version();
}
