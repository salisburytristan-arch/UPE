#include "upe/upe_memory_policy.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <chrono>
#include <thread>
#include <functional>

extern "C" void UpeMemoryDiag_RecordFreedPointer(void *ptr);

/*==============================================================================
  GLOBAL MEMORY STATE
==============================================================================*/

typedef struct {
    UpeMemoryBudget budget;
    UpeMemoryStats stats;
    UpeFrameArena frame_arena;
    
    /* Allocation tracking (hash table for metadata) */
    void **tracked_allocs;            /* Pointers we've allocated */
    UpeAllocationInfo *alloc_infos;   /* Metadata for each */
    uint32_t alloc_capacity;
    uint32_t alloc_count;
    
    /* Freed pointers for use-after-free detection */
    void **freed_pointers;
    uint32_t freed_count;
    uint32_t freed_capacity;
    
    /* Per-subsystem budgets */
    uint64_t budget_used[16];
    
    uint32_t frame_number;
} UpeGlobalMemory;

static UpeGlobalMemory g_memory = {};
static int g_initialized = 0;

static uint32_t _upe_current_thread_id() {
    return (uint32_t)std::hash<std::thread::id>{}(std::this_thread::get_id());
}

static uint64_t _upe_now_us() {
    const auto now = std::chrono::time_point_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now());
    return (uint64_t)now.time_since_epoch().count();
}

/*==============================================================================
  FRAME ARENA IMPLEMENTATION
==============================================================================*/

void* UpeFrameArena_Alloc(UpeFrameArena *arena, size_t size, size_t alignment) {
    assert(arena && arena->buffer);
    
    /* Align offset to requested alignment */
    size_t aligned_offset = (arena->offset + alignment - 1) & ~(alignment - 1);
    
    if (aligned_offset + size > arena->capacity) {
        fprintf(stderr, "UpeFrameArena: Out of memory! Requested %zu bytes (aligned to %zu), "
                        "available %zu bytes\n", size, alignment, arena->capacity - aligned_offset);
        return NULL;
    }
    
    void *ptr = &arena->buffer[aligned_offset];
    arena->offset = aligned_offset + size;
    
    if (arena->offset > arena->peak_frame) {
        arena->peak_frame = arena->offset;
    }
    
    UpeMemory_PoisonNew(ptr, size);
    return ptr;
}

void UpeFrameArena_GetStats(UpeFrameArena *arena, size_t *out_used, size_t *out_peak) {
    if (out_used) *out_used = arena->offset;
    if (out_peak) *out_peak = arena->peak_frame;
}

/*==============================================================================
  POOL ALLOCATOR IMPLEMENTATION
==============================================================================*/

UpePoolAllocator* UpePoolAllocator_Create(size_t object_size, uint32_t capacity,
                                          UpeAllocatorType allocator, UpeMemoryTag tag) {
    UpePoolAllocator *pool = (UpePoolAllocator*)malloc(sizeof(UpePoolAllocator));
    if (!pool) return NULL;
    
    pool->object_size = object_size;
    pool->capacity = capacity;
    pool->count = 0;
    pool->first_free = 0xFFFFFFFF;
    
    /* Allocate dense object array */
    pool->pool = malloc(object_size * capacity);
    if (!pool->pool) {
        free(pool);
        return NULL;
    }
    
    /* Allocate generation array for handle validation */
    pool->generations = (uint32_t*)malloc(sizeof(uint32_t) * capacity);
    if (!pool->generations) {
        free(pool->pool);
        free(pool);
        return NULL;
    }
    
    memset(pool->pool, 0, object_size * capacity);
    memset(pool->generations, 0, sizeof(uint32_t) * capacity);
    
    return pool;
}

void UpePoolAllocator_Destroy(UpePoolAllocator *pool) {
    if (!pool) return;
    free(pool->pool);
    free(pool->generations);
    free(pool);
}

UpePoolHandle UpePoolAllocator_Alloc(UpePoolAllocator *pool) {
    assert(pool);
    
    UpePoolHandle handle = {0xFFFFFFFF, 0};
    
    if (pool->first_free != 0xFFFFFFFF) {
        /* Use free list */
        handle.index = pool->first_free;
        uint32_t *next_ptr = (uint32_t*)((uint8_t*)pool->pool + handle.index * pool->object_size);
        pool->first_free = *next_ptr;
    } else if (pool->count < pool->capacity) {
        /* Allocate from dense end */
        handle.index = pool->count;
        pool->count++;
    } else {
        fprintf(stderr, "Pool allocator exhausted! Capacity %u reached.\n", pool->capacity);
        return handle;
    }
    
    handle.generation = pool->generations[handle.index];
    memset((uint8_t*)pool->pool + handle.index * pool->object_size, 0, pool->object_size);
    
    return handle;
}

