#ifndef UPE_THREADING_H
#define UPE_THREADING_H

#include <stdint.h>

// ============================================================================
// THREADING HEADER
//
// - Deterministic thread pool (fixed worker threads)
// - Work-stealing for physics constraint solving
// - Graph coloring for parallel constraint groups
// - Cross-platform (Windows/Linux/macOS)
// ============================================================================

typedef void (*UpeThreadWorkFunc)(void *data);

typedef struct UpeThreadWorkItem {
    UpeThreadWorkFunc func;
    void *data;
} UpeThreadWorkItem;

// Thread pool management
void UpeThreading_Init(uint32_t num_threads);
void UpeThreading_Shutdown(void);

// Work submission
uint32_t UpeThreading_SubmitWork(UpeThreadWorkFunc func, void *data);
uint32_t UpeThreading_SubmitBatchWork(const UpeThreadWorkItem *items, uint32_t count);

// Synchronization
void UpeThreading_WaitAll(void);
uint32_t UpeThreading_GetActiveTaskCount(void);
uint32_t UpeThreading_GetThreadCount(void);

// Graph coloring for constraint parallelization
uint32_t UpeThreading_ColorConstraintGraph(
    const uint32_t *constraint_graph, uint32_t num_constraints,
    uint32_t *out_colors);

#endif // UPE_THREADING_H
