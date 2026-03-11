#include "upe/upe_pbd.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

void UpePbd_Init(UpePbdSystem *pbd, uint32_t max_particles, uint32_t max_constraints) {
    if (!pbd) return;
    
    memset(pbd, 0, sizeof(UpePbdSystem));
    
    pbd->particle_capacity = max_particles;
    pbd->particles = (UpePbdParticle*)malloc(max_particles * sizeof(UpePbdParticle));
    
    pbd->distance_capacity = max_constraints;
    pbd->distance_constraints = (UpePbdDistanceConstraint*)malloc(max_constraints * sizeof(UpePbdDistanceConstraint));
    
    pbd->bending_capacity = max_constraints / 2;
    pbd->bending_constraints = (UpePbdBendingConstraint*)malloc(pbd->bending_capacity * sizeof(UpePbdBendingConstraint));
    
    pbd->volume_capacity = max_constraints / 4;
    pbd->volume_constraints = (UpePbdVolumeConstraint*)malloc(pbd->volume_capacity * sizeof(UpePbdVolumeConstraint));
    
    pbd->collision_capacity = max_particles * 4;
    pbd->collision_constraints = (UpePbdCollisionConstraint*)malloc(pbd->collision_capacity * sizeof(UpePbdCollisionConstraint));
    
    // Default parameters
    pbd->gravity.x = 0.0f;
    pbd->gravity.y = -9.81f;
    pbd->gravity.z = 0.0f;
    pbd->solver_iterations = 4;
    pbd->damping = 0.99f;
    pbd->sleep_threshold = 0.01f;
    pbd->bounds_min.x = -100.0f;
    pbd->bounds_min.y = -10.0f;
    pbd->bounds_min.z = -100.0f;
    pbd->bounds_max.x = 100.0f;
    pbd->bounds_max.y = 100.0f;
    pbd->bounds_max.z = 100.0f;
    pbd->ground_friction = 0.5f;
    pbd->restitution = 0.0f;
}

void UpePbd_Destroy(UpePbdSystem *pbd) {
    if (!pbd) return;
    
    free(pbd->particles);
    free(pbd->distance_constraints);
    free(pbd->bending_constraints);
    free(pbd->volume_constraints);
    free(pbd->collision_constraints);
    
    memset(pbd, 0, sizeof(UpePbdSystem));
}

void UpePbd_Clear(UpePbdSystem *pbd) {
    if (!pbd) return;
    
    pbd->particle_count = 0;
    pbd->distance_count = 0;
    pbd->bending_count = 0;
    pbd->volume_count = 0;
    pbd->collision_count = 0;
}

uint32_t UpePbd_AddParticle(UpePbdSystem *pbd, UpeVec3 position, float mass, float radius) {
    if (!pbd || pbd->particle_count >= pbd->particle_capacity) {
        return 0xFFFFFFFF;
    }
    
    uint32_t index = pbd->particle_count++;
    UpePbdParticle *p = &pbd->particles[index];
    
    p->position = position;
    p->previous_pos = position;
    p->velocity.x = 0.0f;
    p->velocity.y = 0.0f;
    p->velocity.z = 0.0f;
    p->inv_mass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
    p->radius = radius;
    p->flags = 0;
    
    return index;
}

void UpePbd_PinParticle(UpePbdSystem *pbd, uint32_t index, int pinned) {
    if (!pbd || index >= pbd->particle_count) return;
    
    if (pinned) {
        pbd->particles[index].inv_mass = 0.0f;
        pbd->particles[index].flags |= 1;
    } else {
        pbd->particles[index].flags &= ~1;
    }
}

uint32_t UpePbd_AddDistanceConstraint(UpePbdSystem *pbd, uint32_t a, uint32_t b,
                                       float rest_length, float compliance) {
    if (!pbd || pbd->distance_count >= pbd->distance_capacity) {
        return 0xFFFFFFFF;
    }
    
    uint32_t index = pbd->distance_count++;
    UpePbdDistanceConstraint *c = &pbd->distance_constraints[index];
    
    c->particle_a = a;
    c->particle_b = b;
    c->rest_length = rest_length;
    c->compliance = compliance;
    c->lambda = 0.0f;
    
    return index;
}