void UpePoolAllocator_Free(UpePoolAllocator *pool, UpePoolHandle handle) {
    assert(pool);
    assert(handle.index < pool->capacity);
    
    if (pool->generations[handle.index] != handle.generation) {
        fprintf(stderr, "Double-free or invalid handle detected!\n");
        return;
    }
    
    /* Increment generation for next use of this slot */
    pool->generations[handle.index]++;
    if (pool->generations[handle.index] == 0) {
        pool->generations[handle.index] = 1;  /* Avoid 0 generation */
    }
    
    /* Add to free list */
    uint32_t *next_ptr = (uint32_t*)((uint8_t*)pool->pool + handle.index * pool->object_size);
    *next_ptr = pool->first_free;
    pool->first_free = handle.index;
}

void* UpePoolAllocator_GetPtr(UpePoolAllocator *pool, UpePoolHandle handle) {
    assert(pool);
    if (handle.index >= pool->capacity) return NULL;
    if (pool->generations[handle.index] != handle.generation) return NULL;
    
    return (uint8_t*)pool->pool + handle.index * pool->object_size;
}

int UpePoolAllocator_IsValid(UpePoolAllocator *pool, UpePoolHandle handle) {
    assert(pool);
    if (handle.index >= pool->capacity) return 0;
    return pool->generations[handle.index] == handle.generation ? 1 : 0;
}

/*==============================================================================
  GLOBAL ALLOCATION TRACKING
==============================================================================*/

static int _find_alloc_slot(void *ptr, uint32_t *out_index) {
    for (uint32_t i = 0; i < g_memory.alloc_count; i++) {
        if (g_memory.tracked_allocs[i] == ptr) {
            if (out_index) *out_index = i;
            return 1;
        }
    }
    return 0;
}

static void _track_allocation(void *ptr, size_t size, size_t alignment,
                              UpeAllocatorType allocator, UpeMemoryTag tag,
                              uint32_t callsite_hash) {
    if (!g_initialized) return;
    
    /* Grow tracking arrays if needed */
    if (g_memory.alloc_count >= g_memory.alloc_capacity) {
        uint32_t new_capacity = g_memory.alloc_capacity ? g_memory.alloc_capacity * 2 : 256;
        
        void **new_allocs = (void**)realloc(g_memory.tracked_allocs, new_capacity * sizeof(void*));
        UpeAllocationInfo *new_infos = (UpeAllocationInfo*)realloc(g_memory.alloc_infos, 
                                                                     new_capacity * sizeof(UpeAllocationInfo));
        
        if (!new_allocs || !new_infos) {
            fprintf(stderr, "Failed to grow allocation tracking array\n");
            return;
        }
        
        g_memory.tracked_allocs = new_allocs;
        g_memory.alloc_infos = new_infos;
        g_memory.alloc_capacity = new_capacity;
    }
    
    uint32_t idx = g_memory.alloc_count++;
    g_memory.tracked_allocs[idx] = ptr;
    
    UpeAllocationInfo *info = &g_memory.alloc_infos[idx];
    info->size = size;
    info->alignment = alignment;
    info->allocator = allocator;
    info->tag = tag;
    info->callsite_hash = callsite_hash;
    info->thread_id = _upe_current_thread_id();
    info->frame_number = g_memory.frame_number;
    info->timestamp_us = _upe_now_us();
    
    /* Update stats */
    g_memory.stats.total_allocated += size;
    if (g_memory.stats.total_allocated > g_memory.stats.peak_allocated) {
        g_memory.stats.peak_allocated = g_memory.stats.total_allocated;
    }
    g_memory.stats.allocation_count++;
    g_memory.stats.allocations_this_frame++;
    
    /* Update per-tag stats */
    if (tag < 16) {
        g_memory.stats.bytes_by_tag[tag] += size;
    }
    
    /* Update per-allocator stats */
    if (allocator < 5) {
        g_memory.stats.bytes_by_allocator[allocator] += size;
    }
}

