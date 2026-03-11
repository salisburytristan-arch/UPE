#pragma once

#include <stddef.h>
#include <stdint.h>
#include "upe/upe_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpePool {
    uint8_t *memory;
    void *free_list;
    size_t block_size;
    uint32_t capacity;
    uint32_t in_use;
    UpeAllocator *allocator;
} UpePool;

void UpePool_Init(UpePool *pool, size_t block_size, uint32_t capacity, UpeAllocator *allocator);
void UpePool_Destroy(UpePool *pool);
void *UpePool_Alloc(UpePool *pool);
void UpePool_Free(UpePool *pool, void *ptr);

#ifdef __cplusplus
}
#endif
