#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  UNIFIED PHYSICS ENGINE - MEMORY POLICY & ALLOCATION ARCHITECTURE
  
  Core design: No leaks, deterministic, zero allocations during World::Step()
  except for pre-reserved frame arena and labeled pool objects.
  
  Allocation Lifetime Classes:
  - STATIC: engine startup → shutdown
  - WORLD: level load → level unload (must return to baseline)
  - FRAME: one tick (reset each frame)
  - JOB: short-lived task (worker thread scratch)
  - RESOURCE: assets with handles + refcounts
==============================================================================*/

/*==================== MEMORY BUDGET DECLARATION ====================*/

typedef struct UpeMemoryBudget {
    uint64_t global_max;              /* Hard global RAM cap */
    uint64_t broadphase_pairs_max;    /* Candidate pair buffer */
    uint64_t contacts_max;            /* Contact/manifold pool */
    uint64_t constraints_max;         /* Constraints/joints pool */
    uint64_t particles_max;           /* Particle storage */
    uint64_t terrain_chunks_max;      /* Heightfield/voxel cells */
    uint64_t meshes_max;              /* Mesh + BVH storage */
    uint64_t frame_scratch_max;       /* Per-tick arena scratch */
    uint64_t debug_telemetry_max;     /* Debug draw + profiles */
} UpeMemoryBudget;

/*==================== ALLOCATOR IDENTIFIERS ====================*/

typedef enum {
    UPE_ALLOCATOR_STATIC = 0,     /* Long-lived engine structures */
    UPE_ALLOCATOR_WORLD = 1,      /* Level-scoped lifetime */
    UPE_ALLOCATOR_FRAME = 2,      /* Reset every frame */
    UPE_ALLOCATOR_JOB = 3,        /* Thread-local job scratch */
    UPE_ALLOCATOR_GENERAL = 4     /* Fallback (dev/editor only) */
} UpeAllocatorType;

/*==================== SUBSYSTEM TAGS (bitmask) ====================*/

typedef enum {
    UPE_TAG_BROADPHASE = 0x01,
    UPE_TAG_CONTACTS = 0x02,
    UPE_TAG_CONSTRAINTS = 0x04,
    UPE_TAG_PARTICLES = 0x08,
    UPE_TAG_FLUIDS = 0x10,
    UPE_TAG_TERRAIN = 0x20,
    UPE_TAG_MESHES = 0x40,
    UPE_TAG_DEBUG = 0x80,
    UPE_TAG_FIELDS = 0x100,
    UPE_TAG_ANIMATION = 0x200,
    UPE_TAG_CACHE = 0x400,
    UPE_TAG_OTHER = 0x800
} UpeMemoryTag;

/*==================== ALLOCATION METADATA ====================*/

typedef struct UpeAllocationInfo {
    size_t size;                      /* Block size in bytes */
    size_t alignment;                 /* Requested alignment */
    UpeAllocatorType allocator;       /* Which allocator owns this */
    UpeMemoryTag tag;                 /* Subsystem tag */
    uint32_t callsite_hash;           /* file:line or return address */
    uint32_t thread_id;               /* Thread that allocated */
    uint32_t frame_number;            /* Frame counter at alloc time */
    uint64_t timestamp_us;            /* Microsecond timestamp */
} UpeAllocationInfo;

/*==================== ALLOCATOR INTERFACE ====================*/

typedef void* (*UpeAllocFn)(size_t size, size_t alignment, 
                             UpeAllocatorType allocator, UpeMemoryTag tag,
                             uint32_t callsite_hash);
typedef void (*UpeFreeFn)(void *ptr);
typedef void (*UpeResetFn)(void);  /* Reset frame arena */

typedef struct {
    UpeAllocFn alloc;
    UpeFreeFn free;
    UpeResetFn reset_frame;
} UpeAllocatorInterface;

/*==================== MEMORY STATISTICS ====================*/

typedef struct UpeMemoryStats {
    uint64_t total_allocated;         /* Total bytes in use */
    uint64_t peak_allocated;          /* Peak since last reset */
    uint64_t allocation_count;        /* Number of live allocations */
    uint64_t allocations_this_frame;  /* Allocations in current frame */
    uint64_t deallocations_this_frame;
    uint64_t bytes_by_tag[16];        /* Per-tag breakdown */
    uint64_t bytes_by_allocator[5];   /* Per-allocator breakdown */
    uint64_t frame_arena_used;        /* Current frame arena usage */
    uint64_t frame_arena_peak;        /* Peak frame arena in this frame */
} UpeMemoryStats;

/*==================== CORE MEMORY FUNCTIONS ====================*/

