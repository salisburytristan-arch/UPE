#include "upe/upe_fem.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// VISCOELASTIC MATERIAL MODELS
//
// Kelvin-Voigt: Spring + damper in PARALLEL (solid-like creep)
//   σ = E * ε + η * dε/dt
//
// Maxwell: Spring + damper in SERIES (fluid-like stress relaxation)
//   dσ/dt + (E/η) * σ = E * dε/dt
//
// Used for: mud, tar, silly putty, asphalt, etc.
// ============================================================================

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

// Add viscoelastic extension to FEM system
typedef struct UpeViscoelasticSystem {
    UpeFemSystem *fem;
    UpeViscoelasticElement *elements;  // One per FEM element
    uint32_t element_count;
    
    float dt;
} UpeViscoelasticSystem;

// Initialize viscoelastic system
void UpeViscoelastic_Init(UpeViscoelasticSystem *visco, UpeFemSystem *fem) {
    if (!visco || !fem) return;
    
    visco->fem = fem;
    visco->element_count = fem->element_count;
    visco->elements = (UpeViscoelasticElement*)calloc(fem->element_count, sizeof(UpeViscoelasticElement));
    
    // Initialize all elements to zero (identity needed later)
    for (uint32_t i = 0; i < fem->element_count; ++i) {
        memset(&visco->elements[i].strain, 0, sizeof(float) * 9);
        memset(&visco->elements[i].strain_rate, 0, sizeof(float) * 9);
        memset(&visco->elements[i].stress, 0, sizeof(float) * 9);
        memset(&visco->elements[i].stress_prev, 0, sizeof(float) * 9);
        visco->elements[i].damage = 0.0f;
    }
}

void UpeViscoelastic_Destroy(UpeViscoelasticSystem *visco) {
    if (!visco) return;
    free(visco->elements);
    memset(visco, 0, sizeof(UpeViscoelasticSystem));
}

// Kelvin-Voigt model: σ = E * ε + η * dε/dt
static void KelvinVoigtStress(float E, float eta, const float strain[9], const float strain_rate[9], float stress_out[9]) {
    for (int i = 0; i < 9; ++i) {
        stress_out[i] = E * strain[i] + eta * strain_rate[i];
    }
}

// Maxwell model: dσ/dt + (E/η) * σ = E * dε/dt
// Backward Euler: σ_new = (E * dε + σ_old) / (1 + E/η * Δt)
static void MaxwellStress(float E, float eta, float dt, const float strain_rate[9], 
                          const float stress_old[9], float stress_new[9]) {
    float alpha = E / (eta + E * dt);  // After simplification
    
    for (int i = 0; i < 9; ++i) {
        stress_new[i] = alpha * (E * strain_rate[i] * dt + stress_old[i]);
    }
}

// Mud-like material (high viscosity, low elasticity)
void UpeViscoelastic_SetMaterialMud(UpeViscoelasticSystem *visco) {
    if (!visco || !visco->elements) return;
    
    for (uint32_t i = 0; i < visco->element_count; ++i) {
        visco->elements[i].kv_modulus = 1e4f;        // Low elasticity (10 kPa)
        visco->elements[i].kv_viscosity = 1e5f;      // High viscosity (100 Pa·s)
        
        visco->elements[i].maxwell_modulus = 5e3f;
        visco->elements[i].maxwell_viscosity = 5e5f;
    }
}

// Tar-like material (very high viscosity, very low elasticity)
void UpeViscoelastic_SetMaterialTar(UpeViscoelasticSystem *visco) {
    if (!visco || !visco->elements) return;
    
    for (uint32_t i = 0; i < visco->element_count; ++i) {
        visco->elements[i].kv_modulus = 1e3f;        // Very low elasticity (1 kPa)
        visco->elements[i].kv_viscosity = 1e7f;      // Extreme viscosity (10 MPa·s)
        
        visco->elements[i].maxwell_modulus = 5e2f;
        visco->elements[i].maxwell_viscosity = 5e7f;
    }
}

// Silly putty (moderate viscosity, moderate elasticity, reversible deformation)
void UpeViscoelastic_SetMaterialPutty(UpeViscoelasticSystem *visco) {
    if (!visco || !visco->elements) return;
    
    for (uint32_t i = 0; i < visco->element_count; ++i) {
        visco->elements[i].kv_modulus = 1e5f;        // Moderate elasticity (100 kPa)
        visco->elements[i].kv_viscosity = 1e4f;      // Moderate viscosity (10 Pa·s)
        
        visco->elements[i].maxwell_modulus = 5e4f;
        visco->elements[i].maxwell_viscosity = 2e4f;
    }
}

// Asphalt (temperature-dependent, roughly modeled as moderate properties)
void UpeViscoelastic_SetMaterialAsphalt(UpeViscoelasticSystem *visco) {
    if (!visco || !visco->elements) return;
    
    for (uint32_t i = 0; i < visco->element_count; ++i) {
        visco->elements[i].kv_modulus = 1e7f;        // High elasticity (10 MPa)
        visco->elements[i].kv_viscosity = 1e3f;      // Low viscosity at high temp
        
        visco->elements[i].maxwell_modulus = 5e6f;
        visco->elements[i].maxwell_viscosity = 5e3f;
    }
}

