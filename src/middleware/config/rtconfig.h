/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-06-15     rtthread     The first version of RT-Thread Nano config
 */

#ifndef RTCONFIG_H
#define RTCONFIG_H

/* ==============================================================
 * RT-Thread Nano Configuration for PolarFire SoC Icicle Kit
 *
 * This configuration mirrors FreeRTOSConfig.h in the same
 * middleware/config/ directory to maintain consistent OS parameters
 * for AMP comparisons.
 * ============================================================== */

/* ======================== Kernel ============================ */

/* The maximum number of priorities. max: 256 */
#define RT_THREAD_PRIORITY_MAX  31

/* Tick frequency (Hz). OS tick period = 1000 / RT_TICK_PER_SECOND ms */
#define RT_TICK_PER_SECOND      1000

/* Alignment size for memory pool and message queue */
#define RT_ALIGN_SIZE           8

/* The name length of the kernel object */
#define RT_NAME_MAX             8

/* The byte order */
#define RT_BIG_ENDIAN           0
#define RT_LITTLE_ENDIAN        1
#define RT_BYTE_ORDER           RT_LITTLE_ENDIAN

/* The CPU stack alignment size */
#define RT_CPUS_NR              1

/* ======================== Memory Management ================ */

/* Dynamic heap management */
#define RT_USING_HEAP

/* The size of the heap. 512 KB as default */
#define RT_HW_HEAP_BEGIN        (void*)&__heap_start
#define RT_HW_HEAP_END          (void*)&__heap_end

/* ======================== Hook Support ===================== */

/* Using hook */
#define RT_USING_HOOK

/* Idle hook */
/* #define RT_USING_IDLE_HOOK */

/* ======================== Software Timer ================== */

/* Using software timer */
#define RT_USING_TIMER

/* The priority of timer thread */
#define RT_TIMER_THREAD_PRIO        25

/* The stack size of timer thread */
#define RT_TIMER_THREAD_STACK_SIZE  1024

/* ======================== IPC Mechanisms ================== */

/* Using semaphore */
#define RT_USING_SEMAPHORE

/* Using mutex */
#define RT_USING_MUTEX

/* Using event */
#define RT_USING_EVENT

/* Using mailbox */
#define RT_USING_MAILBOX

/* Using message queue */
#define RT_USING_MESSAGEQUEUE

/* ======================== Debug Options =================== */

/* Enable debug */
/* #define RT_DEBUG */

/* Enable init function call */
#define RT_USING_COMPONENTS_INIT

/* Using user main */
/* #define RT_USING_USER_MAIN */

/* ======================== Device System =================== */

/* Using device system */
/* #define RT_USING_DEVICE */

/* ======================== Console ========================= */

/* Using console */
#define RT_USING_CONSOLE

/* The buffer size of console */
#define RT_CONSOLEBUF_SIZE      128

/* ======================== libc Interface ================== */

/* Using libc */
/* #define RT_USING_LIBC */

/* ======================== Power Management ================ */

/* Using power management */
/* #define RT_USING_PM */

/* ======================== Architecture ==================== */

/* RISC-V architecture */
#define ARCH_RISCV
#define ARCH_RISCV64

/* ======================== Compiler ======================== */
#define RT_USING_NEWLIBC

#endif /* RTCONFIG_H */
