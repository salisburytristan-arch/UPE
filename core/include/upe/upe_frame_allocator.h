#ifndef UPE_FRAME_ALLOCATOR_H
#define UPE_FRAME_ALLOCATOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Frame allocator: bump allocator that resets each frame
 *
 * Used for temporary allocations that don't outlive a single time step:
 * - Contact manifolds
 * - Collision pairs
 * - Constraint graph
 * - Physics queries
 *
 * Allocations are extremely fast (pointer increment) and memory is automatically
 * reclaimed at frame boundary with no fragmentation.
 */

typedef struct {
    uint8_t *memory;
    uint32_t capacity;
    uint32_t offset;         /* Current allocation position */
    uint32_t peak_offset;    /* Peak usage statistics */
} UpeFrameAllocator;

/**
 * Create a frame allocator with given capacity
 */
UpeFrameAllocator *UpeFrameAllocator_Create(uint32_t capacity);

/**
 * Destroy frame allocator and free underlying memory
 */
void UpeFrameAllocator_Destroy(UpeFrameAllocator *alloc);

/**
 * Allocate bytes from frame allocator
 *
 * Returns NULL if allocation would exceed capacity.
 * All allocations are naturally aligned at 64-byte boundaries.
 */
void *UpeFrameAllocator_Allocate(UpeFrameAllocator *alloc, uint32_t size, uint32_t alignment);

/**
 * Reset allocator to beginning of frame (deallocates all allocations)
 *
 * No individual free() calls; all memory is reclaimed at once.
 */
void UpeFrameAllocator_Reset(UpeFrameAllocator *alloc);

/**
 * Get current allocation statistics
 */
void UpeFrameAllocator_GetStats(UpeFrameAllocator *alloc, uint32_t *allocated,
                                 uint32_t *available, uint32_t *peak_used);

/**
 * Allocate typed array from frame allocator
 */
#define UpeFrameAllocator_AllocArray(alloc, type, count) \
    (type *)UpeFrameAllocator_Allocate(alloc, (count) * sizeof(type), alignof(type))

/**
 * Thread-local frame allocator for scratch memory
 *
 * Each thread has its own instance to avoid synchronization.
 * Useful for temporary working memory during physics updates.
 */
typedef struct {
    UpeFrameAllocator *frame_allocator;
} UpeThreadLocalAllocator;

/**
 * Get thread-local frame allocator
 *
 * Creates on first call, returns same instance on subsequent calls.
 */
UpeThreadLocalAllocator *UpeFrameAllocator_GetThreadLocal(void);

/**
 * Specify capacity for thread-local allocator
 *
 * Should be called once per thread before using physics engine.
 */
void UpeFrameAllocator_SetThreadLocalCapacity(uint32_t capacity);

/**
 * Reset thread-local allocator
 */
void UpeFrameAllocator_ResetThreadLocal(void);

/**
 * Arena marker for stack-based allocation patterns
 *
 * Allows allocating temporary data and rolling back to a saved point.
 */
typedef struct {
    uint32_t saved_offset;
} UpeFrameAllocatorMarker;

/**
 * Save current allocation position
 */
UpeFrameAllocatorMarker UpeFrameAllocator_SaveMarker(UpeFrameAllocator *alloc);

/**
 * Restore allocator to saved position (frees all allocations since marker)
 */
void UpeFrameAllocator_RestoreMarker(UpeFrameAllocator *alloc, UpeFrameAllocatorMarker marker);

#ifdef __cplusplus
}
#endif

#endif /* UPE_FRAME_ALLOCATOR_H */
