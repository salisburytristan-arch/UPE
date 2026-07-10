#include "upe/upe_memory_context.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/*==============================================================================
  MEMORY CONTEXT IMPLEMENTATION
==============================================================================*/

typedef struct {
    float pos[3];
    float normal[3];
    float depth;
    uint32_t pad;
} UpeContactStub;

typedef struct {
    UpePoolHandle contacts[16];
    uint32_t count;
} UpeManifoldStub;

typedef struct {
    uint32_t body_a;
    uint32_t body_b;
    float stiffness;
} UpeConstraintStub;

typedef struct {
    float pos[3];
    float vel[3];
    float mass;
} UpeParticleStub;

typedef struct {
    uint32_t shape_id;
    float mass;
} UpeBodyStub;

typedef struct {
    uint32_t type;
    float scale[3];
} UpeShapeStub;

typedef struct {
    uint32_t body_a;
    uint32_t body_b;
    float limit;
} UpeJointStub;

UpeMemoryContext* UpeMemoryContext_Create(const UpeMemoryBudget *budget,
                                          size_t frame_arena_size_mb) {
    assert(budget);
    
    UpeMemoryContext *ctx = (UpeMemoryContext*)malloc(sizeof(UpeMemoryContext));
    if (!ctx) return NULL;
    
    memset(ctx, 0, sizeof(UpeMemoryContext));
    
    /* Set subsystem budgets from global budget */
    ctx->budget_broadphase = budget->broadphase_pairs_max;
    ctx->budget_contacts = budget->contacts_max;
    ctx->budget_constraints = budget->constraints_max;
    ctx->budget_particles = budget->particles_max;
    ctx->budget_fluids = budget->contacts_max;  /* Estimate */
    ctx->budget_terrain = budget->terrain_chunks_max;
    ctx->budget_meshes = budget->meshes_max;
    ctx->budget_debug = budget->debug_telemetry_max;
    
    /* Create pool allocators */
    ctx->pool_contacts = UpePoolAllocator_Create(
        sizeof(UpeContactStub),
        1024,
        UPE_ALLOCATOR_WORLD,
        UPE_TAG_CONTACTS
    );
    
    ctx->pool_manifolds = UpePoolAllocator_Create(
        sizeof(UpeManifoldStub),
        256,
        UPE_ALLOCATOR_WORLD,
        UPE_TAG_CONTACTS
    );
    
    ctx->pool_constraints = UpePoolAllocator_Create(
        sizeof(UpeConstraintStub),
        512,
        UPE_ALLOCATOR_WORLD,
        UPE_TAG_CONSTRAINTS
    );
    
    ctx->pool_particles = UpePoolAllocator_Create(
        sizeof(UpeParticleStub),
        2048,
        UPE_ALLOCATOR_WORLD,
        UPE_TAG_PARTICLES
    );
    
    /* Create handle pools for objects */
    ctx->pool_bodies = UpeHandlePool_Create(
        sizeof(UpeBodyStub),
        512
    );
    
    ctx->pool_shapes = UpeHandlePool_Create(
        sizeof(UpeShapeStub),
        512
    );
    
    ctx->pool_joints = UpeHandlePool_Create(
        sizeof(UpeJointStub),
        256
    );
    
    /* Create frame arena */
    ctx->frame_arena = (UpeFrameArena*)malloc(sizeof(UpeFrameArena));
    if (ctx->frame_arena) {
        ctx->frame_arena->capacity = frame_arena_size_mb * 1024 * 1024;
        ctx->frame_arena->buffer = (uint8_t*)malloc(ctx->frame_arena->capacity);
        ctx->frame_arena->offset = 0;
        ctx->frame_arena->peak_frame = 0;
    }
    
    ctx->current_pressure = UPE_MEMORY_OK;
    ctx->current_lod = UPE_LOD_FULL;
    
    return ctx;
}

void UpeMemoryContext_Destroy(UpeMemoryContext *ctx) {
    if (!ctx) return;
    
    UpePoolAllocator_Destroy(ctx->pool_contacts);
    UpePoolAllocator_Destroy(ctx->pool_manifolds);
    UpePoolAllocator_Destroy(ctx->pool_constraints);
    UpePoolAllocator_Destroy(ctx->pool_particles);
    
    UpeHandlePool_Destroy(ctx->pool_bodies);
    UpeHandlePool_Destroy(ctx->pool_shapes);
    UpeHandlePool_Destroy(ctx->pool_joints);
    
    if (ctx->frame_arena) {
        free(ctx->frame_arena->buffer);
        free(ctx->frame_arena);
    }
    
    free(ctx);
}

/*==================== FRAME MANAGEMENT ====================*/

