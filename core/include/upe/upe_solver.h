#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeBodyState {
    UpeVec3 position;
    UpeVec3 velocity;
    UpeQuat rotation;
    UpeVec3 angular_velocity;
    float inv_mass;
    UpeVec3 inv_inertia;
} UpeBodyState;

typedef struct UpeSolverConstraint {
    uint32_t body_a;
    uint32_t body_b;
    UpeVec3 normal;
    UpeVec3 contact_point;  // World-space contact position
    float penetration;
    float lambda;           // Accumulated normal impulse
    float lambda_t1;        // Accumulated friction impulse (tangent 1)
    float lambda_t2;        // Accumulated friction impulse (tangent 2)
    float friction;         // Combined friction coefficient (for simple mode)
    float friction_static;  // Static friction coefficient (sticking)
    float friction_dynamic; // Dynamic friction coefficient (sliding)
    float restitution;      // Restitution coefficient (bounciness)
    int is_sliding;         // Friction state: 0 = sticking, 1 = sliding
    UpeVec3 r_a;           // Contact point relative to body A center
    UpeVec3 r_b;           // Contact point relative to body B center
    UpeVec3 tangent1;      // Friction direction 1 (perpendicular to normal)
    UpeVec3 tangent2;      // Friction direction 2 (perpendicular to normal and tangent1)
    float cfm;              // Constraint force mixing (compliance) for soft constraints
    float bias;             // Baumgarte bias for penetration correction
    float friction_anisotropy;  // Anisotropic friction ratio (>1 = slippery along tangent2)
} UpeSolverConstraint;

void UpeSolver_PgsIteration(UpeBodyState *bodies, uint32_t body_count,
                            UpeSolverConstraint *constraints, uint32_t constraint_count);

#ifdef __cplusplus
}
#endif
