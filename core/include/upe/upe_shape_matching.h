#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Shape Matching for Deformable Bodies
  
  Uses polar decomposition to recover rotation from a deformed particle cluster,
  then applies restoring forces to drive particles back toward the rest shape.
  
  References: Mueller et al. "Meshless Deformations Based on Shape Matching" (2005).
  
  Suitable for:
  - Soft bodies needing stable deformation without FEM overhead
  - Character muscle volumes
  - Jelly/foam-like materials
  - Rigid body with compliant deformation
*/

#define UPE_SHAPE_MATCHING_MAX_PARTICLES 4096

typedef struct UpeShapeMatchingBody {
    /* Rest-state positions (local frame) */
    UpeVec3 *rest_positions;      /* original shape, n_particles entries */
    float   *masses;              /* per-particle mass */
    UpeVec3  rest_com;            /* center of mass in rest state */

    /* Current simulation state */
    UpeVec3 *positions;           /* current world positions */
    UpeVec3 *velocities;
    uint32_t n_particles;

    /* Derived match state (computed each step) */
    UpeVec3  current_com;         /* current center of mass */
    UpeMat3  rotation;            /* recovered rotation R */
    UpeVec3  translation;         /* com translation */

    /* Stiffness (0=no restoring force, 1=rigid) */
    float stiffness;              /* alpha in [0,1] */
    float damping;
} UpeShapeMatchingBody;

/* Lifecycle */
void UpeShapeMatch_Init(UpeShapeMatchingBody *body, uint32_t n_particles,
                        float stiffness, float damping);
void UpeShapeMatch_Destroy(UpeShapeMatchingBody *body);

/* Set rest pose from current positions */
void UpeShapeMatch_SetRestPose(UpeShapeMatchingBody *body);

/* Step: compute best-fit rotation, apply restoring velocities */
void UpeShapeMatch_Step(UpeShapeMatchingBody *body, float dt);

/* Polar decomposition of a 3x3 matrix into R (rotation) and S (symmetric).
   M = R * S.  Returns 0 on success. */
int UpeShapeMatch_PolarDecompose(const UpeMat3 *M, UpeMat3 *R, UpeMat3 *S);

/* Utility: compute center of mass from positions and masses */
UpeVec3 UpeShapeMatch_CenterOfMass(const UpeVec3 *positions,
                                    const float   *masses,
                                    uint32_t       n_particles);

/* Utility: compute Apq covariance matrix used in shape matching */
UpeMat3 UpeShapeMatch_ComputeApq(const UpeShapeMatchingBody *body);

#ifdef __cplusplus
}
#endif