void UpeMemoryContext_PrepareFrame(UpeMemoryContext *ctx) {
    assert(ctx);
    
    /* Reset frame arena at start of tick */
    if (ctx->frame_arena) {
        ctx->frame_arena->offset = 0;
        ctx->frame_arena->peak_frame = 0;
    }
    
    /* Check memory pressure */
    ctx->current_pressure = UpeMemory_GetPressure();
    ctx->current_lod = UpeMemoryDiag_GetCurrentLOD();
}

void UpeMemoryContext_FinalizeFrame(UpeMemoryContext *ctx, double frame_time_ms) {
    assert(ctx);

    uint32_t contact_count = ctx->pool_contacts ? ctx->pool_contacts->count : 0u;
    uint32_t manifold_count = ctx->pool_manifolds ? ctx->pool_manifolds->count : 0u;
    uint32_t constraint_count = ctx->pool_constraints ? ctx->pool_constraints->count : 0u;
    uint32_t particle_count = ctx->pool_particles ? ctx->pool_particles->count : 0u;
    uint32_t body_count = (ctx->pool_bodies && ctx->pool_bodies->table) ? ctx->pool_bodies->table->count : 0u;

    uint64_t candidate_pairs = (body_count > 1u)
        ? ((uint64_t)body_count * (uint64_t)(body_count - 1u)) / 2u
        : 0u;
    if (candidate_pairs > ctx->budget_broadphase) {
        candidate_pairs = ctx->budget_broadphase;
    }

    UpeMemoryDiag_RecordFrameMetrics(
        frame_time_ms,
        contact_count,
        manifold_count,
        constraint_count,
        particle_count,
        (uint32_t)candidate_pairs
    );
}

/*==================== BUDGET ENFORCEMENT ====================*/

int UpeMemoryContext_CanAllocate(UpeMemoryContext *ctx, UpeMemoryTag tag,
                                 uint64_t requested_bytes) {
    assert(ctx);
    
    UpeMemoryStats stats;
    UpeMemory_GetStats(&stats);
    
    /* Under critical pressure, deny non-essential allocations */
    if (ctx->current_lod == UPE_LOD_CRITICAL) {
        if (tag != UPE_TAG_CONTACTS && tag != UPE_TAG_CONSTRAINTS) {
            return 0;  /* Don't allocate non-physics under critical pressure */
        }
    }
    
    /* Check per-tag budget */
    if (tag == UPE_TAG_CONTACTS && stats.bytes_by_tag[UPE_TAG_CONTACTS] + requested_bytes > ctx->budget_contacts) {
        return 0;
    }
    if (tag == UPE_TAG_CONSTRAINTS && stats.bytes_by_tag[UPE_TAG_CONSTRAINTS] + requested_bytes > ctx->budget_constraints) {
        return 0;
    }
    if (tag == UPE_TAG_PARTICLES && stats.bytes_by_tag[UPE_TAG_PARTICLES] + requested_bytes > ctx->budget_particles) {
        return 0;
    }
    
    return 1;
}

void* UpeMemoryContext_AllocSubsystem(UpeMemoryContext *ctx, UpeMemoryTag tag,
                                      size_t size, size_t alignment) {
    assert(ctx);
    
    if (!UpeMemoryContext_CanAllocate(ctx, tag, size)) {
        return NULL;
    }
    
    return UpeMemory_Alloc(size, alignment, UPE_ALLOCATOR_WORLD, tag, 0);
}

/*==================== POOL MANAGEMENT ====================*/

UpePoolHandle UpeMemoryContext_AllocContact(UpeMemoryContext *ctx) {
    assert(ctx && ctx->pool_contacts);
    
    UpePoolHandle handle = UpePoolAllocator_Alloc(ctx->pool_contacts);
    
    if (handle.index == 0xFFFFFFFF) {
        fprintf(stderr, "Contact pool exhausted!\n");
    }
    
    return handle;
}

void UpeMemoryContext_FreeContact(UpeMemoryContext *ctx, UpePoolHandle handle) {
    assert(ctx && ctx->pool_contacts);
    UpePoolAllocator_Free(ctx->pool_contacts, handle);
}

UpePoolHandle UpeMemoryContext_AllocManifold(UpeMemoryContext *ctx) {
    assert(ctx && ctx->pool_manifolds);
    
    UpePoolHandle handle = UpePoolAllocator_Alloc(ctx->pool_manifolds);
    
    if (handle.index == 0xFFFFFFFF) {
        fprintf(stderr, "Manifold pool exhausted!\n");
    }
    
    return handle;
}

void UpeMemoryContext_FreeManifold(UpeMemoryContext *ctx, UpePoolHandle handle) {
    assert(ctx && ctx->pool_manifolds);
    UpePoolAllocator_Free(ctx->pool_manifolds, handle);
}

/*==================== HANDLE-BASED BODY MANAGEMENT ====================*/

