#include <stdlib.h>
#include <string.h>
#include "upe/upe_softbody.h"
#include "upe/upe_math.h"

// Forward declarations for internal helpers
static void SoftBody_VerletIntegrate(UpeSoftBody *body, float dt);
static void SoftBody_SolveConstraints(UpeSoftBody *body, uint32_t iterations);
static void SoftBody_UpdateVelocities(UpeSoftBody *body, float dt);

// Create soft body
UpeSoftBody *UpeSoftBody_Create(const UpeSoftBodyDesc *desc)
{
    if (!desc || desc->vertex_count == 0) {
        return NULL;
    }

    UpeSoftBody *body = (UpeSoftBody *)malloc(sizeof(UpeSoftBody));
    if (!body) return NULL;

    body->id = (uint32_t)((uintptr_t)body / sizeof(UpeSoftBody)); // Simple ID from address

    // Copy vertex data
    body->vertex_count = desc->vertex_count;
    body->vertices = (UpeSoftBodyVertex *)malloc(sizeof(UpeSoftBodyVertex) * desc->vertex_count);
    if (!body->vertices) {
        free(body);
        return NULL;
    }
    
    for (uint32_t i = 0; i < desc->vertex_count; i++) {
        body->vertices[i] = desc->vertices[i];
        // Compute inverse mass
        if (body->vertices[i].mass > 0.0f) {
            body->vertices[i].inv_mass = 1.0f / body->vertices[i].mass;
        } else {
            body->vertices[i].inv_mass = 0.0f;  // Pinned
        }
        // Initialize velocity as zero (Verlet tracks via position history)
        body->vertices[i].velocity.x = 0.0f;
        body->vertices[i].velocity.y = 0.0f;
        body->vertices[i].velocity.z = 0.0f;
        body->vertices[i].previous_position = body->vertices[i].position;
    }

    // Copy constraint data
    body->constraint_count = desc->constraint_count;
    body->constraints = (UpeSoftBodyConstraint *)malloc(sizeof(UpeSoftBodyConstraint) * desc->constraint_count);
    if (body->constraints) {
        memcpy(body->constraints, desc->constraints, sizeof(UpeSoftBodyConstraint) * desc->constraint_count);
    }

    // Set simulation parameters with defaults
    body->gravity = desc->gravity;
    body->air_damping = desc->air_damping > 0.0f ? desc->air_damping : 0.99f;  // Default: 1% damping per frame
    body->wind_force = desc->wind_force >= 0.0f ? desc->wind_force : 0.0f;
    body->wind_direction = desc->wind_direction;

    return body;
}

// Destroy soft body
void UpeSoftBody_Destroy(UpeSoftBody *body)
{
    if (!body) return;
    
    if (body->vertices) {
        free(body->vertices);
        body->vertices = NULL;
    }
    
    if (body->constraints) {
        free(body->constraints);
        body->constraints = NULL;
    }
    
    free(body);
}

// Verlet integration step
static void SoftBody_VerletIntegrate(UpeSoftBody *body, float dt)
{
    // Apply forces (gravity, wind, damping) using Verlet
    for (uint32_t i = 0; i < body->vertex_count; i++) {
        UpeSoftBodyVertex *v = &body->vertices[i];
        
        if (v->inv_mass == 0.0f) {
            // Pinned vertex: keep position fixed
            v->previous_position = v->position;
            continue;
        }

        // Compute velocity from position history
        UpeVec3 vel = UpeVec3_Sub(v->position, v->previous_position);
        
        // Apply damping (air resistance)
        vel = UpeVec3_Scale(vel, body->air_damping);
        
        // Accumulate forces
        UpeVec3 acceleration = body->gravity;  // Gravity acceleration
        
        // Add wind force (acts perpendicular to surface, simplified as constant direction)
        if (body->wind_force > 0.0f) {
            UpeVec3 wind_accel = UpeVec3_Scale(body->wind_direction, body->wind_force * v->inv_mass);
            acceleration = UpeVec3_Add(acceleration, wind_accel);
        }
        
        // Verlet integration: p_new = p + v + a*dt²
        UpeVec3 accel_dt2 = UpeVec3_Scale(acceleration, dt * dt);
        UpeVec3 new_pos = UpeVec3_Add(v->position, vel);
        new_pos = UpeVec3_Add(new_pos, accel_dt2);
        
        // Update position history
        v->previous_position = v->position;
        v->position = new_pos;
    }
}

