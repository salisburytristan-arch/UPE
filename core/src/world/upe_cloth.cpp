#include "upe/upe_cloth.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

void UpeCloth_Init(UpeCloth *cloth, uint32_t grid_w, uint32_t grid_h, float spacing) {
    if (!cloth) {
        return;
    }
    uint32_t vertex_count = grid_w * grid_h;
    cloth->vertex_count = vertex_count;
    cloth->grid_w = grid_w;
    cloth->grid_h = grid_h;
    cloth->vertices = (UpeClothVertex *)malloc(vertex_count * sizeof(UpeClothVertex));
    cloth->self_collision_enabled = 1;
    cloth->self_collision_radius = spacing * 0.5f;
    cloth->tearing_enabled = 1;
    cloth->tear_distance = spacing * 2.0f;
    cloth->wind_velocity = {0.0f, 0.0f, 0.0f};
    cloth->air_density = 1.2f;
    cloth->drag_coefficient = 0.1f;
    
    // XPBD parameters
    cloth->xpbd_enabled = 1;               // Use XPBD by default
    cloth->damping = 0.99f;                // 1% damping per frame
    cloth->friction_coefficient = 0.3f;
    cloth->wrinkle_count = 0;
    cloth->wrinkle_capacity = 500;
    cloth->wrinkles = (UpeClothWrinkleConstraint *)malloc(cloth->wrinkle_capacity * sizeof(UpeClothWrinkleConstraint));
    
    for (uint32_t y = 0; y < grid_h; ++y) {
        for (uint32_t x = 0; x < grid_w; ++x) {
            uint32_t idx = y * grid_w + x;
            cloth->vertices[idx].position = {x * spacing, y * spacing, 0.0f};
            cloth->vertices[idx].velocity = {0.0f, 0.0f, 0.0f};
            cloth->vertices[idx].inv_mass = 1.0f;
            cloth->vertices[idx].pinned = (y == 0) ? 1 : 0;
        }
    }
    
    // Structural constraints (horizontal + vertical)
    uint32_t structural_count = ((grid_h - 1) * grid_w) + ((grid_w - 1) * grid_h);
    
    // Shear constraints (diagonals)
    uint32_t shear_count = (grid_w - 1) * (grid_h - 1) * 2;
    
    // Bending constraints (skip-one structural)
    uint32_t bending_count = ((grid_h - 2) * grid_w) + ((grid_w - 2) * grid_h);
    
    uint32_t constraint_count = structural_count + shear_count + bending_count;
    cloth->constraint_count = constraint_count;
    cloth->constraint_capacity = constraint_count + 100; // Extra for runtime additions
    cloth->constraints = (UpeClothConstraint *)malloc(cloth->constraint_capacity * sizeof(UpeClothConstraint));
    
    uint32_t c_idx = 0;
    const uint32_t iterations = 5;  // For XPBD compliance calculation
    
    // Structural horizontal
    for (uint32_t y = 0; y < grid_h; ++y) {
        for (uint32_t x = 0; x < grid_w - 1; ++x) {
            uint32_t a = y * grid_w + x;
            uint32_t b = y * grid_w + x + 1;
            cloth->constraints[c_idx].vertex_a = a;
            cloth->constraints[c_idx].vertex_b = b;
            cloth->constraints[c_idx].rest_length = spacing;
            cloth->constraints[c_idx].stiffness = 1.0f;
            cloth->constraints[c_idx].type = 0; // Structural
            cloth->constraints[c_idx].compliance = 1.0f / (cloth->constraints[c_idx].stiffness * iterations);
            cloth->constraints[c_idx].lambda = 0.0f;
            c_idx++;
        }
    }
    
    // Structural vertical
    for (uint32_t y = 0; y < grid_h - 1; ++y) {
        for (uint32_t x = 0; x < grid_w; ++x) {
            uint32_t a = y * grid_w + x;
            uint32_t b = (y + 1) * grid_w + x;
            cloth->constraints[c_idx].vertex_a = a;
            cloth->constraints[c_idx].vertex_b = b;
            cloth->constraints[c_idx].rest_length = spacing;
            cloth->constraints[c_idx].stiffness = 1.0f;
            cloth->constraints[c_idx].type = 0; // Structural
            cloth->constraints[c_idx].compliance = 1.0f / (cloth->constraints[c_idx].stiffness * iterations);
            cloth->constraints[c_idx].lambda = 0.0f;
            c_idx++;
        }
    }
    
    // Shear constraints (diagonals for in-plane resistance)
    for (uint32_t y = 0; y < grid_h - 1; ++y) {
        for (uint32_t x = 0; x < grid_w - 1; ++x) {
            // Diagonal 1
            uint32_t a = y * grid_w + x;
            uint32_t b = (y + 1) * grid_w + x + 1;
            cloth->constraints[c_idx].vertex_a = a;
            cloth->constraints[c_idx].vertex_b = b;
            cloth->constraints[c_idx].rest_length = spacing * sqrtf(2.0f);
            cloth->constraints[c_idx].stiffness = 0.8f;
            cloth->constraints[c_idx].type = 1; // Shear
            cloth->constraints[c_idx].compliance = 1.0f / (cloth->constraints[c_idx].stiffness * iterations);
            cloth->constraints[c_idx].lambda = 0.0f;
            c_idx++;
            
            // Diagonal 2
            a = y * grid_w + x + 1;
            b = (y + 1) * grid_w + x;
            cloth->constraints[c_idx].vertex_a = a;
            cloth->constraints[c_idx].vertex_b = b;
            cloth->constraints[c_idx].rest_length = spacing * sqrtf(2.0f);
            cloth->constraints[c_idx].stiffness = 0.8f;
            cloth->constraints[c_idx].type = 1; // Shear
            cloth->constraints[c_idx].compliance = 1.0f / (cloth->constraints[c_idx].stiffness * iterations);
            cloth->constraints[c_idx].lambda = 0.0f;
            c_idx++;
        }
    }
    
    // Bending constraints (connect vertices 2 apart for bending resistance)
    for (uint32_t y = 0; y < grid_h; ++y) {
        for (uint32_t x = 0; x < grid_w - 2; ++x) {
            uint32_t a = y * grid_w + x;
            uint32_t b = y * grid_w + x + 2;
            cloth->constraints[c_idx].vertex_a = a;
            cloth->constraints[c_idx].vertex_b = b;
            cloth->constraints[c_idx].rest_length = spacing * 2.0f;
            cloth->constraints[c_idx].stiffness = 0.5f;
            cloth->constraints[c_idx].type = 2; // Bending
            cloth->constraints[c_idx].compliance = 1.0f / (cloth->constraints[c_idx].stiffness * iterations);
            cloth->constraints[c_idx].lambda = 0.0f;
            c_idx++;
        }
    }
    
    for (uint32_t y = 0; y < grid_h - 2; ++y) {
        for (uint32_t x = 0; x < grid_w; ++x) {
            uint32_t a = y * grid_w + x;
            uint32_t b = (y + 2) * grid_w + x;
            cloth->constraints[c_idx].vertex_a = a;
            cloth->constraints[c_idx].vertex_b = b;
            cloth->constraints[c_idx].rest_length = spacing * 2.0f;
            cloth->constraints[c_idx].stiffness = 0.5f;
            cloth->constraints[c_idx].type = 2; // Bending
            cloth->constraints[c_idx].compliance = 1.0f / (cloth->constraints[c_idx].stiffness * iterations);
            cloth->constraints[c_idx].lambda = 0.0f;
            c_idx++;
        }
    }
}

