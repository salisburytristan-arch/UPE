#include "upe/upe_ringbuffer.h"
#include <string.h>

void UpeRingBuffer_Init(UpeRingBuffer *buffer, uint32_t capacity, uint32_t stride, UpeAllocator *allocator) {
    if (!buffer || capacity == 0 || stride == 0) {
        return;
    }
    buffer->allocator = allocator;
    buffer->data = UpeAlloc(allocator, (size_t)capacity * stride, 16);
    buffer->capacity = buffer->data ? capacity : 0;
    buffer->stride = stride;
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
}

void UpeRingBuffer_Destroy(UpeRingBuffer *buffer) {
    if (!buffer) {
        return;
    }
    if (buffer->data) {
        UpeFree(buffer->allocator, buffer->data);
    }
    buffer->data = nullptr;
    buffer->capacity = 0;
    buffer->stride = 0;
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
}

int UpeRingBuffer_Push(UpeRingBuffer *buffer, const void *element) {
    if (!buffer || !element || buffer->count == buffer->capacity || buffer->capacity == 0) {
        return 0;
    }
    char *dst = (char *)buffer->data + (size_t)buffer->tail * buffer->stride;
    memcpy(dst, element, buffer->stride);
    buffer->tail = (buffer->tail + 1) % buffer->capacity;
    buffer->count++;
    return 1;
}

int UpeRingBuffer_Pop(UpeRingBuffer *buffer, void *out_element) {
    if (!buffer || buffer->count == 0 || buffer->capacity == 0) {
        return 0;
    }
    char *src = (char *)buffer->data + (size_t)buffer->head * buffer->stride;
    if (out_element) {
        memcpy(out_element, src, buffer->stride);
    }
    buffer->head = (buffer->head + 1) % buffer->capacity;
    buffer->count--;
    return 1;
}
