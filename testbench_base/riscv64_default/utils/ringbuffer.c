#include "ringbuffer.h"

#ifndef REG64
    #define REG64(addr)                     (*(volatile unsigned long long *)(unsigned long long)(addr))
#endif

extern int dma_init(uint64_t src_addr, uint64_t dst_addr);
extern void delayus(unsigned long dly);

/**
 * @file
 * Implementation of ring buffer functions.
 */

void ring_buffer_init(ring_buffer_t *buffer)
{
    buffer->buffer = (int16_t *)0x60100000ULL;
    buffer->tail_index = 0;
    buffer->head_index = 0;
}

void ring_buffer_queue(ring_buffer_t *buffer, int16_t data)
{
    /* Is buffer full? */
    if (ring_buffer_is_full(buffer))
    {
        /* Is going to overwrite the oldest byte */
        /* Increase tail index */
        buffer->tail_index = ((buffer->tail_index + 1) & RING_BUFFER_MASK);
    }
    /* Place data in buffer */
    buffer->buffer[buffer->head_index] = data;
    buffer->head_index = ((buffer->head_index + 1) & RING_BUFFER_MASK);
}

extern void uart_putc(uint8_t ch);

void ring_buffer_queue_arr(ring_buffer_t *buffer, const int16_t *data, ring_buffer_size_t size)
{
    (void)data;
    // uint64_t reg_val;
    // /* Add bytes; one by one */
    // /* Is buffer full? */
    // if (ring_buffer_is_full(buffer)) {
    //   /* Is going to overwrite the oldest byte */
    //   /* Increase tail index */
    //   buffer->tail_index = ((buffer->tail_index + size) & RING_BUFFER_MASK);
    // }
    // uart_putc( (uint64_t)(0x60100000UL + buffer->head_index * 2) >> 24);
    // uart_putc( (uint64_t)(0x60100000UL + buffer->head_index * 2) >> 16);
    // uart_putc( (uint64_t)(0x60100000UL + buffer->head_index * 2) >> 8);
    // uart_putc( (uint64_t)(0x60100000UL + buffer->head_index * 2));
    // dma_init((uint64_t)(uint64_t *)data, (uint64_t)(0x60100000UL + buffer->head_index * 2));
    // do {
    //   delayus(10);
    //   reg_val = REG64(0x62500188UL);
    // } while (reg_val == 0);
    // do {
    //   /* code */
    //   REG64(0x62500198UL) = 0x3UL;
    //   reg_val = REG64(0x62500188UL);
    // } while (reg_val);
    buffer->head_index = ((buffer->head_index + size) & RING_BUFFER_MASK);
}
extern void show_char(int8_t ch);


#include <stdio.h>
int16_t ring_buffer_dequeue(ring_buffer_t *buffer, int16_t *data)
{
    (void)data;
    if (ring_buffer_is_empty(buffer))
    {
        /* No items */
        return 0;
    }
    *data = buffer->buffer[buffer->tail_index];
    // printf("%hd,", *data);
    buffer->tail_index = ((buffer->tail_index + 1) & RING_BUFFER_MASK);
    return 1;
}

ring_buffer_size_t ring_buffer_dequeue_arr(ring_buffer_t *buffer, int16_t *data, ring_buffer_size_t len)
{
    if (ring_buffer_is_empty(buffer))
    {
        return 0;  // No items.
    }

    ring_buffer_size_t initial_tail = buffer->tail_index;
    ring_buffer_size_t space_left = len;

    // Determine the number of items in the buffer.
    ring_buffer_size_t buffer_count = (buffer->head_index - initial_tail) & RING_BUFFER_MASK;

    // Determine how many items to dequeue.
    ring_buffer_size_t dequeue_count = (buffer_count < space_left) ? buffer_count : space_left;

    for (ring_buffer_size_t i = 0; i < dequeue_count; ++i)
    {
        *data++ = buffer->buffer[(initial_tail + i) & RING_BUFFER_MASK];
    }

    // Update the tail index after processing.
    buffer->tail_index = (initial_tail + dequeue_count) & RING_BUFFER_MASK;

    return dequeue_count;
}


int16_t ring_buffer_peek(ring_buffer_t *buffer, int16_t *data, ring_buffer_size_t index)
{
    if (index >= ring_buffer_num_items(buffer))
    {
        /* No items at index */
        return 0;
    }
    /* Add index to pointer */
    ring_buffer_size_t data_index = ((buffer->tail_index + index) & RING_BUFFER_MASK);
    *data = buffer->buffer[data_index];
    return 1;
}

int16_t ring_buffer_peek_array(ring_buffer_t *buffer, int16_t *data, ring_buffer_size_t len)
{
    if (ring_buffer_is_empty(buffer))
    {
        /* No items */
        return 0;
    }
    int16_t *data_ptr = data;
    ring_buffer_size_t cnt = 0;
    while ((cnt < len) && ring_buffer_peek(buffer, data_ptr, cnt))
    {
        cnt++;
        data_ptr++;
    }
    return 1;
}

void ring_buffer_clearall(ring_buffer_t *buffer)
{
    buffer->head_index = 0;
    buffer->tail_index = 0;
}

extern inline uint8_t ring_buffer_is_empty(ring_buffer_t *buffer);
extern inline uint8_t ring_buffer_is_full(ring_buffer_t *buffer);
extern inline ring_buffer_size_t ring_buffer_num_items(ring_buffer_t *buffer);