UpeBodyRef UpeMemoryContext_CreateBody(UpeMemoryContext *ctx) {
    UpeBodyRef ref = {};
    assert(ctx && ctx->pool_bodies);
    
    ref.handle = UpeHandlePool_Alloc(ctx->pool_bodies);
    ref.body_ptr = UpeHandlePool_Get(ctx->pool_bodies, ref.handle);
    
    return ref;
}

void UpeMemoryContext_DestroyBody(UpeMemoryContext *ctx, UpeBodyRef ref) {
    assert(ctx && ctx->pool_bodies);
    UpeHandlePool_Free(ctx->pool_bodies, ref.handle);
}

UpeBodyRef UpeMemoryContext_GetBodyRef(UpeMemoryContext *ctx, UpeHandle handle) {
    UpeBodyRef ref = {};
    assert(ctx && ctx->pool_bodies);
    
    ref.handle = handle;
    ref.body_ptr = UpeHandlePool_Get(ctx->pool_bodies, handle);
    
    return ref;
}

/*==================== PRESSURE RESPONSE ====================*/

void UpeMemoryContext_OnPressureChanged(UpeMemoryContext *ctx, UpeMemoryPressure pressure) {
    assert(ctx);
    
    ctx->current_pressure = pressure;
    
    /* Update LOD based on pressure */
    switch (pressure) {
        case UPE_MEMORY_OK:
            ctx->current_lod = UPE_LOD_FULL;
            break;
        case UPE_MEMORY_LOW:
            ctx->current_lod = UPE_LOD_REDUCED;
            break;
        case UPE_MEMORY_CRITICAL:
            ctx->current_lod = UPE_LOD_LOW;
            break;
    }
}

void UpeMemoryContext_GetReclaimList(UpeMemoryContext *ctx, UpeMemoryReclaimList *out_list) {
    assert(ctx && out_list);
    
    memset(out_list, 0, sizeof(UpeMemoryReclaimList));
    
    /* In real implementation, would identify non-critical allocations
       (debug draw, optional caches, low-priority particles, etc.) */
}

/*==================== STATISTICS ====================*/

void UpeMemoryContext_GetStats(UpeMemoryContext *ctx, UpeMemoryContextStats *out_stats) {
    assert(ctx && out_stats);
    
    memset(out_stats, 0, sizeof(UpeMemoryContextStats));
    
    UpeMemoryStats global_stats;
    UpeMemory_GetStats(&global_stats);
    
    out_stats->total_allocated = global_stats.total_allocated;
    out_stats->peak_allocated = global_stats.peak_allocated;
    out_stats->fragmentation_ratio = UpeMemoryDiag_GetFragmentationRatio();
    out_stats->current_lod = ctx->current_lod;
    
    /* Count active objects (in real implementation, would be accurate) */
    out_stats->active_bodies = ctx->pool_bodies ? ctx->pool_bodies->table->count : 0;
    out_stats->active_contacts = ctx->pool_contacts ? ctx->pool_contacts->count : 0;
    out_stats->active_manifolds = ctx->pool_manifolds ? ctx->pool_manifolds->count : 0;
    out_stats->active_constraints = ctx->pool_constraints ? ctx->pool_constraints->count : 0;
}

#ifdef UPE_DEBUG_MEMORY

int UpeMemoryContext_ValidateAllStructures(UpeMemoryContext *ctx) {
    assert(ctx);
    
    int valid = 1;
    
    if (!ctx->pool_contacts || ctx->pool_contacts->capacity == 0) {
        fprintf(stderr, "Contact pool invalid\n");
        valid = 0;
    }
    
    if (!ctx->pool_bodies || ctx->pool_bodies->table->capacity == 0) {
        fprintf(stderr, "Body pool invalid\n");
        valid = 0;
    }
    
    /* Validate all allocations for guard corruption */
    if (!UpeMemory_ValidateCanaries()) {
        fprintf(stderr, "Memory corruption detected\n");
        valid = 0;
    }
    
    return valid;
}

void UpeMemoryContext_EmitFrameReport(UpeMemoryContext *ctx, const char *filename) {
    assert(ctx && filename);
    
    FILE *file = fopen(filename, "a");
    if (!file) return;
    
    UpeMemoryContextStats stats;
    UpeMemoryContext_GetStats(ctx, &stats);
    
    fprintf(file, "Memory: %.1f MB / Peak: %.1f MB | Bodies: %u | Contacts: %u | LOD: %d\n",
            stats.total_allocated / 1024.0 / 1024.0,
            stats.peak_allocated / 1024.0 / 1024.0,
            stats.active_bodies,
            stats.active_contacts,
            stats.current_lod);
    
    fclose(file);
}

#endif /* UPE_DEBUG_MEMORY */
