#pragma once

#include "upe/upe_memory_policy.h"
#include "upe/upe_memory_diag.h"
#include "upe/upe_handle.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  MEMORY SUBSYSTEM INTEGRATION
  
  Coordinates memory policy, diagnostics, and handles into unified subsystem.
  Enforces:
  - No heap allocations during World::Step() (only frame arena + pools)
  - Per-subsystem budgets with hard ceilings
  - Leak-proof patterns via generation IDs
  - Allocation telemetry with LOD degradation
==============================================================================*/

/*==================== WORLD-LEVEL MEMORY CONTEXT ====================*/

typedef struct UpeMemoryContext {
    /* Subsystem budgets (tags) */
    uint64_t budget_broadphase;
    uint64_t budget_contacts;
    uint64_t budget_constraints;
    uint64_t budget_particles;
    uint64_t budget_fluids;
    uint64_t budget_terrain;
    uint64_t budget_meshes;
    uint64_t budget_debug;
    
    /* Pool allocators for critical subsystems */
    UpePoolAllocator *pool_contacts;
    UpePoolAllocator *pool_manifolds;
    UpePoolAllocator *pool_constraints;
    UpePoolAllocator *pool_particles;
    
    /* Handle tables for object identification */
    UpeHandlePool *pool_bodies;
    UpeHandlePool *pool_shapes;
    UpeHandlePool *pool_joints;
    
    /* Frame-tick scratch memory */
    UpeFrameArena *frame_arena;
    
    /* Current memory pressure */
    UpeMemoryPressure current_pressure;
    UpeMemoryLOD current_lod;
    
} UpeMemoryContext;

/*==================== INITIALIZATION ====================*/

/* Create memory context for world with given budget */
UpeMemoryContext* UpeMemoryContext_Create(const UpeMemoryBudget *budget,
                                          size_t frame_arena_size_mb);

void UpeMemoryContext_Destroy(UpeMemoryContext *ctx);

/*==================== PRE-TICK PREPARATION ====================*/

/* Call once per tick before World::Step() to prepare frame arena */
void UpeMemoryContext_PrepareFrame(UpeMemoryContext *ctx);

/* Call after World::Step() to reset frame arena and collect metrics */
void UpeMemoryContext_FinalizeFrame(UpeMemoryContext *ctx, double frame_time_ms);

/*==================== BUDGET ENFORCEMENT ====================*/

/* Check if subsystem can allocate more (respecting budget + LOD) */
int UpeMemoryContext_CanAllocate(UpeMemoryContext *ctx, UpeMemoryTag tag, 
                                 uint64_t requested_bytes);

/* Allocate from subsystem budget (may fail under pressure) */
void* UpeMemoryContext_AllocSubsystem(UpeMemoryContext *ctx, UpeMemoryTag tag,
                                      size_t size, size_t alignment);

/*==================== CONTACT/MANIFOLD POOL MANAGEMENT ====================*/

/* Allocate contact from pool with automatic budget enforcement */
UpePoolHandle UpeMemoryContext_AllocContact(UpeMemoryContext *ctx);

void UpeMemoryContext_FreeContact(UpeMemoryContext *ctx, UpePoolHandle handle);

UpePoolHandle UpeMemoryContext_AllocManifold(UpeMemoryContext *ctx);

void UpeMemoryContext_FreeManifold(UpeMemoryContext *ctx, UpePoolHandle handle);

/*==================== HANDLE-BASED BODY MANAGEMENT ====================*/

typedef struct {
    UpeHandle handle;
    void *body_ptr;
} UpeBodyRef;

UpeBodyRef UpeMemoryContext_CreateBody(UpeMemoryContext *ctx);
void UpeMemoryContext_DestroyBody(UpeMemoryContext *ctx, UpeBodyRef ref);
UpeBodyRef UpeMemoryContext_GetBodyRef(UpeMemoryContext *ctx, UpeHandle handle);

/*==================== MEMORY PRESSURE RESPONSE ====================*/

/* Called when memory pressure changes */
void UpeMemoryContext_OnPressureChanged(UpeMemoryContext *ctx, UpeMemoryPressure pressure);

/* Get list of allocations to drop/reduce under pressure */
typedef struct {
    void **objects_to_free;
    uint32_t count;
} UpeMemoryReclaimList;

void UpeMemoryContext_GetReclaimList(UpeMemoryContext *ctx, UpeMemoryReclaimList *out_list);

/*==================== DEBUG MODE FEATURES ====================*/

#ifdef UPE_DEBUG_MEMORY

/* Comprehensive memory check (slow, use in debug only) */
int UpeMemoryContext_ValidateAllStructures(UpeMemoryContext *ctx);

/* Emit memory report for frame */
void UpeMemoryContext_EmitFrameReport(UpeMemoryContext *ctx, const char *filename);

#endif

/*==================== STATISTICS ====================*/

typedef struct {
    uint64_t total_allocated;
    uint64_t peak_allocated;
    uint32_t active_bodies;
    uint32_t active_contacts;
    uint32_t active_manifolds;
    uint32_t active_constraints;
    double fragmentation_ratio;
    UpeMemoryLOD current_lod;
} UpeMemoryContextStats;

void UpeMemoryContext_GetStats(UpeMemoryContext *ctx, UpeMemoryContextStats *out_stats);

#ifdef __cplusplus
}
#endif