// Constraint solving (XPBD: eXtended Position Based Dynamics)
static void SoftBody_SolveConstraints(UpeSoftBody *body, uint32_t iterations)
{
    // Iteratively satisfy distance constraints
    for (uint32_t iter = 0; iter < iterations; iter++) {
        for (uint32_t c = 0; c < body->constraint_count; c++) {
            UpeSoftBodyConstraint *con = &body->constraints[c];
            
            uint32_t a = con->vertex_a;
            uint32_t b = con->vertex_b;
            
            if (a >= body->vertex_count || b >= body->vertex_count) {
                continue;  // Invalid constraint, skip
            }
            
            UpeSoftBodyVertex *va = &body->vertices[a];
            UpeSoftBodyVertex *vb = &body->vertices[b];
            
            // Skip if both pinned
            if (va->inv_mass == 0.0f && vb->inv_mass == 0.0f) {
                continue;
            }
            
            // Compute current distance
            UpeVec3 delta = UpeVec3_Sub(vb->position, va->position);
            float dist = UpeVec3_Length(delta);
            
            if (dist < 0.0001f) {
                continue;  // Degenerate constraint
            }
            
            // Constraint error
            float error = dist - con->rest_length;
            
            // Scale correction: how much to move to satisfy constraint
            // w = inv_mass_a + inv_mass_b + compliance term
            float w = va->inv_mass + vb->inv_mass + con->compliance;
            if (w < 0.0001f) {
                continue;  // No valid weights
            }
            
            // XPBD Lagrange multiplier approach
            // Scale = -error / w (moves proportionally to inverse mass)
            float scale = error / w;

            // Correction vector (normalized delta × scale)
            UpeVec3 correction = UpeVec3_Scale(UpeVec3_Normalize(delta), scale);

            // Apply corrections (each moves toward other by inv_mass fraction)
            va->position = UpeVec3_Add(va->position, UpeVec3_Scale(correction, va->inv_mass));
            vb->position = UpeVec3_Sub(vb->position, UpeVec3_Scale(correction, vb->inv_mass));
        }
    }
}

// Update velocities from position changes
static void SoftBody_UpdateVelocities(UpeSoftBody *body, float dt)
{
    if (dt < 0.0001f) return;
    
    for (uint32_t i = 0; i < body->vertex_count; i++) {
        UpeSoftBodyVertex *v = &body->vertices[i];
        
        // Velocity = (position - previous_position) / dt
        // But we already did position updates, so just compute from position change
        UpeVec3 pos_delta = UpeVec3_Sub(v->position, v->previous_position);
        v->velocity = UpeVec3_Scale(pos_delta, 1.0f / dt);
    }
}

// Step soft body simulation
void UpeSoftBody_Step(UpeSoftBody *body, float dt, uint32_t solver_iterations)
{
    if (!body || dt < 0.0001f) {
        return;
    }
    
    // Clamp dt for numerical stability
    if (dt > 0.1f) {
        dt = 0.1f;
    }
    
    // Phase 1: Verlet integration (predict new positions)
    SoftBody_VerletIntegrate(body, dt);
    
    // Phase 2: Constraint solving (project to valid positions)
    if (solver_iterations > 0) {
        SoftBody_SolveConstraints(body, solver_iterations);
    }
    
    // Phase 3: Update velocities for contact/collision detection
    SoftBody_UpdateVelocities(body, dt);
}

// Pin vertex to current position
void UpeSoftBody_PinVertex(UpeSoftBody *body, uint32_t vertex_idx, UpeVec3 position)
{
    if (!body || vertex_idx >= body->vertex_count) {
        return;
    }
    
    UpeSoftBodyVertex *v = &body->vertices[vertex_idx];
    v->position = position;
    v->previous_position = position;  // Verlet: zero velocity when pinned
    v->inv_mass = 0.0f;                 // Infinite mass (immobile)
    v->pinned = 1;
}

// Unpin vertex (restore mass)
void UpeSoftBody_UnpinVertex(UpeSoftBody *body, uint32_t vertex_idx)
{
    if (!body || vertex_idx >= body->vertex_count) {
        return;
    }
    
    UpeSoftBodyVertex *v = &body->vertices[vertex_idx];
    if (v->mass > 0.0f) {
        v->inv_mass = 1.0f / v->mass;
    } else {
        v->inv_mass = 1.0f;  // Default to 1 kg if mass not set
    }
    v->pinned = 0;
}

// Apply impulse to vertex
void UpeSoftBody_ApplyForce(UpeSoftBody *body, uint32_t vertex_idx, UpeVec3 force)
{
    if (!body || vertex_idx >= body->vertex_count) {
        return;
    }
    
    UpeSoftBodyVertex *v = &body->vertices[vertex_idx];
    if (v->inv_mass == 0.0f) {
        return;  // Pinned, can't move
    }
    
    // Force = mass × acceleration, so adjust position directly via Verlet
    // Approximation: add force as impulse (instant velocity change)
    UpeVec3 impulse_accel = UpeVec3_Scale(force, v->inv_mass);
    v->position = UpeVec3_Add(v->position, impulse_accel);
}

// Get vertex position
UpeVec3 UpeSoftBody_GetVertexPosition(UpeSoftBody *body, uint32_t vertex_idx)
{
    if (!body || vertex_idx >= body->vertex_count) {
        UpeVec3 zero_vec;
        zero_vec.x = 0.0f;
        zero_vec.y = 0.0f;
        zero_vec.z = 0.0f;
        return zero_vec;
    }
    
    return body->vertices[vertex_idx].position;
}

