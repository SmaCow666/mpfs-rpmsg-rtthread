/*
 * Copyright (c) 2024, RT-Thread PolarFire SoC Port
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generic single-producer/single-consumer ring buffer.
 * All write operations are ISR-safe. Read operations are thread-safe
 * when only one thread/consumer reads (standard SPSC contract).
 */

#ifndef RINGBUF_H_
#define RINGBUF_H_

#include <rtthread.h>

struct ringbuf {
    char *buf;
    rt_uint16_t size;   /* must be power of 2 */
    volatile rt_uint16_t rd;
    volatile rt_uint16_t wr;
};

/* Initialise a ring buffer instance with a caller-provided buffer.
 * @param rb   pointer to ringbuf control block
 * @param buf  pointer to caller-allocated buffer (size must be power of 2)
 * @param size buffer size (must be power of 2)
 */
void ringbuf_init(struct ringbuf *rb, char *buf, rt_uint16_t size);

/* Write one byte (ISR-safe). Drops byte when full.
 * @return 1 on success, 0 on full */
rt_uint16_t ringbuf_putc(struct ringbuf *rb, char c);

/* Read one byte (thread-safe for single consumer).
 * @return byte value, or -1 if empty */
int ringbuf_getc(struct ringbuf *rb);

/* Query number of bytes available for reading.
 * @return number of bytes in buffer */
rt_uint16_t ringbuf_available(struct ringbuf *rb);

/* Query number of free slots for writing.
 * @return number of free bytes */
rt_uint16_t ringbuf_free(struct ringbuf *rb);

#endif /* RINGBUF_H_ */
