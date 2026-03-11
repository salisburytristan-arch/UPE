#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_transform.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Convex Hull Collision Detection
  
  - GJK (Gilbert-Johnson-Keerthi) for distance/separation
  - EPA (Expanding Polytope Algorithm) for penetration depth
  - Support function interface for arbitrary convex shapes
*/

#define UPE_CONVEX_MAX_VERTICES 256
#define UPE_EPA_MAX_ITERATIONS 32
#define UPE_GJK_MAX_ITERATIONS 64

typedef struct UpeConvexHull {
    UpeVec3 *vertices;
    uint32_t vertex_count;
    UpeVec3 centroid;
    float bounding_radius;
} UpeConvexHull;

// Support function: returns point on surface in given direction
typedef UpeVec3 (*UpeSupportFunction)(const void *shape, UpeVec3 direction);

// GJK result
typedef struct UpeGjkResult {
    int intersecting;        // 1 if intersecting, 0 if separated
    float distance;          // Minimum distance (if separated)
    UpeVec3 closest_a;       // Closest point on A
    UpeVec3 closest_b;       // Closest point on B
    UpeVec3 normal;          // Separation direction (A to B)
    uint32_t iterations;
} UpeGjkResult;

// EPA result
typedef struct UpeEpaResult {
    UpeVec3 normal;          // Penetration normal (A to B)
    float depth;             // Penetration depth
    UpeVec3 contact_point;   // Contact point in world space
    uint32_t iterations;
} UpeEpaResult;

// Convex hull creation
UpeConvexHull* UpeConvexHull_Create(uint32_t max_vertices);
void UpeConvexHull_Destroy(UpeConvexHull *hull);
void UpeConvexHull_AddVertex(UpeConvexHull *hull, UpeVec3 vertex);
void UpeConvexHull_ComputeProperties(UpeConvexHull *hull);

// Support functions for primitives
UpeVec3 UpeSupport_Sphere(const void *shape, UpeVec3 direction);
UpeVec3 UpeSupport_Box(const void *shape, UpeVec3 direction);
UpeVec3 UpeSupport_Capsule(const void *shape, UpeVec3 direction);
UpeVec3 UpeSupport_ConvexHull(const void *shape, UpeVec3 direction);

// GJK algorithm
UpeGjkResult UpeGjk_Test(
    const void *shape_a, UpeSupportFunction support_a, UpeTransform3 xform_a,
    const void *shape_b, UpeSupportFunction support_b, UpeTransform3 xform_b);

// EPA algorithm (call after GJK returns intersection)
UpeEpaResult UpeEpa_ComputePenetration(
    const void *shape_a, UpeSupportFunction support_a, UpeTransform3 xform_a,
    const void *shape_b, UpeSupportFunction support_b, UpeTransform3 xform_b,
    UpeVec3 *gjk_simplex, uint32_t simplex_count);

// High-level collision test
typedef struct UpeConvexCollisionResult {
    int colliding;
    UpeVec3 normal;          // Contact normal (A to B)
    float depth;             // Penetration depth (or distance if separated)
    UpeVec3 contact_point;
} UpeConvexCollisionResult;

UpeConvexCollisionResult UpeConvex_TestCollision(
    const void *shape_a, UpeSupportFunction support_a, UpeTransform3 xform_a,
    const void *shape_b, UpeSupportFunction support_b, UpeTransform3 xform_b);

// QuickHull convex hull builder
void UpeQuickHull_Build(const UpeVec3 *points, uint32_t count, UpeConvexHull *out_hull);

// Compute mass properties (mass, center of mass, inertia tensor diagonal)
void UpeConvexHull_ComputeMassProperties(const UpeConvexHull *hull, float density, 
                                         float *out_mass, UpeVec3 *out_center_of_mass, 
                                         UpeVec3 *out_inertia);

#ifdef __cplusplus
}
#endif
