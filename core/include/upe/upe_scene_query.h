#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Scene Queries (9.0.0.0)
  
  Raycast / sweep / overlap across all geometry types and domains
  (rigid + SDF + terrain + triggers).
  
  Include: query filters, backface rules, triggers-only, continuous sweeps.
*/

// Query types
typedef enum UpeQueryType {
    UPE_QUERY_RAYCAST = 0,       // Ray intersection
    UPE_QUERY_SWEEP = 1,         // Shape sweep (convex cast)
    UPE_QUERY_OVERLAP = 2,       // Static overlap test
    UPE_QUERY_CLOSEST_POINT = 3  // Find closest point on geometry
} UpeQueryType;

// Query flags
#define UPE_QUERY_FLAG_BACKFACE_CULL  (1 << 0)  // Ignore backfaces
#define UPE_QUERY_FLAG_TRIGGERS_ONLY  (1 << 1)  // Only test triggers
#define UPE_QUERY_FLAG_SOLIDS_ONLY    (1 << 2)  // Only test solids
#define UPE_QUERY_FLAG_CONTINUOUS     (1 << 3)  // CCD-style (first hit only)
#define UPE_QUERY_FLAG_ANY_HIT        (1 << 4)  // Stop at first hit
#define UPE_QUERY_FLAG_SORT_BY_DISTANCE (1 << 5) // Return hits sorted
#define UPE_QUERY_FLAG_ALLOW_SLEEPING (1 << 6)  // Test sleeping bodies too

// Hit result
typedef struct UpeQueryHit {
    uint32_t body_id;            // Which body/shape was hit
    uint32_t shape_id;
    
    UpeVec3 position;            // World-space hit position
    UpeVec3 normal;              // Surface normal at hit
    float distance;              // Distance along ray/sweep
    float penetration;           // Penetration depth (for overlap)
    
    // Material at hit point
    uint32_t material_id;
    
    // Feature ID (for persistent contacts)
    uint32_t feature_id;
    
    // Trigger flag
    int is_trigger;
    
} UpeQueryHit;

// Query filter (callback for custom filtering)
typedef int (*UpeQueryFilterFunc)(uint32_t body_id, uint32_t shape_id, void *user_data);

// Raycast parameters
typedef struct UpeRaycastParams {
    UpeVec3 origin;
    UpeVec3 direction;           // Should be normalized
    float max_distance;
    
    uint32_t flags;              // Bitfield of UPE_QUERY_FLAG_*
    
    // Layer filtering (bitfield: query_layers & body_layers)
    uint32_t layer_mask;
    
    // Custom filter callback
    UpeQueryFilterFunc filter;
    void *filter_user_data;
    
    // Output
    UpeQueryHit *hits;
    uint32_t max_hits;
    uint32_t hit_count;          // Filled by query
    
} UpeRaycastParams;

// Sweep parameters (convex cast)
typedef struct UpeSweepParams {
    // Shape to sweep
    uint32_t shape_type;         // Box, sphere, capsule, convex
    UpeVec3 shape_center;
    UpeQuat shape_rotation;
    float shape_params[4];       // Dimensions (radius, half-extents, etc.)
    
    // Sweep direction
    UpeVec3 direction;           // Should be normalized
    float max_distance;
    
    uint32_t flags;
    uint32_t layer_mask;
    
    UpeQueryFilterFunc filter;
    void *filter_user_data;
    
    // Output
    UpeQueryHit *hits;
    uint32_t max_hits;
    uint32_t hit_count;
    
} UpeSweepParams;

// Overlap parameters
typedef struct UpeOverlapParams {
    // Test shape
    uint32_t shape_type;
    UpeVec3 shape_center;
    UpeQuat shape_rotation;
    float shape_params[4];
    
    uint32_t flags;
    uint32_t layer_mask;
    
    UpeQueryFilterFunc filter;
    void *filter_user_data;
    
    // Output
    UpeQueryHit *overlaps;
    uint32_t max_overlaps;
    uint32_t overlap_count;
    
} UpeOverlapParams;