uint32_t UpePbd_AddBendingConstraint(UpePbdSystem *pbd, uint32_t p0, uint32_t p1,
                                      uint32_t p2, uint32_t p3, float compliance) {
    if (!pbd || pbd->bending_count >= pbd->bending_capacity) {
        return 0xFFFFFFFF;
    }
    
    uint32_t index = pbd->bending_count++;
    UpePbdBendingConstraint *c = &pbd->bending_constraints[index];
    
    c->p0 = p0;
    c->p1 = p1;
    c->p2 = p2;
    c->p3 = p3;
    c->rest_angle = 0.0f; // Flat initial state
    c->compliance = compliance;
    c->lambda = 0.0f;
    
    return index;
}

uint32_t UpePbd_AddVolumeConstraint(UpePbdSystem *pbd, uint32_t p0, uint32_t p1,
                                     uint32_t p2, uint32_t p3, float compliance) {
    if (!pbd || pbd->volume_count >= pbd->volume_capacity) {
        return 0xFFFFFFFF;
    }
    
    // Compute rest volume
    UpePbdParticle *pa = &pbd->particles[p0];
    UpePbdParticle *pb = &pbd->particles[p1];
    UpePbdParticle *pc = &pbd->particles[p2];
    UpePbdParticle *pd = &pbd->particles[p3];
    
    UpeVec3 v1 = {pb->position.x - pa->position.x, pb->position.y - pa->position.y, pb->position.z - pa->position.z};
    UpeVec3 v2 = {pc->position.x - pa->position.x, pc->position.y - pa->position.y, pc->position.z - pa->position.z};
    UpeVec3 v3 = {pd->position.x - pa->position.x, pd->position.y - pa->position.y, pd->position.z - pa->position.z};
    
    // Volume = 1/6 * |v1 . (v2 x v3)|
    UpeVec3 cross;
    cross.x = v2.y * v3.z - v2.z * v3.y;
    cross.y = v2.z * v3.x - v2.x * v3.z;
    cross.z = v2.x * v3.y - v2.y * v3.x;
    
    float volume = fabsf(v1.x * cross.x + v1.y * cross.y + v1.z * cross.z) / 6.0f;
    
    uint32_t index = pbd->volume_count++;
    UpePbdVolumeConstraint *c = &pbd->volume_constraints[index];
    
    c->p0 = p0;
    c->p1 = p1;
    c->p2 = p2;
    c->p3 = p3;
    c->rest_volume = volume;
    c->compliance = compliance;
    c->lambda = 0.0f;
    
    return index;
}

// Solve distance constraint (XPBD)
static void SolveDistanceConstraint(UpePbdSystem *pbd, UpePbdDistanceConstraint *c, float dt) {
    UpePbdParticle *pa = &pbd->particles[c->particle_a];
    UpePbdParticle *pb = &pbd->particles[c->particle_b];
    
    float w_sum = pa->inv_mass + pb->inv_mass;
    if (w_sum < 1e-6f) return;
    
    UpeVec3 delta = {pb->position.x - pa->position.x,
                     pb->position.y - pa->position.y,
                     pb->position.z - pa->position.z};
    
    float dist = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
    if (dist < 1e-6f) return;
    
    UpeVec3 dir = {delta.x / dist, delta.y / dist, delta.z / dist};
    
    // XPBD with compliance
    float C = dist - c->rest_length;
    float alpha = c->compliance / (dt * dt);
    float delta_lambda = (-C - alpha * c->lambda) / (w_sum + alpha);
    c->lambda += delta_lambda;
    
    // Apply correction
    UpeVec3 correction = {dir.x * delta_lambda, dir.y * delta_lambda, dir.z * delta_lambda};
    
    pa->position.x -= correction.x * pa->inv_mass;
    pa->position.y -= correction.y * pa->inv_mass;
    pa->position.z -= correction.z * pa->inv_mass;
    
    pb->position.x += correction.x * pb->inv_mass;
    pb->position.y += correction.y * pb->inv_mass;
    pb->position.z += correction.z * pb->inv_mass;
}

