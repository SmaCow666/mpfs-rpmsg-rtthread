/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NOTE: Scaffold file. Authoritative source comes from:
 *   https://github.com/RT-Thread/rtthread-nano.git
 */

#ifndef __RT_THREAD_H__
#define __RT_THREAD_H__

#include <rtdef.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== System Init ========== */
void rt_system_scheduler_init(void);
void rt_system_scheduler_start(void);
void rt_system_timer_init(void);
void rt_system_heap_init(void *begin_addr, void *end_addr);

/* ========== Thread Management ========== */
rt_err_t rt_thread_init(struct rt_thread *thread,
                        const char *name,
                        void (*entry)(void *parameter),
                        void *parameter,
                        void *stack_start,
                        rt_uint32_t stack_size,
                        rt_uint8_t priority,
                        rt_uint32_t tick);
rt_err_t rt_thread_detach(struct rt_thread *thread);
rt_thread_t rt_thread_create(const char *name,
                             void (*entry)(void *parameter),
                             void *parameter,
                             rt_uint32_t stack_size,
                             rt_uint8_t priority,
                             rt_uint32_t tick);
rt_err_t rt_thread_delete(rt_thread_t thread);
rt_thread_t rt_thread_self(void);
rt_err_t rt_thread_yield(void);
rt_err_t rt_thread_sleep(rt_tick_t tick);
rt_err_t rt_thread_mdelay(rt_int32_t ms);
void rt_thread_suspend(struct rt_thread *thread);
void rt_thread_resume(struct rt_thread *thread);

/* ========== Timer Management ========== */
void rt_tick_increase(void);
rt_tick_t rt_tick_get(void);
void rt_tick_set(rt_tick_t tick);

/* ========== Semaphore ========== */
rt_err_t rt_sem_init(rt_sem_t sem, const char *name, rt_uint32_t value, rt_uint8_t flag);
rt_err_t rt_sem_detach(rt_sem_t sem);
rt_sem_t rt_sem_create(const char *name, rt_uint32_t value, rt_uint8_t flag);
rt_err_t rt_sem_delete(rt_sem_t sem);
rt_err_t rt_sem_take(rt_sem_t sem, rt_int32_t time);
rt_err_t rt_sem_trytake(rt_sem_t sem);
rt_err_t rt_sem_release(rt_sem_t sem);

/* ========== Mutex ========== */
rt_err_t rt_mutex_init(rt_mutex_t mutex, const char *name, rt_uint8_t flag);
rt_err_t rt_mutex_detach(rt_mutex_t mutex);
rt_mutex_t rt_mutex_create(const char *name, rt_uint8_t flag);
rt_err_t rt_mutex_delete(rt_mutex_t mutex);
rt_err_t rt_mutex_take(rt_mutex_t mutex, rt_int32_t time);
rt_err_t rt_mutex_release(rt_mutex_t mutex);

/* ========== Event ========== */
rt_err_t rt_event_init(rt_event_t event, const char *name, rt_uint8_t flag);
rt_err_t rt_event_detach(rt_event_t event);
rt_event_t rt_event_create(const char *name, rt_uint8_t flag);
rt_err_t rt_event_delete(rt_event_t event);
rt_err_t rt_event_send(rt_event_t event, rt_uint32_t set);
rt_err_t rt_event_recv(rt_event_t event, rt_uint32_t set,
                       rt_uint8_t option, rt_int32_t timeout, rt_uint32_t *recved);

/* ========== Mailbox ========== */
rt_err_t rt_mb_init(rt_mailbox_t mb, const char *name, void *msgpool,
                    rt_size_t size, rt_uint8_t flag);
rt_err_t rt_mb_detach(rt_mailbox_t mb);
rt_mailbox_t rt_mb_create(const char *name, rt_size_t size, rt_uint8_t flag);
rt_err_t rt_mb_delete(rt_mailbox_t mb);
rt_err_t rt_mb_send(rt_mailbox_t mb, rt_ubase_t value);
rt_err_t rt_mb_send_wait(rt_mailbox_t mb, rt_ubase_t value, rt_int32_t timeout);
rt_err_t rt_mb_recv(rt_mailbox_t mb, rt_ubase_t *value, rt_int32_t timeout);

/* ========== Message Queue ========== */
rt_err_t rt_mq_init(rt_mq_t mq, const char *name, void *msgpool,
                    rt_size_t msg_size, rt_size_t pool_size, rt_uint8_t flag);
rt_err_t rt_mq_detach(rt_mq_t mq);
rt_mq_t rt_mq_create(const char *name, rt_size_t msg_size,
                     rt_size_t max_msgs, rt_uint8_t flag);
rt_err_t rt_mq_delete(rt_mq_t mq);
rt_err_t rt_mq_send(rt_mq_t mq, const void *buffer, rt_size_t size);
rt_err_t rt_mq_recv(rt_mq_t mq, void *buffer, rt_size_t size,
                    rt_int32_t timeout);

/* ========== Memory Management ========== */
void *rt_malloc(rt_size_t nbytes);
void *rt_realloc(void *ptr, rt_size_t nbytes);
void *rt_calloc(rt_size_t count, rt_size_t size);
void rt_free(void *ptr);

/* ========== Hook Functions ========== */
void rt_scheduler_sethook(void (*hook)(struct rt_thread *from, struct rt_thread *to));
void rt_timer_sethook(void (*hook)(struct rt_timer *timer));

/* ========== Console ========== */
void rt_kprintf(const char *fmt, ...);
rt_err_t rt_console_set_device(const char *name);

/* ========== Component Init ========== */
void rt_components_board_init(void);

/* ========== idle hook ========== */
void rt_thread_idle_sethook(void (*hook)(void));
void rt_thread_idle_delhook(void (*hook)(void));

/* ========== Scheduler ========== */
void rt_schedule(void);
void rt_scheduler_sethook(void (*hook)(struct rt_thread *from, struct rt_thread *to));

/* ========== Software Timer ========== */
rt_err_t rt_timer_init(rt_timer_t timer,
                       const char *name,
                       void (*timeout)(void *parameter),
                       void *parameter,
                       rt_tick_t time,
                       rt_uint8_t flag);
rt_err_t rt_timer_detach(rt_timer_t timer);
rt_timer_t rt_timer_create(const char *name,
                           void (*timeout)(void *parameter),
                           void *parameter,
                           rt_tick_t time,
                           rt_uint8_t flag);
rt_err_t rt_timer_delete(rt_timer_t timer);
rt_err_t rt_timer_start(rt_timer_t timer);
rt_err_t rt_timer_stop(rt_timer_t timer);
rt_err_t rt_timer_control(rt_timer_t timer, int cmd, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* __RT_THREAD_H__ */
