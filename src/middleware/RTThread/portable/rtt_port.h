/*
 * Copyright (c) 2024, RT-Thread PolarFire SoC Port
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-06-15                  First port: RISC-V rv64 port header
 *
 * rtt_port.h - Port macro definitions for RT-Thread on RISC-V rv64
 * Mimics FreeRTOS portmacro.h structure.
 */

#ifndef RTT_PORT_H__
#define RTT_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ==============================================================
 * Type and Architecture Definitions
 * ============================================================== */

/* RT-Thread base types (rv64) */
typedef unsigned long          rt_base_t;
typedef long                   rt_base_signed_t;
typedef unsigned long          rt_ubase_t;

/* Stack type (64-bit for rv64) */
typedef unsigned long long     rt_stack_t;
typedef unsigned long long     rt_uint64_t;

/* Byte alignment */
#define RT_ALIGNMENT           8
#define RT_ALIGNMENT_MASK      (RT_ALIGNMENT - 1)

/* Stack growth direction (downward) */
#define RT_STACK_GROWTH        (-1)

/* ==============================================================
 * Critical Section Management
 * ============================================================== */

/* Disable machine-mode interrupts (MIE bit in mstatus, bit 3) */
#define rt_hw_interrupt_disable()                                   \
    ({                                                              \
        unsigned long __mask;                                       \
        __asm volatile("csrr %0, mstatus" : "=r"(__mask));          \
        __asm volatile("csrc mstatus, 8");                          \
        __mask;                                                     \
    })

/* Enable machine-mode interrupts (restore saved mstatus) */
#define rt_hw_interrupt_enable(__mask)                              \
    __asm volatile("csrw mstatus, %0" :: "r"(__mask))

/* ==============================================================
 * Context Switch Macros
 * ============================================================== */

/* Trigger PendSV (or in RISC-V: trigger a software interrupt) */
void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to);
void rt_hw_context_switch_to(rt_ubase_t to);
void rt_hw_context_switch_interrupt(rt_ubase_t from, rt_ubase_t to);

/* ==============================================================
 * Thread Flags
 * ============================================================== */

/* Thread status flags */
#define RT_THREAD_STAT_MASK     0x07
#define RT_THREAD_READY         0x00
#define RT_THREAD_SUSPEND       0x01
#define RT_THREAD_INIT          0x02
#define RT_THREAD_CLOSE         0x03

/* Thread error codes */
#define RT_EOK                  0
#define RT_ERROR                1
#define RT_ETIMEOUT             2
#define RT_EFULL                3
#define RT_EEMPTY               4
#define RT_ENOMEM               5
#define RT_ENOSYS               6

/* ==============================================================
 * TCB Structure (mirror of struct rt_thread in rtdef.h)
 * ============================================================== */

/* Lightweight thread control block for port-level access */
struct rt_thread_port {
    rt_ubase_t       sp;           /* Stack pointer (must be first!) */
    rt_ubase_t       init_stack;   /* Initial stack address */
    void            *entry;        /* Thread entry function */
    void            *parameter;    /* Entry parameter */
    void            *stack_addr;   /* Stack address */
    rt_ubase_t       stack_size;   /* Stack size */
    rt_ubase_t       error;        /* Error code */
    rt_ubase_t       stat;         /* Thread status */
    rt_ubase_t       current_priority;
    rt_ubase_t       init_priority;
};

#ifdef __cplusplus
}
#endif

#endif /* RTT_PORT_H__ */
