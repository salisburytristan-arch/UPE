#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Persistent Contact Manifolds (11.0.0.0 + 7.0.0)
  
  Full manifold caching with feature IDs (not just "nearby point" matching).
  Stable stacking + reduced jitter + better friction.
  
  Improvements:
  - Feature-based matching (not distance threshold)
  - Multiple contact points (true manifolds)
  - Warm-starting with proper impulse application
  - Static vs dynamic friction split
*/

// Contact feature types (for persistent matching)
typedef enum UpeContactFeature {
    UPE_FEATURE_VERTEX = 0,      // Point
    UPE_FEATURE_EDGE = 1,        // Line segment
    UPE_FEATURE_FACE = 2         // Planar region
} UpeContactFeature;

// Feature ID (for matching contacts across frames)
typedef struct UpeFeatureID {
    UpeContactFeature type;
    uint32_t indices[4];         // Vertex/edge/face indices
    uint32_t shape_id;
} UpeFeatureID;

// Contact point (single point in manifold)
typedef struct UpeContactPoint {
    UpeVec3 local_pos_a;         // In body A's local space
    UpeVec3 local_pos_b;         // In body B's local space
    
    UpeVec3 world_pos;           // World-space position
    float penetration_depth;
    
    // Feature IDs (for persistence)
    UpeFeatureID feature_a;
    UpeFeatureID feature_b;
    
    // Accumulated impulses (for warm-starting)
    float normal_impulse;
    float tangent_impulse_u;
    float tangent_impulse_v;
    
    // Contact properties
    float friction;              // Combined friction
    float restitution;           // Combined restitution
    float bias_velocity;         // For position correction
    
    // Lifecycle
    uint32_t lifetime;           // Frames this contact has existed
    int persisted;               // Matched from previous frame
    
} UpeContactPoint;

// Contact manifold (collection of contact points between two bodies)
typedef struct UpeContactManifold {
    uint32_t body_a;
    uint32_t body_b;
    
    UpeVec3 normal;              // Contact normal (from A to B)
    
    // Contact points (up to 4 for box-box, 1 for sphere)
    UpeContactPoint points[4];
    uint32_t point_count;
    
    // Material properties
    uint32_t material_a;
    uint32_t material_b;
    
    // Friction state
    int in_static_friction;      // Currently sticking (not sliding)
    UpeVec3 previous_relative_vel;
    
    // Manifold ID (deterministic hash of body IDs)
    uint64_t manifold_id;
    
    // Lifecycle
    uint32_t lifetime;
    int active;
    
} UpeContactManifold;

// Manifold cache (persistent across frames)
typedef struct UpeManifoldCache {
    UpeContactManifold *manifolds;
    uint32_t manifold_count;
    uint32_t manifold_capacity;
    
    // Hash table for O(1) lookup by body pair
    uint64_t *manifold_map;      // Key = hash(body_a, body_b)
    uint32_t map_size;
    
    // Warm-start impulse persistence
    float warm_start_factor;     // 0..1 (how much to keep)
    
} UpeManifoldCache;

// Initialization
void UpeManifold_InitCache(UpeManifoldCache *cache, uint32_t initial_capacity);
void UpeManifold_DestroyCache(UpeManifoldCache *cache);

// Manifold management
UpeContactManifold* UpeManifold_Find(UpeManifoldCache *cache, uint32_t body_a, uint32_t body_b);
UpeContactManifold* UpeManifold_GetOrCreate(UpeManifoldCache *cache, uint32_t body_a, uint32_t body_b);
void UpeManifold_Remove(UpeManifoldCache *cache, uint64_t manifold_id);

// Contact point management
void UpeManifold_AddPoint(UpeContactManifold *manifold, const UpeContactPoint *point);
void UpeManifold_ReducePoints(UpeContactManifold *manifold);  // Keep best 4 points

// Contact matching (feature-based persistence)
int UpeManifold_MatchPoint(const UpeContactPoint *old_point, const UpeContactPoint *new_point);
void UpeManifold_UpdatePersistence(UpeContactManifold *manifold, const UpeContactPoint *new_points,
                                   uint32_t new_count);

// Warm-starting
void UpeManifold_WarmStart(UpeContactManifold *manifold, void *body_a, void *body_b);

// Static/dynamic friction
void UpeManifold_UpdateFrictionState(UpeContactManifold *manifold, const void *body_a, const void *body_b);
float UpeManifold_GetFriction(const UpeContactManifold *manifold);  // Returns static or kinetic

// Contact generation (creates manifolds from narrowphase)
void UpeManifold_GenerateSphereVsSphere(UpeContactManifold *manifold, const void *sphere_a,
                                        const void *sphere_b);
void UpeManifold_GenerateBoxVsBox(UpeContactManifold *manifold, const void *box_a, const void *box_b);
void UpeManifold_GenerateCapsuleVsCapsule(UpeContactManifold *manifold, const void *cap_a,
                                           const void *cap_b);

// Manifold quality
float UpeManifold_ComputeArea(const UpeContactManifold *manifold);
float UpeManifold_ComputeDepth(const UpeContactManifold *manifold);

// Lifecycle management
void UpeManifold_AgeAll(UpeManifoldCache *cache);  // Increment lifetimes
void UpeManifold_RemoveStale(UpeManifoldCache *cache, uint32_t max_age);

// Deterministic ordering
void UpeManifold_Sort(UpeManifoldCache *cache);  // Sort by manifold_id
uint64_t UpeManifold_Hash(uint32_t body_a, uint32_t body_b);

// Feature ID generation
UpeFeatureID UpeFeature_Vertex(uint32_t shape_id, uint32_t vertex_index);
UpeFeatureID UpeFeature_Edge(uint32_t shape_id, uint32_t vertex_a, uint32_t vertex_b);
UpeFeatureID UpeFeature_Face(uint32_t shape_id, uint32_t face_index);

int UpeFeature_Match(const UpeFeatureID *a, const UpeFeatureID *b);

// Debug visualization
void UpeManifold_DebugDraw(const UpeContactManifold *manifold, void *debug_draw);

// Statistics
void UpeManifold_PrintStats(const UpeManifoldCache *cache);

#ifdef __cplusplus
}
#endif
