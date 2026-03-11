#ifndef UPE_CCD_SCALAR_H
#define UPE_CCD_SCALAR_H

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Continuous Collision Detection (CCD) for fast-moving objects
 *
 * Prevents tunneling by computing the time of impact (TOI) between two objects
 * moving over a time step. This is especially important for:
 * - Projectiles and fast-moving bodies
 * - Thin geometry that shouldn't be missed
 * - Deterministic behavior (no missed collisions)
 *
 * The CCD system sweeps one shape through space towards another, computing the
 * earliest time at which they collide [0, 1] where 1 is the end of the time step.
 */

typedef struct {
    float time_of_impact;   /* [0, 1] when collision occurs relative to dt */
    UpeVec3 contact_point;  /* World contact point at TOI */
    float separation;       /* Separation distance at TOI (debug info) */
} UpeCCDResult;

/**
 * Compute time of impact between a moving sphere and static sphere
 *
 * Solves the quadratic equation for when sphere_a (moving from p0_a to p1_a)
 * first touches sphere_b (at rest at p_b).
 *
 * Returns 1 if collision happens within [0, 1], 0 otherwise.
 */
int UpeCCD_SphereSphere(UpeVec3 p0_a, UpeVec3 p1_a, float radius_a,
                        UpeVec3 p_b, float radius_b,
                        UpeCCDResult *out_result);

/**
 * Compute time of impact between moving sphere and static plane
 *
 * Returns 1 if sphere sweeps toward or into the plane during time step.
 */
int UpeCCD_SpherePlane(UpeVec3 p0, UpeVec3 p1, float radius,
                       UpeVec3 plane_point, UpeVec3 plane_normal,
                       UpeCCDResult *out_result);

/**
 * Compute time of impact between moving sphere and static capsule
 *
 * Sweeps sphere from p0 to p1 and finds when it first touches capsule.
 */
int UpeCCD_SphereCapsule(UpeVec3 p0, UpeVec3 p1, float radius,
                         UpeVec3 cap_a, UpeVec3 cap_b, float cap_radius,
                         UpeCCDResult *out_result);

/**
 * Compute time of impact between moving capsule and static capsule
 *
 * More complex: reduces to distance between line segments.
 */
int UpeCCD_CapsuleCapsule(UpeVec3 p0_a, UpeVec3 p1_a, UpeVec3 p0_b, UpeVec3 p1_b,
                          float radius_a, float radius_b,
                          UpeCCDResult *out_result);

/**
 * Compute time of impact between moving convex hull and static convex hull
 *
 * Uses GJK-based distance queries iteratively to find collision time.
 */
int UpeCCD_ConvexConvex(const float *vertices_a, uint32_t vertex_count_a,
                        const float *vertices_b, uint32_t vertex_count_b,
                        UpeVec3 velocity_a,
                        UpeCCDResult *out_result);

/**
 * Clamp TOI result to [0, 1] range and provide safe separation distance
 */
float UpeCCD_SafeTOI(float raw_toi);

/**
 * Statistics for CCD operations
 */
typedef struct {
    uint32_t queries_performed;
    uint32_t collisions_detected;
    uint32_t tunneling_prevented;
} UpeCCDStats;

/**
 * Get CCD statistics
 */
void UpeCCD_GetStats(UpeCCDStats *stats);

/**
 * Reset CCD statistics
 */
void UpeCCD_ResetStats(void);

#ifdef __cplusplus
}
#endif

#endif /* UPE_CCD_SCALAR_H */
