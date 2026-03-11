#ifndef UPE_MULTISCALE_H
#define UPE_MULTISCALE_H

#include "upe_math.h"
#include <stdint.h>

// ============================================================================
// MULTI-SCALE PHYSICS FRAMEWORK
//
// Handles physics across scales:
// - Molecular (10^-9 m) - MD simulation
// - Micro (10^-6 m) - Particle systems, SPH
// - Meso (10^-3 m) - Rigid bodies, soft bodies
// - Macro (1-1000 m) - Vehicles, characters, large structures
// - Planetary (10^6 m+) - Orbital mechanics, N-body
//
// Challenge: Each scale has different:
// - Time steps (femtoseconds → days)
// - Space discretization (angstroms → kilometers)
// - Dominant forces (intermolecular → gravitational)
// ============================================================================

typedef enum UpePhysicsScale {
    UPE_SCALE_MOLECULAR = 0,   // 10^-9 m (molecules, atoms)
    UPE_SCALE_MICRO = 1,        // 10^-6 m (particles, droplets)
    UPE_SCALE_MESO = 2,         // 10^-3 m (millimeters, small objects)
    UPE_SCALE_MACRO = 3,        // 1-1000 m (human scale)
    UPE_SCALE_PLANETARY = 4     // 10^6+ m (orbits, planets)
} UpePhysicsScale;

typedef enum UpeFluidRegime {
    UPE_FLUID_INCOMPRESSIBLE = 0,
    UPE_FLUID_WEAKLY_COMPRESSIBLE = 1,
    UPE_FLUID_COMPRESSIBLE = 2,
    UPE_FLUID_SUPERSONIC = 3
} UpeFluidRegime;

typedef enum UpeFlowRegime {
    UPE_FLOW_LAMINAR = 0,
    UPE_FLOW_TRANSITIONAL = 1,
    UPE_FLOW_TURBULENT = 2
} UpeFlowRegime;

// Multi-scale coordination
typedef struct UpeMultiScaleConfig {
    UpePhysicsScale primary_scale;
    float scale_coupling_radius;  // Distance at which to couple scales
    uint32_t enable_scale_bridging;
    
    // Time stepping across scales
    float molecular_dt;     // 10^-15 s (femtoseconds)
    float micro_dt;         // 10^-6 s (microseconds)
    float meso_dt;          // 10^-3 s (milliseconds)
    float macro_dt;         // 0.016 s (60 Hz)
    float planetary_dt;     // 1-3600 s (seconds to hours)
    
    // Force cutoffs
    float molecular_cutoff;   // van der Waals cutoff
    float micro_cutoff;       // Short-range forces
    float macro_cutoff;       // Contact forces
    float gravity_cutoff;     // Long-range gravity
} UpeMultiScaleConfig;

// Scale bridging (connect different scale simulations)
typedef struct UpeScaleBridge {
    UpePhysicsScale scale_from;
    UpePhysicsScale scale_to;
    
    // Coarse-graining (fine → coarse)
    void (*coarsen_func)(const void *fine_data, void *coarse_data);
    
    // Refinement (coarse → fine)
    void (*refine_func)(const void *coarse_data, void *fine_data);
    
    // Homogenization (average fine properties to coarse)
    void (*homogenize_func)(const void *fine_region, float *coarse_properties);
} UpeScaleBridge;

// Molecular dynamics (simplified, for coupling to macro scale)
typedef struct UpeMolecularSystem {
    UpeVec3 *positions;
    UpeVec3 *velocities;
    float *masses;
    uint32_t num_atoms;
    
    // Lennard-Jones parameters
    float epsilon;  // Energy well depth
    float sigma;    // Collision diameter
    float cutoff;
    
    // Statistical properties (for coarse-graining)
    float temperature;
    float pressure;
    float density;
} UpeMolecularSystem;

// Initialization
void UpeMultiScale_Init(UpeMultiScaleConfig *config);
void UpeMultiScale_SetScale(UpeMultiScaleConfig *config, UpePhysicsScale scale);

// Scale bridging
void UpeMultiScale_CreateBridge(
    UpeScaleBridge *bridge,
    UpePhysicsScale from,
    UpePhysicsScale to);
void UpeMultiScale_CoarsenMolecularToMacro(
    const UpeMolecularSystem *molecular,
    float *macro_density,
    float *macro_pressure,
    float *macro_temperature);

// Molecular dynamics (basic)
void UpeMD_Init(UpeMolecularSystem *md, uint32_t num_atoms);
void UpeMD_Step(UpeMolecularSystem *md, float dt);
void UpeMD_ComputeLennardJones(UpeMolecularSystem *md);
void UpeMD_Destroy(UpeMolecularSystem *md);

// Regime detection
UpeFluidRegime UpeMultiScale_DetectFluidRegime(float mach_number);
UpeFlowRegime UpeMultiScale_DetectFlowRegime(float reynolds_number);

// Adaptive physics (switch solvers based on conditions)
typedef struct UpeAdaptivePhysics {
    UpeFluidRegime current_fluid_regime;
    UpeFlowRegime current_flow_regime;
    uint32_t use_compressibility;
    uint32_t use_turbulence_model;
} UpeAdaptivePhysics;

void UpeAdaptive_Update(UpeAdaptivePhysics *adaptive, float mach, float reynolds);

#endif // UPE_MULTISCALE_H