static void _untrack_allocation(void *ptr) {
    if (!g_initialized) return;
    
    uint32_t idx = 0;
    if (!_find_alloc_slot(ptr, &idx)) {
        /* Not tracked - likely from external allocator */
        return;
    }
    
    UpeAllocationInfo *info = &g_memory.alloc_infos[idx];
    
    /* Update stats */
    g_memory.stats.total_allocated -= info->size;
    g_memory.stats.allocation_count--;
    g_memory.stats.deallocations_this_frame++;
    
    /* Update per-tag stats */
    if (info->tag < 16) {
        g_memory.stats.bytes_by_tag[info->tag] -= info->size;
    }
    
    /* Update per-allocator stats */
    if (info->allocator < 5) {
        g_memory.stats.bytes_by_allocator[info->allocator] -= info->size;
    }
    
    /* Remove from tracking */
    if (idx < g_memory.alloc_count - 1) {
        g_memory.tracked_allocs[idx] = g_memory.tracked_allocs[g_memory.alloc_count - 1];
        g_memory.alloc_infos[idx] = g_memory.alloc_infos[g_memory.alloc_count - 1];
    }
    g_memory.alloc_count--;
    
    /* Track freed pointer for use-after-free detection */
    #ifdef UPE_DEBUG_MEMORY
    if (g_memory.freed_count >= g_memory.freed_capacity) {
        g_memory.freed_capacity = g_memory.freed_capacity ? g_memory.freed_capacity * 2 : 256;
        void **new_freed = (void**)realloc(g_memory.freed_pointers, 
                                          g_memory.freed_capacity * sizeof(void*));
        if (new_freed) {
            g_memory.freed_pointers = new_freed;
        }
    }
    
    if (g_memory.freed_count < g_memory.freed_capacity) {
        g_memory.freed_pointers[g_memory.freed_count++] = ptr;
    }
    #endif

    UpeMemoryDiag_RecordFreedPointer(ptr);
}

/*==============================================================================
  PUBLIC API
==============================================================================*/

void UpeMemory_Initialize(const UpeMemoryBudget *budget, size_t frame_arena_size_mb) {
    assert(!g_initialized);
    assert(budget);
    
    memset(&g_memory, 0, sizeof(g_memory));
    
    g_memory.budget = *budget;
    g_memory.frame_arena.capacity = frame_arena_size_mb * 1024 * 1024;
    g_memory.frame_arena.buffer = (uint8_t*)malloc(g_memory.frame_arena.capacity);
    
    if (!g_memory.frame_arena.buffer) {
        fprintf(stderr, "Failed to allocate frame arena (%zu MB)\n", frame_arena_size_mb);
        return;
    }
    
    g_initialized = 1;
}

void UpeMemory_Shutdown(void) {
    if (!g_initialized) return;
    
    UpeMemory_PrintLeakReport();
    
    free(g_memory.frame_arena.buffer);
    free(g_memory.tracked_allocs);
    free(g_memory.alloc_infos);
    free(g_memory.freed_pointers);
    
    memset(&g_memory, 0, sizeof(g_memory));
    g_initialized = 0;
}

void* UpeMemory_Alloc(size_t size, size_t alignment, UpeAllocatorType allocator,
                      UpeMemoryTag tag, uint32_t callsite_hash) {
    if (!g_initialized) {
        return malloc(size);
    }
    
    void *ptr = NULL;
    
    if (allocator == UPE_ALLOCATOR_FRAME) {
        ptr = UpeFrameArena_Alloc(&g_memory.frame_arena, size, alignment);
    } else {
        /* Use system malloc for other allocators (in production, use custom pools) */
        ptr = malloc(size);
    }
    
    if (ptr) {
        _track_allocation(ptr, size, alignment, allocator, tag, callsite_hash);
    }
    
    return ptr;
}

void UpeMemory_Free(void *ptr) {
    if (!ptr || !g_initialized) {
        free(ptr);
        return;
    }
    
    _untrack_allocation(ptr);
    free(ptr);
}

void UpeMemory_ResetFrameArena(void) {
    if (!g_initialized) return;
    
    g_memory.stats.frame_arena_peak = g_memory.frame_arena.peak_frame;
    g_memory.frame_arena.offset = 0;
    g_memory.frame_arena.peak_frame = 0;
    g_memory.stats.allocations_this_frame = 0;
    g_memory.stats.deallocations_this_frame = 0;
    g_memory.frame_number++;
}

void UpeMemory_GetStats(UpeMemoryStats *out_stats) {
    if (!g_initialized || !out_stats) return;
    *out_stats = g_memory.stats;
}

int UpeMemory_CheckBudgets(void) {
    if (!g_initialized) return 1;
    
    if (g_memory.stats.total_allocated > g_memory.budget.global_max) {
        fprintf(stderr, "BUDGET VIOLATION: Total allocated %" PRIu64 " > global max %" PRIu64 "\n",
                g_memory.stats.total_allocated, g_memory.budget.global_max);
        return 0;
    }
    
    return 1;
}

