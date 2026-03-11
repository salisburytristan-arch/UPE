#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  GENERATION-BASED HANDLE SYSTEM
  
  Replaces pointers with (index, generation) pairs. Enables:
  - Stable object identity across reallocations
  - Double-free detection
  - Use-after-free prevention
  - Delete-without-invalidating-other-refs simulation
  - Dense array storage without fragmentation
==============================================================================*/

typedef struct {
    uint32_t index;                   /* Array slot */
    uint32_t generation;              /* Increment on free/reuse */
} UpeHandle;

static inline UpeHandle UpeHandle_Invalid(void) {
    UpeHandle h;
    h.index = 0xFFFFFFFF;
    h.generation = 0;
    return h;
}

#define UPE_HANDLE_INVALID UpeHandle_Invalid()

typedef struct UpeHandleTable {
    uint32_t *generations;            /* Current generation for each slot */
    void **objects;                   /* Actual object pointers */
    uint32_t capacity;
    uint32_t count;
    uint32_t first_free;              /* Free list head */
} UpeHandleTable;

/*==================== HANDLE TABLE OPERATIONS ====================*/

/* Create a handle table of fixed capacity */
UpeHandleTable* UpeHandleTable_Create(uint32_t capacity);

/* Destroy the table */
void UpeHandleTable_Destroy(UpeHandleTable *table);

/* Allocate a new handle, returns UPE_HANDLE_INVALID if full */
UpeHandle UpeHandleTable_Alloc(UpeHandleTable *table);

/* Free a handle (invalidates it for later verification) */
void UpeHandleTable_Free(UpeHandleTable *table, UpeHandle handle);

/* Get object from handle (returns NULL if invalid) */
void* UpeHandleTable_Get(UpeHandleTable *table, UpeHandle handle);

/* Store an object (returns the handle it was stored at, or reuses if passed valid handle) */
UpeHandle UpeHandleTable_Set(UpeHandleTable *table, UpeHandle handle, void *object);

/* Verify a handle is still valid */
int UpeHandleTable_IsValid(UpeHandleTable *table, UpeHandle handle);

/* Query table statistics */
typedef struct {
    uint32_t capacity;
    uint32_t count;
    uint32_t free_count;
    double utilization_percent;
} UpeHandleTableStats;

void UpeHandleTable_GetStats(UpeHandleTable *table, UpeHandleTableStats *out_stats);

/*==================== HANDLE POOL (convenience for fixed-size objects) ====================*/

typedef struct {
    UpeHandleTable *table;
    void *objects;                    /* Dense allocation */
    size_t object_size;
} UpeHandlePool;

/* Create pool for N objects of given size */
UpeHandlePool* UpeHandlePool_Create(size_t object_size, uint32_t capacity);

void UpeHandlePool_Destroy(UpeHandlePool *pool);

UpeHandle UpeHandlePool_Alloc(UpeHandlePool *pool);
void UpeHandlePool_Free(UpeHandlePool *pool, UpeHandle handle);
void* UpeHandlePool_Get(UpeHandlePool *pool, UpeHandle handle);
int UpeHandlePool_IsValid(UpeHandlePool *pool, UpeHandle handle);

/*==================== SPARSE SET ITERATION (for dense traversal with handles) ====================*/

typedef struct {
    UpeHandle *handles;               /* All allocated handles */
    uint32_t count;
    uint32_t capacity;
} UpeHandleIterator;

/* Get iterator over all live handles in a table */
void UpeHandleTable_GetIterator(UpeHandleTable *table, UpeHandleIterator *out_iter);

/* Free iterator */
void UpeHandleIterator_Destroy(UpeHandleIterator *iter);

#ifdef __cplusplus
}
#endif
