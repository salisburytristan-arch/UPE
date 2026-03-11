#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_rng.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeCombustionNode {
    float fuel;
    float oxidizer;
    float temperature;
    float reaction_rate;
    float burn_damage;
} UpeCombustionNode;

typedef struct UpeSmokeParticle {
    UpeVec3 position;
    UpeVec3 velocity;
    float density;
    float temperature;
    float lifetime;
    uint8_t is_flame;
} UpeSmokeParticle;

typedef struct UpeCombustionGrid {
    UpeCombustionNode *nodes;
    float *temperature_buffer;
    UpeSmokeParticle *smoke_particles;
    
    uint32_t count;
    uint32_t smoke_particle_count;
    uint32_t max_smoke_particles;
    
    float ignition_temp;
    float heat_release;
    float thermal_conductivity;
    float ambient_temp;
    float cooling_rate;
    float smoke_threshold;
    float flame_threshold;

    UpeRng rng;
} UpeCombustionGrid;

// Core combustion grid functions
void UpeCombustionGrid_Init(UpeCombustionGrid *grid, uint32_t node_count);
void UpeCombustionGrid_Step(UpeCombustionGrid *grid, float dt);
void UpeCombustionGrid_Destroy(UpeCombustionGrid *grid);

// Manipulation functions
void UpeCombustionGrid_AddHeat(UpeCombustionGrid *grid, uint32_t node_idx, float heat);
void UpeCombustionGrid_SetIgnition(UpeCombustionGrid *grid, uint32_t node_idx, float fuel, float oxidizer);

// Simulation phases (called by Step)
void UpeCombustionGrid_ComputeReactions(UpeCombustionGrid *grid, float dt);
void UpeCombustionGrid_DiffuseHeat(UpeCombustionGrid *grid, float dt);
void UpeCombustionGrid_EmitSmoke(UpeCombustionGrid *grid);
void UpeCombustionGrid_UpdateSmoke(UpeCombustionGrid *grid, float dt);

#ifdef __cplusplus
}
#endif
