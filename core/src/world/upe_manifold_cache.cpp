#include "upe/upe_manifold_cache.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// Manifold Cache Management
// ============================================================================

void UpeManifoldCache_Init(UpeManifoldCache *cache, uint32_t initial_capacity) {
    cache->manifold_capacity = initial_capacity > 0 ? initial_capacity : 512;
    cache->manifolds = (UpeCachedManifold *)malloc(cache->manifold_capacity * sizeof(UpeCachedManifold));
    cache->manifold_count = 0;
    cache->current_frame = 0;
    cache->max_lifetime = 5;  // Keep manifolds for 5 frames after last contact
    
    cache->cache_hits = 0;
    cache->cache_misses = 0;
    cache->cache_evictions = 0;
}

void UpeManifoldCache_Destroy(UpeManifoldCache *cache) {
    free(cache->manifolds);
    cache->manifolds = NULL;
    cache->manifold_count = 0;
    cache->manifold_capacity = 0;
}

// Generate unique manifold ID from body/shape pair
uint32_t UpeManifold_GenerateID(uint32_t body_a, uint32_t body_b, uint32_t shape_a, uint32_t shape_b) {
    // Ensure consistent ordering (smaller ID first)
    if (body_a > body_b) {
        uint32_t temp = body_a;
        body_a = body_b;
        body_b = temp;
        
        temp = shape_a;
        shape_a = shape_b;
        shape_b = temp;
    }
    
    // Simple hash combining all IDs
    uint32_t hash = body_a;
    hash = hash * 31 + body_b;
    hash = hash * 31 + shape_a;
    hash = hash * 31 + shape_b;
    return hash;
}

// Find existing manifold in cache
UpeCachedManifold *UpeManifoldCache_Find(UpeManifoldCache *cache, uint32_t body_a, uint32_t body_b,
                                         uint32_t shape_a, uint32_t shape_b) {
    uint32_t target_id = UpeManifold_GenerateID(body_a, body_b, shape_a, shape_b);
    
    for (uint32_t i = 0; i < cache->manifold_count; ++i) {
        UpeCachedManifold *manifold = &cache->manifolds[i];
        
        if (manifold->is_valid && manifold->manifold_id == target_id) {
            // Verify body/shape IDs match
            int matches = (manifold->body_a == body_a && manifold->body_b == body_b &&
                          manifold->shape_a == shape_a && manifold->shape_b == shape_b) ||
                         (manifold->body_a == body_b && manifold->body_b == body_a &&
                          manifold->shape_a == shape_b && manifold->shape_b == shape_a);
            
            if (matches) {
                cache->cache_hits++;
                return manifold;
            }
        }
    }
    
    cache->cache_misses++;
    return NULL;
}

// Create new manifold or update existing one
UpeCachedManifold *UpeManifoldCache_CreateOrUpdate(UpeManifoldCache *cache, uint32_t body_a, uint32_t body_b,
                                                    uint32_t shape_a, uint32_t shape_b) {
    // Try to find existing manifold first
    UpeCachedManifold *existing = UpeManifoldCache_Find(cache, body_a, body_b, shape_a, shape_b);
    if (existing) {
        // Clear old contact points but preserve impulse data for warm starting
        existing->point_count = 0;
        return existing;
    }
    
    // Need to create new manifold
    if (cache->manifold_count >= cache->manifold_capacity) {
        // Grow cache
        cache->manifold_capacity *= 2;
        cache->manifolds = (UpeCachedManifold *)realloc(cache->manifolds, 
                                  cache->manifold_capacity * sizeof(UpeCachedManifold));
    }
    
    UpeCachedManifold *manifold = &cache->manifolds[cache->manifold_count++];
    
    // Initialize manifold
    manifold->body_a = body_a;
    manifold->body_b = body_b;
    manifold->shape_a = shape_a;
    manifold->shape_b = shape_b;
    manifold->manifold_id = UpeManifold_GenerateID(body_a, body_b, shape_a, shape_b);
    manifold->frame_created = cache->current_frame;
    manifold->frame_last_updated = cache->current_frame;
    manifold->is_valid = 1;
    manifold->point_count = 0;
    manifold->friction = 0.5f;
    manifold->restitution = 0.3f;
    manifold->normal = UpeVec3{0.0f, 1.0f, 0.0f};
    
    // Initialize contact points
    for (int i = 0; i < UPE_MAX_MANIFOLD_POINTS; ++i) {
        manifold->points[i].is_active = 0;
        manifold->points[i].normal_impulse = 0.0f;
        manifold->points[i].tangent_impulse[0] = 0.0f;
        manifold->points[i].tangent_impulse[1] = 0.0f;
        manifold->points[i].feature_id = 0;
    }
    
    return manifold;
}

