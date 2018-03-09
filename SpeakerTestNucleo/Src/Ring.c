#include "stm32f1xx_hal.h"
#include "Ring.h"

#define RING_QSIZE 16384

static uint8_t buffer[RING_QSIZE];
static uint16_t head;
static uint16_t tail;
static uint16_t size;

extern UART_HandleTypeDef huart2;

static uint16_t inc(uint16_t x)
{
    return (x + 1) % RING_QSIZE;
}

uint32_t Ring_GetNumBytesInBuffer(void)
{
    return size;
}

void Ring_AddByte(uint8_t byte)
{
    if (inc(head) != tail)
    {
        ++size;
        if (size > (RING_QSIZE - 256))
        {
            HAL_UART_Transmit_IT(&huart2, "W", 1);
        }
        buffer[head] = byte;
        head = inc(head);
    }
}

uint8_t Ring_GetByte(void)
{
    if (head != tail)
    {
        uint8_t byte = buffer[tail];
        --size;
        tail = inc(tail);
        return byte;
    }
}