/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * 2024-06-15     RT-Thread    Nano kernel source structure
 *
 * NOTE: This file is a scaffold for the RT-Thread Nano port on
 * PolarFire SoC. The authoritative source comes from:
 *   https://github.com/RT-Thread/rtthread-nano.git
 * via git subtree (pending network access).
 */

#ifndef __RT_DEF_H__
#define __RT_DEF_H__

#include <stdint.h>
#include <stddef.h>

/* ========== Base Types ========== */
typedef signed   char                   rt_int8_t;
typedef signed   short                  rt_int16_t;
typedef signed   int                    rt_int32_t;
typedef unsigned char                   rt_uint8_t;
typedef unsigned short                  rt_uint16_t;
typedef unsigned int                    rt_uint32_t;
#if __riscv_xlen == 64
typedef signed   long                   rt_int64_t;
typedef unsigned long                   rt_uint64_t;
typedef unsigned long                   rt_ubase_t;
typedef signed   long                   rt_base_t;
#else
typedef signed   long long              rt_int64_t;
typedef unsigned long long              rt_uint64_t;
typedef unsigned int                    rt_ubase_t;
typedef signed   int                    rt_base_t;
#endif

typedef rt_ubase_t                      rt_err_t;
typedef rt_ubase_t                      rt_size_t;
typedef rt_base_t                       rt_ssize_t;
typedef rt_ubase_t                      rt_tick_t;
typedef rt_ubase_t                      rt_flag_t;
typedef void                           *rt_handle_t;
typedef rt_ubase_t                      rt_off_t;

/* ========== Boolean ========== */
#define RT_TRUE                         1
#define RT_FALSE                        0

/* ========== Null ========== */
#ifndef NULL
#define NULL                            ((void *)0)
#endif

/* ========== Object Type ========== */
enum rt_object_class_type
{
    RT_Object_Class_Thread = 0,
    RT_Object_Class_Semaphore,
    RT_Object_Class_Mutex,
    RT_Object_Class_Event,
    RT_Object_Class_MailBox,
    RT_Object_Class_MessageQueue,
    RT_Object_Class_MemHeap,
    RT_Object_Class_MemPool,
    RT_Object_Class_Device,
    RT_Object_Class_Timer,
    RT_Object_Class_Unknown,
};

/* ========== Thread ========== */
#define RT_THREAD_INIT                  0x00
#define RT_THREAD_READY                 0x01
#define RT_THREAD_SUSPEND               0x02
#define RT_THREAD_RUNNING               0x03
#define RT_THREAD_BLOCK                 RT_THREAD_SUSPEND
#define RT_THREAD_CLOSE                 0x04
#define RT_THREAD_STAT_MASK             0x07

#define RT_EOK                          0
#define RT_ERROR                        1
#define RT_ETIMEOUT                     2
#define RT_EFULL                        3
#define RT_EEMPTY                       4
#define RT_ENOMEM                       5
#define RT_ENOSYS                       6
#define RT_EBUSY                        7
#define RT_EIO                          8
#define RT_EINTR                        9
#define RT_EINVAL                       10

#define RT_ALIGN_SIZE                   8
#define RT_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))
#define RT_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))
#define RT_NULL                         NULL

#define RT_NAME_MAX                     8
#define RT_THREAD_PRIORITY_MAX          31

/* ========== Object ========== */
struct rt_object
{
    char               name[RT_NAME_MAX];
    rt_uint8_t         type;
    rt_uint8_t         flag;
    rt_list_t          list;
};

struct rt_object_information
{
    enum rt_object_class_type type;
    rt_list_t                 object_list;
    rt_size_t                 object_size;
};

/* ========== Timer ========== */
struct rt_timer
{
    struct rt_object    parent;
    rt_list_t           row[1];
    void (*timeout_func)(void *parameter);
    void               *parameter;
    rt_tick_t           init_tick;
    rt_tick_t           timeout_tick;
};

/* ========== Thread ========== */
struct rt_thread
{
    char                name[RT_NAME_MAX];
    rt_uint8_t          type;
    rt_uint8_t          flags;
    rt_list_t           list;
    rt_list_t           tlist;

    void               *sp;
    void               *entry;
    void               *parameter;
    void               *stack_addr;
    rt_uint32_t         stack_size;

    rt_err_t            error;
    rt_uint8_t          stat;
    rt_uint8_t          current_priority;
    rt_uint8_t          init_priority;
    rt_uint8_t          number;
    rt_uint32_t         number_mask;

    rt_ubase_t          init_stack;

    rt_uint32_t         remaining_tick;
    rt_uint32_t         init_tick;

    void               *cleanup;
    rt_uint32_t         user_data;
};

/* ========== List ========== */
struct rt_list_node
{
    struct rt_list_node *next;
    struct rt_list_node *prev;
};
typedef struct rt_list_node rt_list_t;

void rt_list_init(rt_list_t *l);
void rt_list_insert_after(rt_list_t *l, rt_list_t *n);
void rt_list_insert_before(rt_list_t *l, rt_list_t *n);
void rt_list_remove(rt_list_t *n);
int  rt_list_isempty(const rt_list_t *l);

/* ========== IPC ========== */
struct rt_semaphore
{
    struct rt_object    parent;
    rt_uint16_t         value;
    rt_uint16_t         max_value;
};

struct rt_mutex
{
    struct rt_object    parent;
    rt_uint16_t         value;
    rt_uint8_t          original_priority;
    rt_uint8_t          hold;
    struct rt_thread   *owner;
};

struct rt_event
{
    struct rt_object    parent;
    rt_uint32_t         set;
};

struct rt_mailbox
{
    struct rt_object    parent;
    rt_ubase_t         *msg_pool;
    rt_uint16_t         size;
    rt_uint16_t         entry;
    rt_uint16_t         in_offset;
    rt_uint16_t         out_offset;
    rt_list_t           suspend_sender_thread;
};

struct rt_messagequeue
{
    struct rt_object    parent;
    void               *msg_pool;
    rt_uint16_t         msg_size;
    rt_uint16_t         max_msgs;
    rt_uint16_t         entry;
    void               *msg_queue_head;
    void               *msg_queue_tail;
    void               *msg_queue_free;
    rt_list_t           suspend_sender_thread;
};

/* ========== Device ========== */
struct rt_device
{
    struct rt_object    parent;
    rt_uint16_t         flag;
    rt_uint16_t         open_flag;
    rt_uint8_t          ref_count;
    rt_uint8_t          device_id;

    rt_err_t (*init)(rt_device_t dev);
    rt_err_t (*open)(rt_device_t dev, rt_uint16_t oflag);
    rt_err_t (*close)(rt_device_t dev);
    rt_ssize_t (*read)(rt_device_t dev, rt_off_t pos, void *buf, rt_size_t size);
    rt_ssize_t (*write)(rt_device_t dev, rt_off_t pos, const void *buf, rt_size_t size);
    rt_err_t (*control)(rt_device_t dev, int cmd, void *args);
    void *user_data;
};

typedef struct rt_device *rt_device_t;

#endif /* __RT_DEF_H__ */
