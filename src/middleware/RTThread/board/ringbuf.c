/*
 * Copyright (c) 2024, RT-Thread PolarFire SoC Port
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ringbuf.h"

/* Ring buffer uses (size & (size - 1)) == 0 check for power-of-2 */
void ringbuf_init(struct ringbuf *rb, char *buf, rt_uint16_t size)
{
    rb->buf  = buf;
    rb->size = size;
    rb->rd   = 0;
    rb->wr   = 0;
}

rt_uint16_t ringbuf_putc(struct ringbuf *rb, char c)
{
    rt_uint16_t next = (rb->wr + 1) & (rb->size - 1);
    if (next == rb->rd) {
        return 0; /* full */
    }
    rb->buf[rb->wr] = c;
    rb->wr = next;
    return 1;
}

int ringbuf_getc(struct ringbuf *rb)
{
    if (rb->rd == rb->wr) {
        return -1; /* empty */
    }
    char c = rb->buf[rb->rd];
    rb->rd = (rb->rd + 1) & (rb->size - 1);
    return c;
}

rt_uint16_t ringbuf_available(struct ringbuf *rb)
{
    return (rb->wr - rb->rd) & (rb->size - 1);
}

rt_uint16_t ringbuf_free(struct ringbuf *rb)
{
    return (rb->size - 1) - ringbuf_available(rb);
}
