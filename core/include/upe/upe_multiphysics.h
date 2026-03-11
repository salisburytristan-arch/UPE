#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Multiphysics Coupling
  
  - Fluid-Rigid interaction (two-way coupling)
  - Thermo-mechanical coupling (thermal expansion, thermal stress)
  - Combustion-Fluid coupling (fire, explosions)
  - Electromagnetic-Rigid coupling (motors, inductance drag)
  - Acoustic-Geometry coupling (impact sounds, resonance)
*/

// Forward declarations
typedef struct UpeRigidBody UpeRigidBody;
typedef struct UpeSphFluid UpeSphFluid;
typedef struct UpeElectricCharge UpeElectricCharge;
typedef struct UpeMagneticDipole UpeMagneticDipole;

// ========================
// Fluid-Rigid Coupling
// ========================

typedef struct UpeFluidRigidCoupling {
    UpeSphFluid *fluid;
    UpeRigidBody **bodies;
    uint32_t body_count;
    
    float density_threshold;     // Fluid density to apply forces
    float drag_coefficient;
    float buoyancy_factor;
    
} UpeFluidRigidCoupling;

void UpeCouple_FluidRigid_Init(UpeFluidRigidCoupling *coupling, UpeSphFluid *fluid);
void UpeCouple_FluidRigid_AddBody(UpeFluidRigidCoupling *coupling, UpeRigidBody *body);
void UpeCouple_FluidRigid_Step(UpeFluidRigidCoupling *coupling, float dt);

// Apply fluid forces to rigid bodies (drag, buoyancy)
void UpeCouple_ApplyFluidForces(UpeRigidBody *body, const UpeSphFluid *fluid);

// Apply rigid body influence on fluid (splashing, displacement)
void UpeCouple_ApplyBodyDisplacement(UpeSphFluid *fluid, const UpeRigidBody *body);

// ========================
// Thermo-Mechanical Coupling
// ========================

typedef struct UpeThermoMechanicalCoupling {
    float *temperature_field;    // Temperature grid
    UpeRigidBody **bodies;
    uint32_t body_count;
    
    float *thermal_expansion;    // Per-body coefficients
    float *heat_capacity;
    float *thermal_conductivity;
    
    uint32_t grid_width;
    uint32_t grid_height;
    uint32_t grid_depth;
    float cell_size;
    
} UpeThermoMechanicalCoupling;

void UpeCouple_ThermoMech_Init(UpeThermoMechanicalCoupling *coupling, uint32_t grid_res);
void UpeCouple_ThermoMech_AddBody(UpeThermoMechanicalCoupling *coupling, UpeRigidBody *body, float expansion_coeff);
void UpeCouple_ThermoMech_Step(UpeThermoMechanicalCoupling *coupling, float dt);

// Thermal expansion (change body volume/stress due to temperature)
void UpeCouple_ApplyThermalExpansion(UpeRigidBody *body, float delta_temperature, float expansion_coeff);

// Heat generation from friction/deformation
float UpeCouple_ComputeFrictionalHeat(const UpeRigidBody *body, float friction_work);

// ========================
// Combustion-Fluid Coupling
// ========================

typedef struct UpeCombustionFluidCoupling {
    UpeSphFluid *fluid;
    float *fuel_concentration;   // Fuel concentration field
    float *temperature_field;
    float *oxygen_field;
    
    float ignition_temperature;
    float burn_rate;
    float heat_release;          // J/kg (energy per kg fuel)
    float expansion_factor;      // Volume expansion from combustion
    
    uint32_t grid_width;
    uint32_t grid_height;
    uint32_t grid_depth;
    
} UpeCombustionFluidCoupling;

void UpeCouple_Combustion_Init(UpeCombustionFluidCoupling *coupling, UpeSphFluid *fluid, uint32_t grid_res);
void UpeCouple_Combustion_Step(UpeCombustionFluidCoupling *coupling, float dt);

// Ignite fuel at position (spark, explosion trigger)
void UpeCouple_Ignite(UpeCombustionFluidCoupling *coupling, UpeVec3 position, float radius);

// Compute combustion reaction and add pressure/heat to fluid
void UpeCouple_BurnFuel(UpeCombustionFluidCoupling *coupling, float dt);

// ========================
// Electromagnetic-Rigid Coupling
// ========================

typedef struct UpeElectromagneticRigidCoupling {
    UpeRigidBody **bodies;
    UpeElectricCharge **charges;
    UpeMagneticDipole **magnets;
    uint32_t body_count;
    uint32_t charge_count;
    uint32_t magnet_count;
    
    float *body_conductivity;    // Electrical conductivity per body
    
} UpeElectromagneticRigidCoupling;

void UpeCouple_EMRigid_Init(UpeElectromagneticRigidCoupling *coupling);
void UpeCouple_EMRigid_AddBody(UpeElectromagneticRigidCoupling *coupling, UpeRigidBody *body, float conductivity);
void UpeCouple_EMRigid_Step(UpeElectromagneticRigidCoupling *coupling, float dt);

// Lorentz force on moving charged body
UpeVec3 UpeCouple_LorentzForce(const UpeRigidBody *body, UpeVec3 charge_velocity, UpeVec3 magnetic_field);

// Magnetic torque on dipole body
UpeVec3 UpeCouple_MagneticTorque(const UpeRigidBody *body, UpeVec3 dipole_moment, UpeVec3 external_field);

// Eddy current damping (conductor in magnetic field)
UpeVec3 UpeCouple_EddyCurrentDamping(const UpeRigidBody *body, UpeVec3 magnetic_field, float conductivity);

// ========================
// Acoustic-Geometry Coupling
// ========================

typedef struct UpeAcousticGeometryCoupling {
    UpeRigidBody **bodies;
    uint32_t body_count;
    
    // Impact sound generation
    float *impact_sounds;        // Audio buffer
    uint32_t sound_buffer_size;
    uint32_t sample_rate;
    
    // Modal synthesis parameters (for resonance)
    float *modal_frequencies;    // Per-body resonant frequencies
    float *modal_damping;
    uint32_t modal_count;
    
} UpeAcousticGeometryCoupling;

void UpeCouple_Acoustic_Init(UpeAcousticGeometryCoupling *coupling, uint32_t sample_rate);
void UpeCouple_Acoustic_AddBody(UpeAcousticGeometryCoupling *coupling, UpeRigidBody *body);
void UpeCouple_Acoustic_Step(UpeAcousticGeometryCoupling *coupling, float dt);

// Generate impact sound on collision
void UpeCouple_GenerateImpactSound(UpeAcousticGeometryCoupling *coupling,
                                   UpeVec3 impact_position, float impact_force);

// Modal synthesis (excite resonant modes)
void UpeCouple_ExciteModalSynthesis(UpeAcousticGeometryCoupling *coupling, uint32_t body_id, float impulse);

#ifdef __cplusplus
}
#endif