void UpeMemory_PrintLeakReport(void) {
    if (!g_initialized) return;
    
    printf("\n=== MEMORY LEAK REPORT ===\n");
    printf("Total allocated: %" PRIu64 " bytes\n", g_memory.stats.total_allocated);
    printf("Total allocations: %" PRIu64 " live\n", g_memory.stats.allocation_count);
    printf("Peak allocated: %" PRIu64 " bytes\n", g_memory.stats.peak_allocated);
    printf("Frame arena peak: %" PRIu64 " bytes\n", g_memory.stats.frame_arena_peak);
    
    printf("\nBytes by subsystem:\n");
    const char *tag_names[] = {
        "BROADPHASE", "CONTACTS", "CONSTRAINTS", "PARTICLES", "FLUIDS", "TERRAIN",
        "MESHES", "DEBUG", "FIELDS", "ANIMATION", "CACHE", "OTHER"
    };
    for (int i = 0; i < 12; i++) {
        if (g_memory.stats.bytes_by_tag[i] > 0) {
            printf("  %s: %" PRIu64 " bytes\n", tag_names[i], g_memory.stats.bytes_by_tag[i]);
        }
    }
    
    printf("\nBytes by allocator:\n");
    const char *alloc_names[] = {"STATIC", "WORLD", "FRAME", "JOB", "GENERAL"};
    for (int i = 0; i < 5; i++) {
        if (g_memory.stats.bytes_by_allocator[i] > 0) {
            printf("  %s: %" PRIu64 " bytes\n", alloc_names[i], g_memory.stats.bytes_by_allocator[i]);
        }
    }
    
    if (g_memory.stats.total_allocated > 0) {
        printf("\nLive allocations:\n");
        for (uint32_t i = 0; i < g_memory.alloc_count && i < 20; i++) {
            UpeAllocationInfo *info = &g_memory.alloc_infos[i];
            printf("  [%u] %zu bytes (tag=%d, frame=%u)\n", i, info->size, info->tag, info->frame_number);
        }
        if (g_memory.alloc_count > 20) {
            printf("  ... and %u more\n", g_memory.alloc_count - 20);
        }
    }
    printf("=======================\n\n");
}

UpeMemoryPressure UpeMemory_GetPressure(void) {
    if (!g_initialized) return UPE_MEMORY_OK;
    
    uint64_t percent = (g_memory.stats.total_allocated * 100) / g_memory.budget.global_max;
    
    if (percent >= 90) return UPE_MEMORY_CRITICAL;
    if (percent >= 75) return UPE_MEMORY_LOW;
    return UPE_MEMORY_OK;
}

void UpeMemory_NotifyPressure(UpeMemoryPressure pressure) {
    if (!g_initialized) return;

    switch (pressure) {
        case UPE_MEMORY_OK:
            break;
        case UPE_MEMORY_LOW:
            if (g_memory.frame_arena.offset > g_memory.frame_arena.capacity / 2u) {
                g_memory.frame_arena.offset = g_memory.frame_arena.capacity / 2u;
            }
            break;
        case UPE_MEMORY_CRITICAL:
            g_memory.frame_arena.offset = 0;
            g_memory.stats.allocations_this_frame = 0;
            g_memory.stats.deallocations_this_frame = 0;
            break;
        default:
            break;
    }
}

#ifdef UPE_DEBUG_MEMORY

void UpeMemory_PoisonNew(void *ptr, size_t size) {
    memset(ptr, 0xCD, size);  /* 0xCD = used but uninitialized */
}

void UpeMemory_PoisonFree(void *ptr, size_t size) {
    memset(ptr, 0xDD, size);  /* 0xDD = freed memory */
}

int UpeMemory_ValidateCanaries(void) {
    if (!g_initialized) {
        return 1;
    }

    if (!g_memory.frame_arena.buffer && g_memory.frame_arena.capacity > 0) {
        return 0;
    }
    if (g_memory.frame_arena.offset > g_memory.frame_arena.capacity) {
        return 0;
    }
    if (g_memory.alloc_count > g_memory.alloc_capacity) {
        return 0;
    }
    if (g_memory.freed_count > g_memory.freed_capacity) {
        return 0;
    }

    for (uint32_t i = 0; i < g_memory.alloc_count; i++) {
        if (!g_memory.tracked_allocs[i]) {
            return 0;
        }
        if (g_memory.alloc_infos[i].size == 0) {
            return 0;
        }
    }

    return 1;
}

int UpeMemory_CheckUseAfterFree(void *ptr) {
    for (uint32_t i = 0; i < g_memory.freed_count; i++) {
        if (g_memory.freed_pointers[i] == ptr) {
            fprintf(stderr, "Use-after-free detected: %p\n", ptr);
            return 0;
        }
    }
    return 1;
}

#endif /* UPE_DEBUG_MEMORY */