/* Initialize global memory system */
void UpeMemory_Initialize(const UpeMemoryBudget *budget, size_t frame_arena_size_mb);

/* Shutdown (validate all freed) */
void UpeMemory_Shutdown(void);

/* Primary allocation function (wrapped by convenience macros) */
void* UpeMemory_Alloc(size_t size, size_t alignment, UpeAllocatorType allocator,
                      UpeMemoryTag tag, uint32_t callsite_hash);

/* Primary free function */
void UpeMemory_Free(void *ptr);

/* Reset frame arena (call once per tick) */
void UpeMemory_ResetFrameArena(void);

/* Query stats */
void UpeMemory_GetStats(UpeMemoryStats *out_stats);

/* Check budget compliance */
int UpeMemory_CheckBudgets(void);

/* Leak report at shutdown or level unload */
void UpeMemory_PrintLeakReport(void);

/*==================== CONVENIENCE MACROS ====================*/

#define UPE_ALLOC(size, allocator, tag) \
    UpeMemory_Alloc(size, 16, allocator, tag, (uint32_t)(uintptr_t)__builtin_return_address(0))

#define UPE_ALLOC_ALIGNED(size, align, allocator, tag) \
    UpeMemory_Alloc(size, align, allocator, tag, (uint32_t)(uintptr_t)__builtin_return_address(0))

#define UPE_FREE(ptr) UpeMemory_Free(ptr)

/*==================== POOL ALLOCATOR INTERFACE ====================*/

typedef struct UpePoolAllocator {
    void *pool;                       /* Dense array of objects */
    size_t object_size;
    uint32_t capacity;
    uint32_t count;
    uint32_t *generations;            /* For handle validation */
    uint32_t first_free;              /* Free list head */
} UpePoolAllocator;

/* Create fixed-size pool (e.g., for contacts, constraints, manifolds) */
UpePoolAllocator* UpePoolAllocator_Create(size_t object_size, uint32_t capacity,
                                          UpeAllocatorType allocator, UpeMemoryTag tag);

void UpePoolAllocator_Destroy(UpePoolAllocator *pool);

/* Allocate from pool, returns (index, generation) packed as uint64 */
typedef struct {
    uint32_t index;
    uint32_t generation;
} UpePoolHandle;

UpePoolHandle UpePoolAllocator_Alloc(UpePoolAllocator *pool);
void UpePoolAllocator_Free(UpePoolAllocator *pool, UpePoolHandle handle);
void* UpePoolAllocator_GetPtr(UpePoolAllocator *pool, UpePoolHandle handle);
int UpePoolAllocator_IsValid(UpePoolAllocator *pool, UpePoolHandle handle);

/*==================== FRAME ARENA INTERFACE ====================*/

typedef struct UpeFrameArena {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
    size_t peak_frame;
} UpeFrameArena;

/* Allocate from frame arena (no free, just reset at frame end) */
void* UpeFrameArena_Alloc(UpeFrameArena *arena, size_t size, size_t alignment);

/* Get current arena stats */
void UpeFrameArena_GetStats(UpeFrameArena *arena, size_t *out_used, size_t *out_peak);

/*==================== DEBUG GUARDS (debug builds only) ====================*/

#ifdef UPE_DEBUG_MEMORY

/* Fill newly allocated memory with pattern */
void UpeMemory_PoisonNew(void *ptr, size_t size);

/* Fill freed memory with pattern */
void UpeMemory_PoisonFree(void *ptr, size_t size);

/* Check canaries on allocation (call periodically) */
int UpeMemory_ValidateCanaries(void);

/* Detect use-after-free by tracking freed pointers */
int UpeMemory_CheckUseAfterFree(void *ptr);

#else
#define UpeMemory_PoisonNew(p, s) do { } while(0)
#define UpeMemory_PoisonFree(p, s) do { } while(0)
#define UpeMemory_ValidateCanaries() 1
#define UpeMemory_CheckUseAfterFree(p) 1
#endif

/*==================== MEMORY PRESSURE EVENTS ====================*/

typedef enum {
    UPE_MEMORY_OK = 0,
    UPE_MEMORY_LOW = 1,               /* 75%+ of budget used */
    UPE_MEMORY_CRITICAL = 2           /* 90%+ of budget used */
} UpeMemoryPressure;

/* Check current memory pressure */
UpeMemoryPressure UpeMemory_GetPressure(void);

/* Request memory reduction (LOD down, drop cache entries, etc.) */
void UpeMemory_NotifyPressure(UpeMemoryPressure pressure);

#ifdef __cplusplus
}
#endif
