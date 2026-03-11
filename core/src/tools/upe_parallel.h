#ifndef UPE_PARALLEL_H
#define UPE_PARALLEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Deterministic task graph for parallel execution with replay support
typedef struct UpeTask UpeTask;
typedef void (*UpeTaskFunction)(void* data, uint32_t start, uint32_t count);

typedef struct {
    uint32_t task_capacity;
    uint32_t task_count;
    UpeTask* tasks;
    uint32_t worker_count;
    uint32_t deterministic_mode;
} UpeTaskGraph;

UpeTaskGraph* UpeTaskGraph_Create(uint32_t max_tasks, uint32_t worker_count);
void UpeTaskGraph_Destroy(UpeTaskGraph* graph);

// Add task with data range
uint32_t UpeTaskGraph_AddTask(UpeTaskGraph* graph, UpeTaskFunction func, 
                               void* data, uint32_t start, uint32_t count);

// Add dependency between tasks (ensures deterministic ordering)
void UpeTaskGraph_AddDependency(UpeTaskGraph* graph, uint32_t task_id, uint32_t depends_on);

// Execute all tasks respecting dependencies
void UpeTaskGraph_Execute(UpeTaskGraph* graph);

// Reset for next frame
void UpeTaskGraph_Reset(UpeTaskGraph* graph);

#ifdef __cplusplus
}
#endif

#endif // UPE_PARALLEL_H
