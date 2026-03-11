#include "upe/upe_frame_allocator.h"
#include <string.h>
#include <stdlib.h>

/**
 * Frame allocator implementation: bump allocator with frame reset
 */

UpeFrameAllocator *UpeFrameAllocator_Create(uint32_t capacity) {
    UpeFrameAllocator *alloc = (UpeFrameAllocator *)malloc(sizeof(UpeFrameAllocator));
    if (!alloc) return NULL;
    
    alloc->capacity = capacity > 0 ? capacity : 1024 * 1024;  /* Default 1MB */
    alloc->memory = (uint8_t *)malloc(alloc->capacity);
    
    if (!alloc->memory) {
        free(alloc);
        return NULL;
    }
    
    alloc->offset = 0;
    alloc->peak_offset = 0;
    
    return alloc;
}

void UpeFrameAllocator_Destroy(UpeFrameAllocator *alloc) {
    if (!alloc) return;
    
    if (alloc->memory) {
        free(alloc->memory);
    }
    
    free(alloc);
}

static uint32_t AlignOffset(uint32_t offset, uint32_t alignment) {
    if (alignment <= 1) return offset;
    return (offset + alignment - 1) & ~(alignment - 1);
}

void *UpeFrameAllocator_Allocate(UpeFrameAllocator *alloc, uint32_t size, uint32_t alignment) {
    if (!alloc || size == 0) return NULL;
    
    if (alignment == 0) alignment = 1;
    
    uint32_t aligned_offset = AlignOffset(alloc->offset, alignment);
    uint32_t needed = aligned_offset + size;
    
    if (needed > alloc->capacity) {
        /* Out of memory */
        return NULL;
    }
    
    uint8_t *ptr = &alloc->memory[aligned_offset];
    alloc->offset = aligned_offset + size;
    
    if (alloc->offset > alloc->peak_offset) {
        alloc->peak_offset = alloc->offset;
    }
    
    return ptr;
}

void UpeFrameAllocator_Reset(UpeFrameAllocator *alloc) {
    if (!alloc) return;
    alloc->offset = 0;
}

void UpeFrameAllocator_GetStats(UpeFrameAllocator *alloc, uint32_t *allocated,
                                 uint32_t *available, uint32_t *peak_used) {
    if (!alloc) {
        if (allocated) *allocated = 0;
        if (available) *available = 0;
        if (peak_used) *peak_used = 0;
        return;
    }
    
    if (allocated) *allocated = alloc->offset;
    if (available) *available = alloc->capacity - alloc->offset;
    if (peak_used) *peak_used = alloc->peak_offset;
}

/* Thread-local allocator storage */
static __thread UpeThreadLocalAllocator *g_tls_allocator = NULL;
static __thread uint32_t g_tls_capacity = 0;

UpeThreadLocalAllocator *UpeFrameAllocator_GetThreadLocal(void) {
    if (g_tls_allocator) return g_tls_allocator;
    
    /* Create on first access */
    g_tls_allocator = (UpeThreadLocalAllocator *)malloc(sizeof(UpeThreadLocalAllocator));
    if (!g_tls_allocator) return NULL;
    
    uint32_t capacity = g_tls_capacity > 0 ? g_tls_capacity : (512 * 1024);  /* Default 512KB per thread */
    g_tls_allocator->frame_allocator = UpeFrameAllocator_Create(capacity);
    
    if (!g_tls_allocator->frame_allocator) {
        free(g_tls_allocator);
        g_tls_allocator = NULL;
        return NULL;
    }
    
    return g_tls_allocator;
}

void UpeFrameAllocator_SetThreadLocalCapacity(uint32_t capacity) {
    g_tls_capacity = capacity;
}

void UpeFrameAllocator_ResetThreadLocal(void) {
    if (!g_tls_allocator || !g_tls_allocator->frame_allocator) return;
    UpeFrameAllocator_Reset(g_tls_allocator->frame_allocator);
}

UpeFrameAllocatorMarker UpeFrameAllocator_SaveMarker(UpeFrameAllocator *alloc) {
    UpeFrameAllocatorMarker marker = {0};
    if (alloc) marker.saved_offset = alloc->offset;
    return marker;
}

void UpeFrameAllocator_RestoreMarker(UpeFrameAllocator *alloc, UpeFrameAllocatorMarker marker) {
    if (!alloc) return;
    if (marker.saved_offset <= alloc->capacity) {
        alloc->offset = marker.saved_offset;
    }
}
