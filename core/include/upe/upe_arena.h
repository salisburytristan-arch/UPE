#pragma once

#include <stddef.h>
#include <stdint.h>
#include "upe/upe_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeArena {
    uint8_t *data;
    size_t capacity;
    size_t offset;
    UpeAllocator *allocator;
} UpeArena;

void UpeArena_Init(UpeArena *arena, size_t capacity, UpeAllocator *allocator);
void UpeArena_Destroy(UpeArena *arena);
void UpeArena_Reset(UpeArena *arena);
void *UpeArena_Alloc(UpeArena *arena, size_t size, size_t align);

#ifdef __cplusplus
}
#endif
