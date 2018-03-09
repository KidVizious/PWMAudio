#ifndef RING_H_INCLUDED
#define RING_H_INCLUDED

#include <stdint.h>

uint32_t Ring_GetNumBytesInBuffer(void);
void Ring_AddByte(uint8_t byte);
uint8_t Ring_GetByte(void);

#endif