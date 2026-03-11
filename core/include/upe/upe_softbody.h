#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

// Soft body: collection of particles with distance constraints
// Useful for cloth, ropes, soft objects
typedef struct UpeSoftBodyVertex {
    UpeVec3 position;           // Current world position
    UpeVec3 previous_position;  // Previous frame position (for Verlet integration)
    UpeVec3 velocity;           // Computed from position-previous_position
    float mass;                 // Particle mass
    float inv_mass;             // Inverse mass (0 for pinned vertices)
    int pinned;                 // Pin to current position (constrain to world)
} UpeSoftBodyVertex;

typedef struct UpeSoftBodyConstraint {
    uint32_t vertex_a;          // First vertex index
    uint32_t vertex_b;          // Second vertex index
    float rest_length;          // Desired distance between vertices
    float compliance;           // Compliance for stretching (0 = rigid, >0 = stretchy)
    int is_bend;                // Bend constraint (higher compliance) vs. structural
} UpeSoftBodyConstraint;

typedef struct UpeSoftBody {
    uint32_t id;
    
    UpeSoftBodyVertex *vertices;
    uint32_t vertex_count;
    
    UpeSoftBodyConstraint *constraints;
    uint32_t constraint_count;
    
    // Simulation parameters
    UpeVec3 gravity;            // Local gravity override
    float air_damping;          // Velocity damping from air resistance
    float wind_force;           // Magnitude of constant wind force applied
    UpeVec3 wind_direction;     // Direction of wind
} UpeSoftBody;

typedef struct UpeSoftBodyDesc {
    UpeSoftBodyVertex *vertices;
    uint32_t vertex_count;
    
    UpeSoftBodyConstraint *constraints;
    uint32_t constraint_count;
    
    // Optional parameters
    UpeVec3 gravity;
    float air_damping;
    float wind_force;
    UpeVec3 wind_direction;
} UpeSoftBodyDesc;

// Create soft body
UpeSoftBody *UpeSoftBody_Create(const UpeSoftBodyDesc *desc);

// Destroy soft body
void UpeSoftBody_Destroy(UpeSoftBody *body);

// Step soft body simulation (typically run multiple times per physics frame)
void UpeSoftBody_Step(UpeSoftBody *body, float dt, uint32_t solver_iterations);

// Set vertex position and pin it
void UpeSoftBody_PinVertex(UpeSoftBody *body, uint32_t vertex_idx, UpeVec3 position);

// Unpin vertex
void UpeSoftBody_UnpinVertex(UpeSoftBody *body, uint32_t vertex_idx);

// Apply force to vertex
void UpeSoftBody_ApplyForce(UpeSoftBody *body, uint32_t vertex_idx, UpeVec3 force);

// Get vertex position
UpeVec3 UpeSoftBody_GetVertexPosition(UpeSoftBody *body, uint32_t vertex_idx);

#ifdef __cplusplus
}
#endif