// Update viscoelastic element stress given strain history
void UpeViscoelastic_UpdateStress(UpeViscoelasticSystem *visco, uint32_t elem_id, 
                                  const float strain[9], const float strain_rate[9], float dt) {
    if (!visco || elem_id >= visco->element_count) return;
    
    UpeViscoelasticElement *elem = &visco->elements[elem_id];
    
    // Store strain history
    memcpy(elem->strain, strain, 9 * sizeof(float));
    memcpy(elem->strain_rate, strain_rate, 9 * sizeof(float));
    
    // Compute Kelvin-Voigt stress
    float kv_stress[9];
    KelvinVoigtStress(elem->kv_modulus, elem->kv_viscosity, strain, strain_rate, kv_stress);
    
    // Compute Maxwell stress (with memory of previous stress)
    float maxwell_stress[9];
    MaxwellStress(elem->maxwell_modulus, elem->maxwell_viscosity, dt, strain_rate, 
                  elem->stress_prev, maxwell_stress);
    
    // Blended stress: roughly 60% Kelvin-Voigt (solid-like), 40% Maxwell (fluid-like)
    for (int i = 0; i < 9; ++i) {
        elem->stress_prev[i] = maxwell_stress[i];  // Store for next iteration
        elem->stress[i] = 0.6f * kv_stress[i] + 0.4f * maxwell_stress[i];
    }
    
    // Damage: reduce stress under extreme deformations (strain > 0.3)
    float strain_norm = 0.0f;
    for (int i = 0; i < 9; ++i) {
        strain_norm += strain[i] * strain[i];
    }
    strain_norm = sqrtf(strain_norm);
    
    if (strain_norm > 0.3f) {
        elem->damage += 0.005f * (strain_norm - 0.3f);
        elem->damage = (elem->damage > 1.0f) ? 1.0f : elem->damage;
        
        // Reduce stress by damage accumulation
        for (int i = 0; i < 9; ++i) {
            elem->stress[i] *= (1.0f - elem->damage * 0.5f);
        }
    }
}

// Get viscoelastic stress
void UpeViscoelastic_GetStress(const UpeViscoelasticSystem *visco, uint32_t elem_id, float stress_out[9]) {
    if (!visco || elem_id >= visco->element_count) {
        memset(stress_out, 0, 9 * sizeof(float));
        return;
    }
    
    memcpy(stress_out, visco->elements[elem_id].stress, 9 * sizeof(float));
}

// Get damage factor (0 = intact, 1 = fully damaged)
float UpeViscoelastic_GetDamage(const UpeViscoelasticSystem *visco, uint32_t elem_id) {
    if (!visco || elem_id >= visco->element_count) {
        return 0.0f;
    }
    
    return visco->elements[elem_id].damage;
}

// ============================================================================
// HELPER: Compute creep compliance J(t) for Kelvin-Voigt
// J(t) = 1/E * (1 - exp(-E/η * t))
// Shows how much deformation occurs under constant stress
// ============================================================================
float UpeViscoelastic_CreepCompliance_KV(float E, float eta, float time) {
    if (E <= 0.0f) return 0.0f;

    return (1.0f / E) * (1.0f - expf(-E / eta * time));
}

// ============================================================================
// HELPER: Compute stress relaxation G(t) for Maxwell
// G(t) = E * exp(-E/η * t)
// Shows how fast stress decreases under constant strain
// ============================================================================
float UpeViscoelastic_StressRelaxation_Maxwell(float E, float eta, float time) {
    if (E <= 0.0f) return 0.0f;
    
    return E * expf(-E / eta * time);
}

// ============================================================================
// HELPER: Compute effective viscosity
// Lower = flows faster, Higher = more rigid
// ============================================================================
float UpeViscoelastic_GetEffectiveViscosity(const UpeViscoelasticSystem *visco, uint32_t elem_id) {
    if (!visco || elem_id >= visco->element_count) {
        return 0.0f;
    }
    
    const UpeViscoelasticElement *elem = &visco->elements[elem_id];
    
    // Effective viscosity = weighted average
    return 0.6f * elem->kv_viscosity + 0.4f * elem->maxwell_viscosity;
}

// ============================================================================
// HELPER: Compute effective modulus
// Higher = stiffer, Lower = more yielding
// ============================================================================
float UpeViscoelastic_GetEffectiveModulus(const UpeViscoelasticSystem *visco, uint32_t elem_id) {
    if (!visco || elem_id >= visco->element_count) {
        return 0.0f;
    }
    
    const UpeViscoelasticElement *elem = &visco->elements[elem_id];
    
    // Effective modulus = weighted average, reduced by damage
    float modulus = 0.6f * elem->kv_modulus + 0.4f * elem->maxwell_modulus;
    return modulus * (1.0f - elem->damage);
}
