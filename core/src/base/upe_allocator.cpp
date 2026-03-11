#include "upe/upe_allocator.h"
#include "upe/upe_profiler.h"
#include <stdlib.h>

#ifdef _WIN32
#include <malloc.h>
#endif

// Global profiler memory stats (enabled in UpeMemory_Init)
static UpeProfilerMemoryStats g_profiler_memory_stats = {};
static int g_profiler_enabled = 0;

static void *UpeDefaultAlloc(void *user, size_t size, size_t align) {
    (void)user;
    if (align < sizeof(void *)) {
        align = sizeof(void *);
    }
#ifdef _WIN32
    void *ptr = _aligned_malloc(size, align);
#else
    size_t aligned_size = (size + (align - 1)) & ~(align - 1);
    void *ptr = aligned_alloc(align, aligned_size);
#endif
    
    // Track allocation in profiler (category 0 = general allocation)
    if (ptr && g_profiler_enabled) {
        UpeMemory_TrackAlloc(&g_profiler_memory_stats, 0, (uint64_t)size);
    }
    
    return ptr;
}

static void UpeDefaultFree(void *user, void *ptr) {
    (void)user;
    if (!ptr) {
        return;
    }
    
    // Track free in profiler (note: we don't know the size, so we pass 0)
    // For proper tracking, caller should use sized allocations
    if (g_profiler_enabled) {
        UpeMemory_TrackFree(&g_profiler_memory_stats, 0, 0);
    }
    
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

UpeAllocator UpeAllocator_Default() {
    UpeAllocator allocator;
    allocator.alloc = UpeDefaultAlloc;
    allocator.free = UpeDefaultFree;
    allocator.user = nullptr;
    return allocator;
}

void *UpeAlloc(UpeAllocator *allocator, size_t size, size_t align) {
    UpeAllocator fallback;
    if (!allocator || !allocator->alloc) {
        fallback = UpeAllocator_Default();
        allocator = &fallback;
    }
    return allocator->alloc(allocator->user, size, align);
}

void UpeFree(UpeAllocator *allocator, void *ptr) {
    UpeAllocator fallback;
    if (!allocator || !allocator->free) {
        fallback = UpeAllocator_Default();
        allocator = &fallback;
    }
    allocator->free(allocator->user, ptr);
}

// ========================================
// Profiler Memory Tracking API
// ========================================

void UpeAllocator_EnableMemoryProfiling() {
    UpeMemory_Init(&g_profiler_memory_stats);
    g_profiler_enabled = 1;
}

void UpeAllocator_PrintMemoryReport() {
    UpeMemory_PrintReport(&g_profiler_memory_stats);
}

UpeProfilerMemoryStats *UpeAllocator_GetMemoryStats() {
    return &g_profiler_memory_stats;
}
