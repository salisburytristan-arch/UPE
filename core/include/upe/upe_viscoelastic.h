#pragma once

#include "upe/upe_fem.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Viscoelastic Material Models
  
  - Kelvin-Voigt: Spring + damper in parallel (solid-like behavior)
  - Maxwell: Spring + damper in series (fluid-like behavior)
  - Blended model for realistic mud, tar, and putty behavior
*/

typedef struct UpeViscoelasticElement {
    // Kelvin-Voigt parameters
    float kv_modulus;           // E_KV (elastic modulus)
    float kv_viscosity;         // η_KV (damping coefficient)
    
    // Maxwell parameters
    float maxwell_modulus;      // E_M (elastic modulus)
    float maxwell_viscosity;    // η_M (viscosity)
    
    // Material state (strain history)
    float strain[9];            // Current strain tensor
    float strain_rate[9];       // dε/dt
    
    // Stress state
    float stress[9];            // Current stress
    float stress_prev[9];       // Previous step stress (for Maxwell)
    
    // Plasticity/damage
    float damage;               // 0-1 degradation
    
} UpeViscoelasticElement;

typedef struct UpeViscoelasticSystem {
    UpeFemSystem *fem;
    UpeViscoelasticElement *elements;
    uint32_t element_count;
    float dt;
} UpeViscoelasticSystem;

// System management
void UpeViscoelastic_Init(UpeViscoelasticSystem *visco, UpeFemSystem *fem);
void UpeViscoelastic_Destroy(UpeViscoelasticSystem *visco);

// Material presets
void UpeViscoelastic_SetMaterialMud(UpeViscoelasticSystem *visco);
void UpeViscoelastic_SetMaterialTar(UpeViscoelasticSystem *visco);
void UpeViscoelastic_SetMaterialPutty(UpeViscoelasticSystem *visco);
void UpeViscoelastic_SetMaterialAsphalt(UpeViscoelasticSystem *visco);

// Stress computation
void UpeViscoelastic_UpdateStress(UpeViscoelasticSystem *visco, uint32_t elem_id, 
                                  const float strain[9], const float strain_rate[9], float dt);
void UpeViscoelastic_GetStress(const UpeViscoelasticSystem *visco, uint32_t elem_id, float stress_out[9]);

// Material properties
float UpeViscoelastic_GetDamage(const UpeViscoelasticSystem *visco, uint32_t elem_id);
float UpeViscoelastic_GetEffectiveViscosity(const UpeViscoelasticSystem *visco, uint32_t elem_id);
float UpeViscoelastic_GetEffectiveModulus(const UpeViscoelasticSystem *visco, uint32_t elem_id);

// Rheological models (for analysis/visualization)
float UpeViscoelastic_CreepCompliance_KV(float E, float eta, float time);
float UpeViscoelastic_StressRelaxation_Maxwell(float E, float eta, float time);

/* Rate-dependent stress hysteresis (Preisach-style history buffer) */
#define UPE_HYSTERESIS_HISTORY 8
typedef struct UpeHysteresisState {
  float stress_history[UPE_HYSTERESIS_HISTORY];
  float strain_history[UPE_HYSTERESIS_HISTORY];
  uint32_t idx;
  float tau;   /* relaxation time constant  */
  float eta;   /* viscosity coefficient      */
} UpeHysteresisState;

/* Returns updated stress given new strain and strain rate.
   E: elastic modulus  dt: timestep */
float UpeHysteresis_Stress(UpeHysteresisState *s, float strain, float strain_rate,
              float E, float dt);

#ifdef __cplusplus
}
#endif