void UpeCloth_Step(UpeCloth *cloth, float dt, UpeVec3 gravity) {
    if (!cloth) {
        return;
    }
    
    // Use proven PBD solver (v0.13.0 stable)
    // XPBD solver available but requires further debugging for production use
    
    // Integration with gravity and wind
    for (uint32_t i = 0; i < cloth->vertex_count; ++i) {
        if (cloth->vertices[i].pinned) {
            continue;
        }
        
        // Gravity is acceleration, independent of particle mass
        UpeVec3 accel = gravity;
        
        // Wind drag force: F = 0.5 * rho * Cd * A * v²
        // Simplified: acceleration += wind_velocity * Cd * rho
        UpeVec3 relative_wind = UpeVec3_Sub(cloth->wind_velocity, cloth->vertices[i].velocity);
        float wind_mag_sq = relative_wind.x * relative_wind.x + 
                           relative_wind.y * relative_wind.y + 
                           relative_wind.z * relative_wind.z;
        
        if (wind_mag_sq > 1.0e-6f) {
            float wind_factor = cloth->air_density * cloth->drag_coefficient * cloth->vertices[i].inv_mass;
            accel = UpeVec3_Add(accel, UpeVec3_Scale(relative_wind, wind_factor * 0.1f));
        }
        
        cloth->vertices[i].velocity = UpeVec3_Add(cloth->vertices[i].velocity, UpeVec3_Scale(accel, dt));
        cloth->vertices[i].position = UpeVec3_Add(cloth->vertices[i].position, UpeVec3_Scale(cloth->vertices[i].velocity, dt));
    }
    
    // Constraint solving iterations (PBD - Position-Based Dynamics)
    const uint32_t iterations = 5;
    for (uint32_t iter = 0; iter < iterations; ++iter) {
        // Distance constraints
        for (uint32_t i = 0; i < cloth->constraint_count; ++i) {
            UpeClothConstraint *c = &cloth->constraints[i];
            UpeClothVertex *a = &cloth->vertices[c->vertex_a];
            UpeClothVertex *b = &cloth->vertices[c->vertex_b];
            
            UpeVec3 delta = UpeVec3_Sub(b->position, a->position);
            float len = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
            if (len < 1.0e-6f) {
                continue;
            }
            float error = len - c->rest_length;
            UpeVec3 correction = UpeVec3_Scale(delta, (error / len) * 0.5f * c->stiffness);
            
            if (!a->pinned) {
                a->position = UpeVec3_Add(a->position, correction);
            }
            if (!b->pinned) {
                b->position = UpeVec3_Sub(b->position, correction);
            }
        }
        
        // Self-collision (every other iteration to reduce cost)
        if (iter % 2 == 0 && cloth->self_collision_enabled) {
            float radius_sq = cloth->self_collision_radius * cloth->self_collision_radius;
            
            for (uint32_t i = 0; i < cloth->vertex_count; ++i) {
                if (cloth->vertices[i].pinned) {
                    continue;
                }
                
                for (uint32_t j = i + 1; j < cloth->vertex_count; ++j) {
                    if (cloth->vertices[j].pinned) {
                        continue;
                    }
                    
                    // Skip adjacent vertices (they're already constrained)
                    int adjacent = 0;
                    if (cloth->grid_w > 0) {
                        uint32_t row_i = i / cloth->grid_w;
                        uint32_t col_i = i % cloth->grid_w;
                        uint32_t row_j = j / cloth->grid_w;
                        uint32_t col_j = j % cloth->grid_w;
                        
                        if (abs((int)row_i - (int)row_j) <= 1 && abs((int)col_i - (int)col_j) <= 1) {
                            adjacent = 1;
                        }
                    }
                    if (adjacent) {
                        continue;
                    }
                    
                    UpeVec3 delta = UpeVec3_Sub(cloth->vertices[j].position, cloth->vertices[i].position);
                    float dist_sq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
                    
                    if (dist_sq < radius_sq && dist_sq > 1.0e-8f) {
                        float dist = sqrtf(dist_sq);
                        float overlap = cloth->self_collision_radius - dist;
                        UpeVec3 correction = UpeVec3_Scale(delta, overlap / (2.0f * dist));
                        
                        cloth->vertices[i].position = UpeVec3_Sub(cloth->vertices[i].position, correction);
                        cloth->vertices[j].position = UpeVec3_Add(cloth->vertices[j].position, correction);
                    }
                }
            }
        }
    }
    
    // Tearing (check after constraint solving)
    if (cloth->tearing_enabled) {
        for (uint32_t i = 0; i < cloth->constraint_count; ++i) {
            UpeClothConstraint *c = &cloth->constraints[i];
            
            UpeVec3 delta = UpeVec3_Sub(cloth->vertices[c->vertex_b].position, 
                                        cloth->vertices[c->vertex_a].position);
            float len = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
            
            if (len > cloth->tear_distance) {
                if (i < cloth->constraint_count - 1) {
                    cloth->constraints[i] = cloth->constraints[cloth->constraint_count - 1];
                }
                cloth->constraint_count--;
                i--;
            }
        }
    }
}

void UpeCloth_Destroy(UpeCloth *cloth) {
    if (!cloth) {
        return;
    }
    free(cloth->vertices);
    free(cloth->constraints);
    free(cloth->wrinkles);
}
