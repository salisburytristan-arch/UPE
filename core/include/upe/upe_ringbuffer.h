#pragma once

#include <stdint.h>
#include "upe/upe_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeRingBuffer {
    void *data;
    uint32_t capacity;
    uint32_t stride;
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    UpeAllocator *allocator;
} UpeRingBuffer;

void UpeRingBuffer_Init(UpeRingBuffer *buffer, uint32_t capacity, uint32_t stride, UpeAllocator *allocator);
void UpeRingBuffer_Destroy(UpeRingBuffer *buffer);
int UpeRingBuffer_Push(UpeRingBuffer *buffer, const void *element);
int UpeRingBuffer_Pop(UpeRingBuffer *buffer, void *out_element);

#ifdef __cplusplus
}
#endif
