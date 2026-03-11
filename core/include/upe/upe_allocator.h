#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*UpeAllocFn)(void *user, size_t size, size_t align);
typedef void (*UpeFreeFn)(void *user, void *ptr);

typedef struct UpeAllocator {
    UpeAllocFn alloc;
    UpeFreeFn free;
    void *user;
} UpeAllocator;

// Forward declaration for profiler integration
struct UpeProfilerMemoryStats;

UpeAllocator UpeAllocator_Default();
void *UpeAlloc(UpeAllocator *allocator, size_t size, size_t align);
void UpeFree(UpeAllocator *allocator, void *ptr);

// Memory profiling API
void UpeAllocator_EnableMemoryProfiling();
void UpeAllocator_PrintMemoryReport();
struct UpeProfilerMemoryStats *UpeAllocator_GetMemoryStats();

#ifdef __cplusplus
}
#endif