// Solve bending constraint (isometric bending)
static void SolveBendingConstraint(UpePbdSystem *pbd, UpePbdBendingConstraint *c, float dt) {
    UpePbdParticle *p0 = &pbd->particles[c->p0];
    UpePbdParticle *p1 = &pbd->particles[c->p1];
    UpePbdParticle *p3 = &pbd->particles[c->p3];
    
    // Simplified isometric bending: keep distance between p0 and p3
    UpeVec3 delta = {p3->position.x - p0->position.x,
                     p3->position.y - p0->position.y,
                     p3->position.z - p0->position.z};
    
    float dist = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
    if (dist < 1e-6f) return;
    
    // Rest distance (heuristic: 2x edge length)
    UpeVec3 edge = {p1->position.x - p0->position.x,
                    p1->position.y - p0->position.y,
                    p1->position.z - p0->position.z};
    float edge_len = sqrtf(edge.x * edge.x + edge.y * edge.y + edge.z * edge.z);
    float rest_dist = edge_len * 1.414f;
    
    float w_sum = p0->inv_mass + p3->inv_mass;
    if (w_sum < 1e-6f) return;
    
    float C = dist - rest_dist;
    float alpha = c->compliance / (dt * dt);
    float delta_lambda = (-C - alpha * c->lambda) / (w_sum + alpha);
    c->lambda += delta_lambda;
    
    UpeVec3 dir = {delta.x / dist, delta.y / dist, delta.z / dist};
    UpeVec3 correction = {dir.x * delta_lambda, dir.y * delta_lambda, dir.z * delta_lambda};
    
    p0->position.x -= correction.x * p0->inv_mass;
    p0->position.y -= correction.y * p0->inv_mass;
    p0->position.z -= correction.z * p0->inv_mass;
    
    p3->position.x += correction.x * p3->inv_mass;
    p3->position.y += correction.y * p3->inv_mass;
    p3->position.z += correction.z * p3->inv_mass;
}

// Ground collision
static void SolveGroundCollision(UpePbdSystem *pbd, UpePbdParticle *p) {
    float ground_y = pbd->bounds_min.y;
    
    if (p->position.y - p->radius < ground_y) {
        // Position correction
        p->position.y = ground_y + p->radius;
        
        // Friction: tangential velocity damping
        UpeVec3 vel = {p->position.x - p->previous_pos.x,
                       p->position.y - p->previous_pos.y,
                       p->position.z - p->previous_pos.z};
        
        float friction_factor = 1.0f - pbd->ground_friction;
        p->previous_pos.x = p->position.x - vel.x * friction_factor;
        p->previous_pos.z = p->position.z - vel.z * friction_factor;
        
        // Restitution
        if (pbd->restitution > 0.0f) {
            p->previous_pos.y = p->position.y + (p->position.y - p->previous_pos.y) * pbd->restitution;
        }
    }
}

void UpePbd_Step(UpePbdSystem *pbd, float dt) {
    if (!pbd) return;
    
    // 1. Predict positions (integrate external forces)
    for (uint32_t i = 0; i < pbd->particle_count; i++) {
        UpePbdParticle *p = &pbd->particles[i];
        
        if (p->inv_mass == 0.0f) continue; // Pinned
        
        // Apply gravity
        p->velocity.x += pbd->gravity.x * dt;
        p->velocity.y += pbd->gravity.y * dt;
        p->velocity.z += pbd->gravity.z * dt;
        
        // Damping
        p->velocity.x *= pbd->damping;
        p->velocity.y *= pbd->damping;
        p->velocity.z *= pbd->damping;
        
        // Predict position
        p->previous_pos = p->position;
        p->position.x += p->velocity.x * dt;
        p->position.y += p->velocity.y * dt;
        p->position.z += p->velocity.z * dt;
    }
    
    // 2. Generate collision constraints
    pbd->collision_count = 0;
    for (uint32_t i = 0; i < pbd->particle_count; i++) {
        SolveGroundCollision(pbd, &pbd->particles[i]);
    }
    
    // 3. Solve constraints (iterative)
    for (uint32_t iter = 0; iter < pbd->solver_iterations; iter++) {
        // Distance constraints
        for (uint32_t i = 0; i < pbd->distance_count; i++) {
            SolveDistanceConstraint(pbd, &pbd->distance_constraints[i], dt);
        }
        
        // Bending constraints
        for (uint32_t i = 0; i < pbd->bending_count; i++) {
            SolveBendingConstraint(pbd, &pbd->bending_constraints[i], dt);
        }
        
        // Volume constraints (TODO: implement proper tet volume)
    }
    
    // 4. Update velocities
    float inv_dt = 1.0f / dt;
    for (uint32_t i = 0; i < pbd->particle_count; i++) {
        UpePbdParticle *p = &pbd->particles[i];
        
        p->velocity.x = (p->position.x - p->previous_pos.x) * inv_dt;
        p->velocity.y = (p->position.y - p->previous_pos.y) * inv_dt;
        p->velocity.z = (p->position.z - p->previous_pos.z) * inv_dt;
    }
}