// Add contact point to manifold with feature tracking
void UpeManifold_AddPoint(UpeCachedManifold *manifold, UpeVec3 world_point_a, UpeVec3 world_point_b,
                          float penetration, uint32_t feature_id) {
    if (manifold->point_count >= UPE_MAX_MANIFOLD_POINTS) {
        return;  // Manifold full
    }
    
    // Check if similar contact point already exists (within 0.01m tolerance)
    float tolerance = 0.01f;
    for (uint32_t i = 0; i < manifold->point_count; ++i) {
        UpeManifoldPoint *existing = &manifold->points[i];
        if (!existing->is_active) continue;
        
        // Check distance between points
        UpeVec3 diff_a = {world_point_a.x - existing->local_point_a.x,
                          world_point_a.y - existing->local_point_a.y,
                          world_point_a.z - existing->local_point_a.z};
        float dist_sq = diff_a.x * diff_a.x + diff_a.y * diff_a.y + diff_a.z * diff_a.z;
        
        if (dist_sq < tolerance * tolerance) {
            // Update existing point
            existing->local_point_a = world_point_a;
            existing->local_point_b = world_point_b;
            existing->penetration = penetration;
            existing->feature_id = feature_id;
            return;
        }
    }
    
    // Add new contact point
    UpeManifoldPoint *point = &manifold->points[manifold->point_count++];
    point->local_point_a = world_point_a;
    point->local_point_b = world_point_b;
    point->penetration = penetration;
    point->feature_id = feature_id;
    point->is_active = 1;
    
    // Preserve warm start data if this is a persistent contact with same feature ID
    // (impulses already initialized to 0 in CreateOrUpdate)
}

// Mark manifold as touched this frame
void UpeManifoldCache_Touch(UpeManifoldCache *cache, UpeCachedManifold *manifold) {
    manifold->frame_last_updated = cache->current_frame;
}

// Remove expired manifolds (haven't been updated in max_lifetime frames)
void UpeManifoldCache_Cleanup(UpeManifoldCache *cache) {
    uint32_t write_index = 0;
    
    for (uint32_t i = 0; i < cache->manifold_count; ++i) {
        UpeCachedManifold *manifold = &cache->manifolds[i];
        
        // Check if manifold has expired
        uint32_t frames_since_update = cache->current_frame - manifold->frame_last_updated;
        
        if (manifold->is_valid && frames_since_update > cache->max_lifetime) {
            manifold->is_valid = 0;
            cache->cache_evictions++;
            // Don't copy this manifold (it's expired)
        } else if (manifold->is_valid) {
            // Keep this manifold
            if (write_index != i) {
                cache->manifolds[write_index] = cache->manifolds[i];
            }
            write_index++;
        }
    }
    
    cache->manifold_count = write_index;
}

// Advance to next frame
void UpeManifoldCache_AdvanceFrame(UpeManifoldCache *cache) {
    cache->current_frame++;
    
    // Periodic cleanup every 60 frames (1 second at 60Hz)
    if (cache->current_frame % 60 == 0) {
        UpeManifoldCache_Cleanup(cache);
    }
}

// Get statistics
void UpeManifoldCache_GetStats(const UpeManifoldCache *cache, uint32_t *cache_hits, uint32_t *cache_misses,
                               uint32_t *active_manifolds) {
    if (cache_hits) *cache_hits = cache->cache_hits;
    if (cache_misses) *cache_misses = cache->cache_misses;
    if (active_manifolds) *active_manifolds = cache->manifold_count;
}

void UpeManifoldCache_ResetStats(UpeManifoldCache *cache) {
    cache->cache_hits = 0;
    cache->cache_misses = 0;
    cache->cache_evictions = 0;
}

// Validate cached contact point
int UpeManifoldPoint_IsValid(const UpeManifoldPoint *point, UpeVec3 world_point_a, UpeVec3 world_point_b,
                              float tolerance) {
    if (!point->is_active) return 0;
    
    // Check if new points are within tolerance of cached points
    UpeVec3 diff_a = {world_point_a.x - point->local_point_a.x,
                      world_point_a.y - point->local_point_a.y,
                      world_point_a.z - point->local_point_a.z};
    
    float dist_sq_a = diff_a.x * diff_a.x + diff_a.y * diff_a.y + diff_a.z * diff_a.z;
    
    if (dist_sq_a > tolerance * tolerance) return 0;
    
    UpeVec3 diff_b = {world_point_b.x - point->local_point_b.x,
                      world_point_b.y - point->local_point_b.y,
                      world_point_b.z - point->local_point_b.z};
    
    float dist_sq_b = diff_b.x * diff_b.x + diff_b.y * diff_b.y + diff_b.z * diff_b.z;
    
    return (dist_sq_b <= tolerance * tolerance);
}
