#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeRaycastResult {
    int hit;
    float t;
    UpeVec3 position;
    UpeVec3 normal;
    uint32_t body_id;
} UpeRaycastResult;

typedef struct UpeSweepResult {
    int hit;
    float toi;
    UpeVec3 position;
    UpeVec3 normal;
    uint32_t body_id;
} UpeSweepResult;

typedef struct UpeOverlapResult {
    uint32_t body_id;
} UpeOverlapResult;

typedef struct UpeQueryFilter {
    uint32_t layer_mask;
    uint32_t ignore_mask;
    int include_triggers;
} UpeQueryFilter;

UpeQueryFilter UpeQueryFilter_Default();

// Collision filtering helper: returns 1 if bodies should collide based on layers/masks
// body1_layer: collision layer of first body/shape
// body1_mask: collision mask of first body/shape
// body2_layer: collision layer of second body/shape  
// body2_mask: collision mask of second body/shape
int UpeCollisionFilter_ShouldCollide(uint32_t body1_layer, uint32_t body1_mask, 
                                    uint32_t body2_layer, uint32_t body2_mask);

// Forward declaration
typedef struct UpeWorld UpeWorld;

// Raycast queries
int UpeWorld_Raycast(const UpeWorld *world, UpeVec3 origin, UpeVec3 direction, float max_distance,
                     const UpeQueryFilter *filter, UpeRaycastResult *result);
int UpeWorld_RaycastAll(const UpeWorld *world, UpeVec3 origin, UpeVec3 direction, float max_distance,
                        const UpeQueryFilter *filter, UpeRaycastResult *results, uint32_t max_results);

// Batch raycast query: process multiple rays efficiently
// Returns number of rays that hit something (results[i].hit == 1)
uint32_t UpeWorld_RaycastBatch(const UpeWorld *world, 
                               const UpeVec3 *origins, 
                               const UpeVec3 *directions, 
                               const float *max_distances,
                               uint32_t ray_count,
                               const UpeQueryFilter *filter, 
                               UpeRaycastResult *results);

// Overlap queries (AABB-based spatial tests)
uint32_t UpeWorld_OverlapSphere(UpeWorld *world, UpeSphere sphere, UpeOverlapResult *results, uint32_t max_results);
uint32_t UpeWorld_OverlapBox(UpeWorld *world, UpeAabb3 box, UpeOverlapResult *results, uint32_t max_results);
uint32_t UpeWorld_OverlapCapsule(UpeWorld *world, UpeCapsule capsule, UpeOverlapResult *results, uint32_t max_results);

// Sweep queries (CCD-style swept collision tests)
// Returns time-of-impact (toi) in [0, 1] where 0 = immediate contact, 1 = at max_distance
int UpeWorld_SweepSphere(UpeWorld *world, UpeSphere sphere, UpeVec3 velocity, float max_distance,
                        const UpeQueryFilter *filter, UpeSweepResult *result);
int UpeWorld_SweepBox(UpeWorld *world, UpeAabb3 box, UpeVec3 velocity, float max_distance,
                     const UpeQueryFilter *filter, UpeSweepResult *result);
int UpeWorld_SweepCapsule(UpeWorld *world, UpeCapsule capsule, UpeVec3 velocity, float max_distance,
                         const UpeQueryFilter *filter, UpeSweepResult *result);

#ifdef __cplusplus
}
#endif
