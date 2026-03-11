#include "upe/upe_manifold_cache.h"
#include "upe/upe_math.h"
#include <stdlib.h>
#include <string.h>

void UpeManifoldCache_Init(UpeManifoldCache *cache, uint32_t initial_capacity) {
    if (!cache) return;
    
    cache->manifold_capacity = initial_capacity > 0 ? initial_capacity : 256;
    
    cache->manifolds = (UpeCachedManifold *)malloc(
        cache->manifold_capacity * sizeof(UpeCachedManifold));
    cache->manifold_count = 0;
    cache->current_frame = 0;
    cache->max_lifetime = 4;  /* Keep manifolds ~4 frames after last contact */
    cache->cache_hits = 0;
    cache->cache_misses = 0;
    cache->cache_evictions = 0;
}

void UpeManifoldCache_Destroy(UpeManifoldCache *cache) {
    if (!cache || !cache->manifolds) return;
    
    free(cache->manifolds);
    memset(cache, 0, sizeof(UpeManifoldCache));
}

uint32_t UpeManifold_GenerateID(uint32_t body_a, uint32_t body_b,
                                uint32_t shape_a, uint32_t shape_b) {
    /* Simple hash: combine body IDs (ensure consistent ordering) */
    uint32_t min_body = body_a < body_b ? body_a : body_b;
    uint32_t max_body = body_a < body_b ? body_b : body_a;
    
    /* XOR to create unique ID (not cryptographically secure, but good for hashing) */
    return (min_body ^ (max_body << 16)) ^ (shape_a * 73856093) ^ (shape_b * 19349663);
}

static int UpeManifoldCache_GrowStorage(UpeManifoldCache *cache, uint32_t target_capacity) {
    if (target_capacity <= cache->manifold_capacity) return 1;
    
    uint32_t new_capacity = cache->manifold_capacity * 2;
    while (new_capacity < target_capacity) new_capacity *= 2;
    
    UpeCachedManifold *new_manifolds = (UpeCachedManifold *)malloc(
        new_capacity * sizeof(UpeCachedManifold));
    if (!new_manifolds) return 0;
    
    if (cache->manifolds && cache->manifold_count > 0) {
        memcpy(new_manifolds, cache->manifolds, 
               cache->manifold_count * sizeof(UpeCachedManifold));
        free(cache->manifolds);
    }
    
    cache->manifolds = new_manifolds;
    cache->manifold_capacity = new_capacity;
    return 1;
}

UpeCachedManifold *UpeManifoldCache_Find(UpeManifoldCache *cache, uint32_t body_a, uint32_t body_b,
                                         uint32_t shape_a, uint32_t shape_b) {
    if (!cache || !cache->manifolds) return NULL;
    
    uint32_t id = UpeManifold_GenerateID(body_a, body_b, shape_a, shape_b);
    
    /* Linear search (could hash if needed, but linear is fine for small counts) */
    for (uint32_t i = 0; i < cache->manifold_count; i++) {
        if (cache->manifolds[i].manifold_id == id &&
            cache->manifolds[i].is_valid) {
            cache->cache_hits++;
            return &cache->manifolds[i];
        }
    }
    
    return NULL;
}

UpeCachedManifold *UpeManifoldCache_CreateOrUpdate(UpeManifoldCache *cache, uint32_t body_a, uint32_t body_b,
                                                    uint32_t shape_a, uint32_t shape_b) {
    if (!cache) return NULL;
    
    /* Try to find existing */
    UpeCachedManifold *existing = UpeManifoldCache_Find(cache, body_a, body_b, shape_a, shape_b);
    if (existing) {
        existing->frame_last_updated = cache->current_frame;
        return existing;
    }
    
    cache->cache_misses++;
    
    /* Grow if needed */
    if (cache->manifold_count >= cache->manifold_capacity) {
        if (!UpeManifoldCache_GrowStorage(cache, cache->manifold_count + 1)) {
            return NULL;
        }
    }
    
    /* Create new manifold */
    UpeCachedManifold *new_manifold = &cache->manifolds[cache->manifold_count];
    memset(new_manifold, 0, sizeof(UpeCachedManifold));
    
    new_manifold->body_a = body_a;
    new_manifold->body_b = body_b;
    new_manifold->shape_a = shape_a;
    new_manifold->shape_b = shape_b;
    new_manifold->manifold_id = UpeManifold_GenerateID(body_a, body_b, shape_a, shape_b);
    new_manifold->frame_created = cache->current_frame;
    new_manifold->frame_last_updated = cache->current_frame;
    new_manifold->is_valid = 1;
    new_manifold->point_count = 0;
    new_manifold->friction = 0.5f;      /* Default friction */
    new_manifold->restitution = 0.0f;   /* Default no bounce */
    
    cache->manifold_count++;
    return new_manifold;
}

