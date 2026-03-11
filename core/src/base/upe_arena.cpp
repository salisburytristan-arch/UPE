#include "upe/upe_arena.h"

static size_t UpeAlignUp(size_t value, size_t align) {
    size_t mask = align - 1;
    return (value + mask) & ~mask;
}

void UpeArena_Init(UpeArena *arena, size_t capacity, UpeAllocator *allocator) {
    if (!arena || capacity == 0) {
        return;
    }
    arena->allocator = allocator;
    arena->data = (uint8_t *)UpeAlloc(allocator, capacity, 16);
    arena->capacity = arena->data ? capacity : 0;
    arena->offset = 0;
}

void UpeArena_Destroy(UpeArena *arena) {
    if (!arena) {
        return;
    }
    if (arena->data) {
        UpeFree(arena->allocator, arena->data);
    }
    arena->data = nullptr;
    arena->capacity = 0;
    arena->offset = 0;
}

void UpeArena_Reset(UpeArena *arena) {
    if (!arena) {
        return;
    }
    arena->offset = 0;
}

void *UpeArena_Alloc(UpeArena *arena, size_t size, size_t align) {
    if (!arena || !arena->data || size == 0) {
        return nullptr;
    }
    if (align < sizeof(void *)) {
        align = sizeof(void *);
    }
    size_t aligned = UpeAlignUp(arena->offset, align);
    if (aligned + size > arena->capacity) {
        return nullptr;
    }
    void *ptr = arena->data + aligned;
    arena->offset = aligned + size;
    return ptr;
}