// Closest point parameters
typedef struct UpeClosestPointParams {
    UpeVec3 query_point;
    float max_distance;          // Search radius
    
    uint32_t flags;
    uint32_t layer_mask;
    
    UpeQueryFilterFunc filter;
    void *filter_user_data;
    
    // Output
    UpeQueryHit closest_hit;
    int found;
    
} UpeClosestPointParams;

// Scene query system
typedef struct UpeSceneQuerySystem {
    void *world;                 // Pointer to UpeWorld (opaque here)
    
    // Acceleration structure (shares broadphase or separate)
    void *query_accelerator;
    
    // Spatial hash for terrain/heightfields
    uint32_t *terrain_hash;
    
    // Statistics
    uint32_t raycast_count;
    uint32_t sweep_count;
    uint32_t overlap_count;
    
} UpeSceneQuerySystem;

// Initialization
void UpeQuery_Init(UpeSceneQuerySystem *query_sys, void *world);
void UpeQuery_Destroy(UpeSceneQuerySystem *query_sys);

// Main query functions
int UpeQuery_Raycast(UpeSceneQuerySystem *sys, UpeRaycastParams *params);
int UpeQuery_Sweep(UpeSceneQuerySystem *sys, UpeSweepParams *params);
int UpeQuery_Overlap(UpeSceneQuerySystem *sys, UpeOverlapParams *params);
int UpeQuery_ClosestPoint(UpeSceneQuerySystem *sys, UpeClosestPointParams *params);

// Convenience wrappers
int UpeQuery_RaycastSingle(UpeSceneQuerySystem *sys, UpeVec3 origin, UpeVec3 direction,
                           float max_distance, UpeQueryHit *hit);
int UpeQuery_SphereCast(UpeSceneQuerySystem *sys, UpeVec3 center, float radius,
                        UpeVec3 direction, float max_distance, UpeQueryHit *hit);
int UpeQuery_BoxCast(UpeSceneQuerySystem *sys, UpeVec3 center, UpeVec3 half_extents,
                     UpeQuat rotation, UpeVec3 direction, float max_distance, UpeQueryHit *hit);

int UpeQuery_SphereOverlap(UpeSceneQuerySystem *sys, UpeVec3 center, float radius,
                           UpeQueryHit *overlaps, uint32_t max_overlaps);
int UpeQuery_BoxOverlap(UpeSceneQuerySystem *sys, UpeVec3 center, UpeVec3 half_extents,
                        UpeQuat rotation, UpeQueryHit *overlaps, uint32_t max_overlaps);

// Testing against specific geometry types
int UpeQuery_RayVsBox(UpeVec3 ray_origin, UpeVec3 ray_dir, UpeVec3 box_center,
                      UpeVec3 box_half_extents, UpeQuat box_rot, UpeQueryHit *hit);
int UpeQuery_RayVsSphere(UpeVec3 ray_origin, UpeVec3 ray_dir, UpeVec3 sphere_center,
                         float sphere_radius, UpeQueryHit *hit);
int UpeQuery_RayVsCapsule(UpeVec3 ray_origin, UpeVec3 ray_dir, UpeVec3 capsule_a,
                          UpeVec3 capsule_b, float capsule_radius, UpeQueryHit *hit);
int UpeQuery_RayVsTriangle(UpeVec3 ray_origin, UpeVec3 ray_dir, UpeVec3 v0, UpeVec3 v1,
                           UpeVec3 v2, UpeQueryHit *hit);

// Deterministic ordering
void UpeQuery_SortHitsByDistance(UpeQueryHit *hits, uint32_t count);

// Statistics
void UpeQuery_ResetStats(UpeSceneQuerySystem *sys);
void UpeQuery_PrintStats(const UpeSceneQuerySystem *sys);

#ifdef __cplusplus
}
#endif
