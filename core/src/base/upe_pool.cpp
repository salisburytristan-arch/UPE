#include "upe/upe_pool.h"
#include <string.h>

static size_t UpeAlignUp(size_t value, size_t align) {
    size_t mask = align - 1;
    return (value + mask) & ~mask;
}

void UpePool_Init(UpePool *pool, size_t block_size, uint32_t capacity, UpeAllocator *allocator) {
    if (!pool || block_size == 0 || capacity == 0) {
        return;
    }
    size_t aligned = UpeAlignUp(block_size, sizeof(void *));
    size_t total_size = aligned * capacity;
    pool->allocator = allocator;
    pool->memory = (uint8_t *)UpeAlloc(allocator, total_size, 16);
    pool->block_size = aligned;
    pool->capacity = pool->memory ? capacity : 0;
    pool->in_use = 0;
    pool->free_list = nullptr;

    if (!pool->memory) {
        return;
    }

    for (uint32_t i = 0; i < capacity; ++i) {
        uint8_t *block = pool->memory + (size_t)i * aligned;
        void *next = pool->free_list;
        memcpy(block, &next, sizeof(void *));
        pool->free_list = block;
    }
}

void UpePool_Destroy(UpePool *pool) {
    if (!pool) {
        return;
    }
    if (pool->memory) {
        UpeFree(pool->allocator, pool->memory);
    }
    pool->memory = nullptr;
    pool->free_list = nullptr;
    pool->block_size = 0;
    pool->capacity = 0;
    pool->in_use = 0;
}

void *UpePool_Alloc(UpePool *pool) {
    if (!pool || !pool->free_list) {
        return nullptr;
    }
    void *block = pool->free_list;
    void *next = nullptr;
    memcpy(&next, block, sizeof(void *));
    pool->free_list = next;
    pool->in_use++;
    return block;
}

void UpePool_Free(UpePool *pool, void *ptr) {
    if (!pool || !ptr) {
        return;
    }
    void *next = pool->free_list;
    memcpy(ptr, &next, sizeof(void *));
    pool->free_list = ptr;
    if (pool->in_use > 0) {
        pool->in_use--;
    }
}
