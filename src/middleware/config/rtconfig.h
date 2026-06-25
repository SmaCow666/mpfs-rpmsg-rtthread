/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * RT-Thread Configuration for PolarFire SoC (Icicle Kit)
 * Single-core, RISC-V 64-bit, Machine-mode
 */

#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* RT-Thread Version */
#define RT_THREAD_VERSION       "v4.1.0"

/* Architecture */
#define ARCH_CPU_64BIT

/* Clock & Tick */
#define RT_TICK_PER_SECOND      1000
#define RT_CPUS_NR              1

/* Kernel Features */
#define RT_USING_HOOK
#define RT_USING_IDLE_HOOK

/* Memory Management */
#define RT_USING_HEAP
#define RT_HEAP_SIZE            (512 * 1024)
#define RT_USING_MEMHEAP
#define RT_USING_MEMPOOL
#define RT_USING_SLAB
#define RT_USING_SLAB_AS_HEAP

/* Thread */
#define RT_THREAD_PRIORITY_MAX  32
#define RT_NAME_MAX             8
#define RT_ALIGN_SIZE           8

/* IPC */
#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MAILBOX
#define RT_USING_MESSAGEQUEUE

/* Timer */
#define RT_USING_TIMER_SOFT
#define RT_TIMER_THREAD_PRIO    4
#define RT_TIMER_THREAD_STACK_SIZE  1024

/* Console */
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE      256
#define RT_CONSOLE_DEVICE_NAME  "mmuart1"

/* Idle Thread */
#define IDLE_THREAD_STACK_SIZE  1024

/* Main Thread (for rt_application_init) */
#define RT_USING_HEAP
#define RT_MAIN_THREAD_STACK_SIZE   4096
#define RT_MAIN_THREAD_PRIORITY     10

/* Kernel Service */
#define RT_USING_COMPONENTS_INIT
#define RT_KSERVICE_USING_STDLIB

/* FinSH */
// #include "finsh_config.h"
#define RT_USING_FINSH
#define FINSH_USING_MSH
#define FINSH_USING_MSH_ONLY
#define FINSH_USING_SYMTAB
#define FINSH_USING_DESCRIPTION
#define FINSH_THREAD_PRIORITY   21
#define FINSH_THREAD_STACK_SIZE 2048

// #define RT_USING_USER_MAIN

#endif /* RT_CONFIG_H__ */
