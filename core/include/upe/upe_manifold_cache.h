#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximum contact points per manifold (typical: 4 for box-box)
#define UPE_MAX_MANIFOLD_POINTS 4

// Contact point with warm starting data
typedef struct UpeManifoldPoint {
    UpeVec3 local_point_a;        // Contact point in body A's local space
    UpeVec3 local_point_b;        // Contact point in body B's local space
    float penetration;            // Penetration depth
    float normal_impulse;         // Accumulated normal impulse (warm start)
    float tangent_impulse[2];     // Accumulated tangent impulses (friction)
    uint32_t feature_id;          // Feature pair ID for tracking
    int is_active;                // 1 if point is valid, 0 if inactive
} UpeManifoldPoint;

// Contact manifold between two bodies
typedef struct UpeCachedManifold {
    uint32_t body_a;              // First body ID
    uint32_t body_b;              // Second body ID
    uint32_t shape_a;             // First shape ID
    uint32_t shape_b;             // Second shape ID
    
    UpeVec3 normal;               // Contact normal (world space, points from A to B)
    UpeManifoldPoint points[UPE_MAX_MANIFOLD_POINTS];
    uint32_t point_count;         // Number of active contact points
    
    float friction;               // Combined friction coefficient
    float restitution;            // Combined restitution (bounce)
    
    uint32_t manifold_id;         // Unique ID for this manifold
    uint32_t frame_created;       // Frame when manifold was created
    uint32_t frame_last_updated;  // Last frame this manifold was active
    
    int is_valid;                 // 1 if manifold is active, 0 if expired
} UpeCachedManifold;

// Manifold cache for persistent contacts
typedef struct UpeManifoldCache {
    UpeCachedManifold *manifolds;
    uint32_t manifold_count;
    uint32_t manifold_capacity;
    
    uint32_t current_frame;       // Current simulation frame
    uint32_t max_lifetime;        // Maximum frames to keep inactive manifolds
    
    // Statistics
    uint32_t cache_hits;          // Manifolds reused from cache
    uint32_t cache_misses;        // New manifolds created
    uint32_t cache_evictions;     // Manifolds removed due to expiration
} UpeManifoldCache;

// Initialize manifold cache
void UpeManifoldCache_Init(UpeManifoldCache *cache, uint32_t initial_capacity);
void UpeManifoldCache_Destroy(UpeManifoldCache *cache);

// Find existing manifold between two bodies
UpeCachedManifold *UpeManifoldCache_Find(UpeManifoldCache *cache, uint32_t body_a, uint32_t body_b, 
                                         uint32_t shape_a, uint32_t shape_b);

// Create or update manifold
UpeCachedManifold *UpeManifoldCache_CreateOrUpdate(UpeManifoldCache *cache, uint32_t body_a, uint32_t body_b,
                                                    uint32_t shape_a, uint32_t shape_b);

// Add contact point to manifold
void UpeManifold_AddPoint(UpeCachedManifold *manifold, UpeVec3 world_point_a, UpeVec3 world_point_b,
                          float penetration, uint32_t feature_id);

// Update manifold in cache (mark as active this frame)
void UpeManifoldCache_Touch(UpeManifoldCache *cache, UpeCachedManifold *manifold);

// Remove expired manifolds
void UpeManifoldCache_Cleanup(UpeManifoldCache *cache);

// Advance to next frame
void UpeManifoldCache_AdvanceFrame(UpeManifoldCache *cache);

// Get manifold statistics
void UpeManifoldCache_GetStats(const UpeManifoldCache *cache, uint32_t *cache_hits, uint32_t *cache_misses,
                               uint32_t *active_manifolds);

// Reset statistics
void UpeManifoldCache_ResetStats(UpeManifoldCache *cache);

// Validate cached contact point (check if still close to actual contact)
int UpeManifoldPoint_IsValid(const UpeManifoldPoint *point, UpeVec3 world_point_a, UpeVec3 world_point_b,
                              float tolerance);

// Generate manifold ID from body/shape IDs
uint32_t UpeManifold_GenerateID(uint32_t body_a, uint32_t body_b, uint32_t shape_a, uint32_t shape_b);

#ifdef __cplusplus
}
#endif
