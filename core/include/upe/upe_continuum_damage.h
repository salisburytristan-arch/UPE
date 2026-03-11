#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Continuum Damage Mechanics
  
  - Progressive material failure
  - Crack propagation
  - Plastic deformation and yield
  - Stress/strain analysis
*/

typedef struct UpeDamageState {
    float damage;                // 0 = intact, 1 = fully damaged
    float plastic_strain;        // Accumulated plastic deformation
    UpeMatrix3 stress_tensor;    // Cauchy stress
    UpeMatrix3 strain_tensor;    // Strain state
    
    float damage_threshold;      // Stress level to start damage
    float fracture_toughness;    // Energy to propagate crack
    float yield_stress;          // Onset of plasticity
    
} UpeDamageState;

typedef struct UpeContinuumBody {
    UpeVec3 *nodes;              // FEM nodes
    uint32_t *tetrahedra;        // Tetrahedral elements (4 indices each)
    uint32_t node_count;
    uint32_t element_count;
    
    UpeDamageState *damage_states; // One per element
    
    // Material properties
    float youngs_modulus;
    float poissons_ratio;
    float density;
    float hardening_coefficient; // For plasticity
    
    // Fracture
    float *crack_tips;           // Positions of active crack tips
    uint32_t crack_count;
    
} UpeContinuumBody;

typedef struct UpeContinuumSystem {
    UpeContinuumBody *bodies;
    uint32_t body_count;
    uint32_t body_capacity;
    
    float time_step;
    
} UpeContinuumSystem;

void UpeContinuum_Init(UpeContinuumSystem *system, uint32_t max_bodies);
void UpeContinuum_Destroy(UpeContinuumSystem *system);

// Body management
uint32_t UpeContinuum_AddBody(UpeContinuumSystem *system, const UpeContinuumBody *body);

// Simulation
void UpeContinuum_Step(UpeContinuumSystem *system, float dt);

// Stress/strain computation
void UpeContinuum_ComputeStrain(UpeContinuumBody *body, uint32_t element_id, UpeMatrix3 *strain);
void UpeContinuum_ComputeStress(UpeContinuumBody *body, uint32_t element_id, const UpeMatrix3 *strain, UpeMatrix3 *stress);

// Damage evolution
void UpeContinuum_UpdateDamage(UpeContinuumBody *body, uint32_t element_id, float stress_magnitude, float dt);

// Plasticity
void UpeContinuum_ApplyPlasticity(UpeContinuumBody *body, uint32_t element_id, UpeMatrix3 *stress);

// Fracture propagation
void UpeContinuum_PropagateCracks(UpeContinuumBody *body, float dt);
void UpeContinuum_InsertCrack(UpeContinuumBody *body, UpeVec3 tip_position, UpeVec3 direction);

// Query
float UpeContinuum_GetVonMisesStress(const UpeMatrix3 *stress);
float UpeContinuum_GetPrincipalStress(const UpeMatrix3 *stress, uint32_t index); // 0, 1, or 2

#ifdef __cplusplus
}
#endif