void UpeManifold_AddPoint(UpeCachedManifold *manifold, UpeVec3 world_point_a, UpeVec3 world_point_b,
                          float penetration, uint32_t feature_id) {
    if (!manifold || manifold->point_count >= UPE_MAX_MANIFOLD_POINTS) return;
    
    /* Check if we already have this point (by feature ID) */
    for (uint32_t i = 0; i < manifold->point_count; i++) {
        if (manifold->points[i].feature_id == feature_id) {
            /* Update existing point */
            manifold->points[i].local_point_a = world_point_a;
            manifold->points[i].local_point_b = world_point_b;
            manifold->points[i].penetration = penetration;
            manifold->points[i].is_active = 1;
            return;
        }
    }
    
    /* Add new point */
    UpeManifoldPoint *point = &manifold->points[manifold->point_count];
    point->local_point_a = world_point_a;
    point->local_point_b = world_point_b;
    point->penetration = penetration;
    point->feature_id = feature_id;
    point->normal_impulse = 0.0f;
    point->tangent_impulse[0] = 0.0f;
    point->tangent_impulse[1] = 0.0f;
    point->is_active = 1;
    
    manifold->point_count++;
}

void UpeManifoldCache_Touch(UpeManifoldCache *cache, UpeCachedManifold *manifold) {
    if (!manifold) return;
    manifold->frame_last_updated = cache->current_frame;
    manifold->is_valid = 1;
}

void UpeManifoldCache_Cleanup(UpeManifoldCache *cache) {
    if (!cache || !cache->manifolds) return;
    
    uint32_t lifetime_threshold = cache->current_frame > cache->max_lifetime ? 
                                   cache->current_frame - cache->max_lifetime : 0;
    
    /* Mark old manifolds as invalid */
    for (uint32_t i = 0; i < cache->manifold_count; i++) {
        if (cache->manifolds[i].frame_last_updated < lifetime_threshold) {
            cache->manifolds[i].is_valid = 0;
            cache->cache_evictions++;
        }
    }
    
    /* Compact array (remove invalid entries) */
    uint32_t write_idx = 0;
    for (uint32_t i = 0; i < cache->manifold_count; i++) {
        if (cache->manifolds[i].is_valid) {
            if (write_idx != i) {
                memcpy(&cache->manifolds[write_idx], &cache->manifolds[i], 
                       sizeof(UpeCachedManifold));
            }
            write_idx++;
        }
    }
    cache->manifold_count = write_idx;
}

void UpeManifoldCache_AdvanceFrame(UpeManifoldCache *cache) {
    if (!cache) return;
    cache->current_frame++;
}

void UpeManifoldCache_GetStats(const UpeManifoldCache *cache, uint32_t *cache_hits, uint32_t *cache_misses,
                               uint32_t *active_manifolds) {
    if (!cache) return;
    if (cache_hits) *cache_hits = cache->cache_hits;
    if (cache_misses) *cache_misses = cache->cache_misses;
    if (active_manifolds) *active_manifolds = cache->manifold_count;
}

void UpeManifoldCache_ResetStats(UpeManifoldCache *cache) {
    if (!cache) return;
    cache->cache_hits = 0;
    cache->cache_misses = 0;
    cache->cache_evictions = 0;
}

int UpeManifoldPoint_IsValid(const UpeManifoldPoint *point, UpeVec3 world_point_a, UpeVec3 world_point_b,
                              float tolerance) {
    if (!point) return 0;
    
    /* Check if the cached point is still close to the actual contact */
    float dx_a = point->local_point_a.x - world_point_a.x;
    float dy_a = point->local_point_a.y - world_point_a.y;
    float dz_a = point->local_point_a.z - world_point_a.z;
    float dist_a = dx_a * dx_a + dy_a * dy_a + dz_a * dz_a;
    
    float dx_b = point->local_point_b.x - world_point_b.x;
    float dy_b = point->local_point_b.y - world_point_b.y;
    float dz_b = point->local_point_b.z - world_point_b.z;
    float dist_b = dx_b * dx_b + dy_b * dy_b + dz_b * dz_b;
    
    float tol_sq = tolerance * tolerance;
    return (dist_a < tol_sq && dist_b < tol_sq) ? 1 : 0;
}
