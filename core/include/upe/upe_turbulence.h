#ifndef UPE_TURBULENCE_H
#define UPE_TURBULENCE_H

#include "upe_math.h"
#include <stdint.h>

// ============================================================================
// TURBULENCE MODELING
//
// Handles turbulent flow regimes when Reynolds number > ~4000
//
// Approaches:
// - RANS (Reynolds-Averaged Navier-Stokes): Time-averaged
// - LES (Large Eddy Simulation): Resolve large eddies, model small
// - DNS (Direct Numerical Simulation): Resolve all scales (expensive)
// - Vorticity confinement: Add energy to small vortices
//
// Models:
// - k-ε (Industrial standard, 2 equations)
// - k-ω (Better near walls)
// - Smagorinsky (LES subgrid model)
// ============================================================================

typedef enum UpeTurbulenceModel {
    UPE_TURB_NONE = 0,           // Laminar flow
    UPE_TURB_VORTICITY = 1,      // Simple vorticity confinement
    UPE_TURB_K_EPSILON = 2,      // k-ε RANS model
    UPE_TURB_K_OMEGA = 3,        // k-ω RANS model
    UPE_TURB_SMAGORINSKY = 4,    // LES subgrid
    UPE_TURB_DNS = 5             // Direct simulation (very expensive)
} UpeTurbulenceModel;

// k-ε turbulence model state
typedef struct UpeKEpsilonState {
    float *k;           // Turbulent kinetic energy (per cell)
    float *epsilon;     // Dissipation rate (per cell)
    uint32_t grid_size;
    
    // Model constants
    float C_mu;         // 0.09 (standard)
    float C_epsilon1;   // 1.44
    float C_epsilon2;   // 1.92
    float sigma_k;      // 1.0
    float sigma_epsilon;// 1.3
} UpeKEpsilonState;

// LES subgrid scale model
typedef struct UpeSmagorinskyLES {
    float C_s;              // Smagorinsky constant ~0.1-0.2
    float filter_width;     // Grid spacing for LES filter
    float *nu_t;            // Eddy viscosity field
    uint32_t grid_size;
} UpeSmagorinskyLES;

// Turbulence statistics
typedef struct UpeTurbulenceStats {
    float reynolds_number;
    float turbulent_intensity;    // u'/U (fluctuation / mean)
    float integral_length_scale;  // Size of large eddies
    float taylor_microscale;      // Size of energy dissipation
    float kolmogorov_scale;       // Smallest resolved scale
    float energy_cascade_rate;
} UpeTurbulenceStats;

// Initialization
void UpeTurbulence_InitKEpsilon(UpeKEpsilonState *ke, uint32_t grid_size);
void UpeTurbulence_InitSmagorinsky(UpeSmagorinskyLES *les, float filter_width, uint32_t grid_size);
void UpeTurbulence_Destroy(void *turbulence_state);

// k-ε model step
void UpeTurbulence_StepKEpsilon(
    UpeKEpsilonState *ke,
    const float *velocity_u,
    const float *velocity_v,
    const float *velocity_w,
    uint32_t width, uint32_t height, uint32_t depth,
    float dt);

// LES Smagorinsky model
void UpeTurbulence_ComputeSmagorinskyViscosity(
    UpeSmagorinskyLES *les,
    const float *velocity_u,
    const float *velocity_v,
    const float *velocity_w,
    uint32_t width, uint32_t height, uint32_t depth);

// Vorticity confinement (simple turbulence injection)
void UpeTurbulence_VorticityConfinement(
    float *velocity_u,
    float *velocity_v,
    float *velocity_w,
    uint32_t width, uint32_t height, uint32_t depth,
    float confinement_strength,
    float dt);

// Statistics
void UpeTurbulence_ComputeStats(
    const float *velocity_field,
    uint32_t field_size,
    float characteristic_length,
    float kinematic_viscosity,
    UpeTurbulenceStats *stats);

// Regime detection
uint32_t UpeTurbulence_IsFlowTurbulent(float reynolds_number);
UpeTurbulenceModel UpeTurbulence_SelectModel(float reynolds_number, float computational_budget);

#endif // UPE_TURBULENCE_H
