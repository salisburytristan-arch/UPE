#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Position-Based Dynamics (PBD/XPBD)
  
  Unified constraint-based dynamics for:
  - Cloth simulation
  - Soft bodies
  - Ropes/cables
  - Ragdoll constraints
  - Fluid constraints (position-based fluids)
  
  Based on Müller et al. "Position Based Dynamics" (2007)
  Extended with XPBD compliance for stiffness control (Macklin et al. 2016)
*/

#define UPE_PBD_MAX_PARTICLES 32768
#define UPE_PBD_MAX_CONSTRAINTS 131072

typedef struct UpePbdParticle {
    UpeVec3 position;        // Current position (predicted)
    UpeVec3 previous_pos;    // Previous position (for velocity)
    UpeVec3 velocity;        // Velocity (updated after solve)
    float inv_mass;          // 1/mass (0 = infinite mass/pinned)
    float radius;            // Collision radius
    uint32_t flags;          // Pinned, colliding, etc.
} UpePbdParticle;

// Constraint types
typedef enum UpePbdConstraintType {
    UPE_PBD_DISTANCE = 0,      // Two particles at fixed distance
    UPE_PBD_BENDING,           // Three particles for bending resistance
    UPE_PBD_VOLUME,            // Four particles for volume preservation
    UPE_PBD_COLLISION,         // Particle-environment collision
    UPE_PBD_ATTACHMENT,        // Pin to world position
    UPE_PBD_SHAPE_MATCHING,    // Cluster shape preservation
    UPE_PBD_STRETCH_SHEAR,     // Anisotropic stretch/shear (cloth)
    UPE_PBD_LONG_RANGE          // Long-range attachment (faster convergence)
} UpePbdConstraintType;

// Distance constraint (most common)
typedef struct UpePbdDistanceConstraint {
    uint32_t particle_a;
    uint32_t particle_b;
    float rest_length;
    float compliance;        // 0 = rigid, >0 = soft (XPBD)
    float lambda;            // Lagrange multiplier (warmstart)
} UpePbdDistanceConstraint;

// Bending constraint (angle between two edges)
typedef struct UpePbdBendingConstraint {
    uint32_t p0, p1, p2, p3; // Four particles forming two triangles
    float rest_angle;        // Dihedral angle at rest
    float compliance;
    float lambda;
} UpePbdBendingConstraint;

// Volume constraint (tetrahedral volume)
typedef struct UpePbdVolumeConstraint {
    uint32_t p0, p1, p2, p3;
    float rest_volume;
    float compliance;
    float lambda;
} UpePbdVolumeConstraint;

// Collision constraint (particle vs environment)
typedef struct UpePbdCollisionConstraint {
    uint32_t particle;
    UpeVec3 point;           // Collision point
    UpeVec3 normal;          // Collision normal
    float penetration;       // Penetration depth
    float friction;          // Coulomb friction coefficient
} UpePbdCollisionConstraint;

// Shape matching constraint (rigid clusters)
typedef struct UpePbdShapeMatchConstraint {
    uint32_t *particles;     // Array of particle indices
    uint32_t count;
    UpeVec3 *rest_positions; // Original shape
    UpeVec3 com_rest;        // Center of mass at rest
    float compliance;
    float lambda;
} UpePbdShapeMatchConstraint;

// PBD System
typedef struct UpePbdSystem {
    UpePbdParticle *particles;
    uint32_t particle_count;
    uint32_t particle_capacity;
    
    // Constraint arrays
    UpePbdDistanceConstraint *distance_constraints;
    uint32_t distance_count;
    uint32_t distance_capacity;
    
    UpePbdBendingConstraint *bending_constraints;
    uint32_t bending_count;
    uint32_t bending_capacity;
    
    UpePbdVolumeConstraint *volume_constraints;
    uint32_t volume_count;
    uint32_t volume_capacity;
    
    UpePbdCollisionConstraint *collision_constraints;
    uint32_t collision_count;
    uint32_t collision_capacity;
    
    // Solver parameters
    UpeVec3 gravity;
    uint32_t solver_iterations;    // Constraint iterations per step
    float damping;                 // Global velocity damping
    float sleep_threshold;         // Velocity below which particles sleep
    
    // Collision
    UpeVec3 bounds_min;
    UpeVec3 bounds_max;
    float ground_friction;
    float restitution;
} UpePbdSystem;

// System management
void UpePbd_Init(UpePbdSystem *pbd, uint32_t max_particles, uint32_t max_constraints);
void UpePbd_Destroy(UpePbdSystem *pbd);
void UpePbd_Clear(UpePbdSystem *pbd);

// Particle management
uint32_t UpePbd_AddParticle(UpePbdSystem *pbd, UpeVec3 position, float mass, float radius);
void UpePbd_RemoveParticle(UpePbdSystem *pbd, uint32_t index);
void UpePbd_PinParticle(UpePbdSystem *pbd, uint32_t index, int pinned);

// Constraint creation
uint32_t UpePbd_AddDistanceConstraint(UpePbdSystem *pbd, uint32_t a, uint32_t b, 
                                       float rest_length, float compliance);
uint32_t UpePbd_AddBendingConstraint(UpePbdSystem *pbd, uint32_t p0, uint32_t p1, 
                                      uint32_t p2, uint32_t p3, float compliance);
uint32_t UpePbd_AddVolumeConstraint(UpePbdSystem *pbd, uint32_t p0, uint32_t p1,
                                     uint32_t p2, uint32_t p3, float compliance);

// Simulation
void UpePbd_Step(UpePbdSystem *pbd, float dt);

// Utility: Generate constraints for cloth grid
void UpePbd_CreateClothGrid(UpePbdSystem *pbd, 
                            uint32_t width, uint32_t height,
                            float spacing, float mass_per_particle,
                            float stretch_compliance, float bend_compliance);

// Utility: Generate constraints for rope
void UpePbd_CreateRope(UpePbdSystem *pbd,
                      UpeVec3 start, UpeVec3 end,
                      uint32_t segments, float mass_per_segment,
                      float compliance);

#ifdef __cplusplus
}
#endif
