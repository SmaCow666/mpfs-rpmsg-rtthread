/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NOTE: Scaffold file. Authoritative source comes from:
 *   https://github.com/RT-Thread/rtthread-nano.git
 */

#ifndef __RT_HW_H__
#define __RT_HW_H__

#include <rtdef.h>
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Interrupt Control ========== */
rt_base_t rt_hw_interrupt_disable(void);
void      rt_hw_interrupt_enable(rt_base_t level);

/* ========== Context Switch ========== */
void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to);
void rt_hw_context_switch_to(rt_ubase_t to);
void rt_hw_context_switch_interrupt(rt_ubase_t from, rt_ubase_t to);

/* ========== Stack Initialization ========== */
rt_uint8_t *rt_hw_stack_init(void *tentry,
                             void *parameter,
                             rt_uint8_t *stack_addr,
                             void *texit);

/* ========== ISR Entry/Exit ========== */
rt_uint32_t rt_hw_interrupt_thread_save(void);
void rt_hw_interrupt_thread_restore(rt_uint32_t level);

/* ========== Bit Operations ========== */
int rt_hw_bit_find_first_zero(rt_ubase_t value);
int rt_hw_bit_find_first_set(rt_ubase_t value);
void rt_hw_bit_set(rt_ubase_t *addr, int bit);
void rt_hw_bit_clear(rt_ubase_t *addr, int bit);
int rt_hw_bit_test(rt_ubase_t *addr, int bit);

/* ========== Cache Operations ========== */
void rt_hw_cpu_icache_enable(void);
void rt_hw_cpu_icache_disable(void);
void rt_hw_cpu_dcache_enable(void);
void rt_hw_cpu_dcache_disable(void);
void rt_hw_cpu_icache_ops(int ops, void *addr, int size);
void rt_hw_cpu_dcache_ops(int ops, void *addr, int size);

/* ========== DSB/ISB Barriers ========== */
void rt_hw_dsb(void);
void rt_hw_isb(void);

#ifdef __cplusplus
}
#endif

#endif /* __RT_HW_H__ */