void UpePbd_CreateClothGrid(UpePbdSystem *pbd,
                            uint32_t width, uint32_t height,
                            float spacing, float mass_per_particle,
                            float stretch_compliance, float bend_compliance) {
    if (!pbd) return;
    
    uint32_t start_index = pbd->particle_count;
    
    // Create particles
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            UpeVec3 pos = {x * spacing, 0.0f, y * spacing};
            UpePbd_AddParticle(pbd, pos, mass_per_particle, 0.05f);
        }
    }
    
    // Structural constraints (grid edges)
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t idx = start_index + y * width + x;
            
            // Horizontal
            if (x < width - 1) {
                uint32_t right = idx + 1;
                UpePbd_AddDistanceConstraint(pbd, idx, right, spacing, stretch_compliance);
            }
            
            // Vertical
            if (y < height - 1) {
                uint32_t down = idx + width;
                UpePbd_AddDistanceConstraint(pbd, idx, down, spacing, stretch_compliance);
            }
            
            // Diagonal (shear resistance)
            if (x < width - 1 && y < height - 1) {
                uint32_t diag = idx + width + 1;
                UpePbd_AddDistanceConstraint(pbd, idx, diag, spacing * 1.414f, stretch_compliance * 2.0f);
            }
            
            if (x > 0 && y < height - 1) {
                uint32_t diag = idx + width - 1;
                UpePbd_AddDistanceConstraint(pbd, idx, diag, spacing * 1.414f, stretch_compliance * 2.0f);
            }
        }
    }
    
    // Bending constraints (skip-one edges)
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t idx = start_index + y * width + x;
            
            if (x < width - 2) {
                uint32_t skip = idx + 2;
                UpePbd_AddDistanceConstraint(pbd, idx, skip, spacing * 2.0f, bend_compliance);
            }
            
            if (y < height - 2) {
                uint32_t skip = idx + width * 2;
                UpePbd_AddDistanceConstraint(pbd, idx, skip, spacing * 2.0f, bend_compliance);
            }
        }
    }
    
    // Pin top corners
    UpePbd_PinParticle(pbd, start_index, 1);
    UpePbd_PinParticle(pbd, start_index + width - 1, 1);
}

void UpePbd_CreateRope(UpePbdSystem *pbd,
                      UpeVec3 start, UpeVec3 end,
                      uint32_t segments, float mass_per_segment,
                      float compliance) {
    if (!pbd || segments < 2) return;
    
    uint32_t start_index = pbd->particle_count;
    
    UpeVec3 delta = {(end.x - start.x) / segments,
                     (end.y - start.y) / segments,
                     (end.z - start.z) / segments};
    
    float segment_length = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
    
    // Create particles
    for (uint32_t i = 0; i <= segments; i++) {
        UpeVec3 pos = {start.x + delta.x * i,
                       start.y + delta.y * i,
                       start.z + delta.z * i};
        UpePbd_AddParticle(pbd, pos, mass_per_segment, 0.02f);
    }
    
    // Create distance constraints
    for (uint32_t i = 0; i < segments; i++) {
        UpePbd_AddDistanceConstraint(pbd, start_index + i, start_index + i + 1,
                                     segment_length, compliance);
    }
    
    // Pin first particle
    UpePbd_PinParticle(pbd, start_index, 1);
}
