#pragma once

#include <stdint.h>
#include "upe/upe_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeHashMap64 {
    uint64_t *keys;
    uint64_t *values;
    uint8_t *states;
    uint32_t capacity;
    uint32_t count;
    UpeAllocator *allocator;
} UpeHashMap64;

void UpeHashMap64_Init(UpeHashMap64 *map, uint32_t capacity, UpeAllocator *allocator);
void UpeHashMap64_Destroy(UpeHashMap64 *map);
void UpeHashMap64_Clear(UpeHashMap64 *map);
int UpeHashMap64_Set(UpeHashMap64 *map, uint64_t key, uint64_t value);
int UpeHashMap64_Get(const UpeHashMap64 *map, uint64_t key, uint64_t *out_value);
int UpeHashMap64_Remove(UpeHashMap64 *map, uint64_t key);

#ifdef __cplusplus
}
#endif
