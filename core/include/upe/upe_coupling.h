#ifndef UPE_COUPLING_H
#define UPE_COUPLING_H

#include "upe_math.h"
#include <stdint.h>

// ============================================================================
// MULTI-PHYSICS COUPLING FRAMEWORK
//
// Integrates different physics solvers:
// - Rigid bodies ↔ Fluids (two-way coupling)
// - Fluids ↔ Deformables (FSI - Fluid-Structure Interaction)
// - Particles ↔ Grids (PIC/FLIP/MPM)
// - EM fields ↔ Fluids (MHD - Magnetohydrodynamics)
// - Thermal ↔ Everything (temperature affects all properties)
//
// Coupling strategies:
// - Explicit: Fast, conditionally stable
// - Semi-implicit: Balanced, mostly stable
// - Fully implicit: Slow, unconditionally stable
// ============================================================================

typedef enum UpeCouplingStrategy {
    UPE_COUPLING_EXPLICIT = 0,       // Simple but can be unstable
    UPE_COUPLING_SEMI_IMPLICIT = 1,  // Balanced approach
    UPE_COUPLING_IMPLICIT = 2        // Most stable, most expensive
} UpeCouplingStrategy;

typedef enum UpePhysicsDomain {
    UPE_DOMAIN_RIGID = 0,
    UPE_DOMAIN_SOFT = 1,
    UPE_DOMAIN_FLUID = 2,
    UPE_DOMAIN_THERMAL = 3,
    UPE_DOMAIN_EM = 4,
    UPE_DOMAIN_ACOUSTIC = 5,
    UPE_DOMAIN_GRANULAR = 6,
    UPE_DOMAIN_PARTICLE = 7
} UpePhysicsDomain;

// Coupling interface between two domains
typedef struct UpeCouplingInterface {
    UpePhysicsDomain domain_a;
    UpePhysicsDomain domain_b;
    UpeCouplingStrategy strategy;
    
    // Transfer functions
    void (*transfer_a_to_b)(const void *a_data, void *b_data);
    void (*transfer_b_to_a)(const void *b_data, void *a_data);
    
    // Coupling strength (0.0 = independent, 1.0 = fully coupled)
    float coupling_strength;
    
    // Iteration count for implicit coupling
    uint32_t max_iterations;
    float tolerance;
} UpeCouplingInterface;

// Global coupling coordinator
typedef struct UpeCouplingCoordinator {
    UpeCouplingInterface *interfaces;
    uint32_t num_interfaces;
    uint32_t max_interfaces;
    
    // Execution order (topological sort of dependency graph)
    UpePhysicsDomain *execution_order;
    uint32_t num_domains_active;
    
    // Performance tracking
    uint64_t total_coupling_iterations;
    float avg_coupling_residual;
} UpeCouplingCoordinator;

// Rigid-Fluid coupling (two-way)
typedef struct UpeRigidFluidCoupling {
    // Rigid → Fluid: Forces from bodies to fluid
    void *rigid_world;
    void *fluid_system;
    
    // Fluid → Rigid: Pressure/viscous forces from fluid to bodies
    float drag_coefficient;
    float added_mass_coefficient;
    
    // Coupling region
    float interaction_radius;
    uint32_t use_pressure_forces;
    uint32_t use_viscous_forces;
} UpeRigidFluidCoupling;

// FSI (Fluid-Structure Interaction)
typedef struct UpeFSICoupling {
    void *fluid_system;
    void *structure_fem;
    
    UpeCouplingStrategy strategy;
    
    // Interface mesh
    UpeVec3 *interface_positions;
    UpeVec3 *interface_normals;
    uint32_t num_interface_nodes;
    
    // Forces
    float *pressure_on_structure;
    float *structure_velocity;
} UpeFSICoupling;

// MHD (Magnetohydrodynamics) - EM + Fluid coupling
typedef struct UpeMHDCoupling {
    void *fluid_system;
    void *em_system;
    
    // Lorentz force J×B affects fluid
    float electrical_conductivity;
    
    // Fluid velocity affects B field (magnetic induction)
    uint32_t enable_induction;
} UpeMHDCoupling;

// Thermal coupling (temperature affects all domains)
typedef struct UpeThermalCoupling {
    void *thermal_system;
    
    // Temperature → Material properties
    float (*viscosity_from_temp)(float T);
    float (*density_from_temp)(float T);
    float (*youngs_modulus_from_temp)(float T);
    
    // Heat sources from other domains
    float friction_heat_coefficient;
    float compression_heat_coefficient;
    float em_heat_coefficient;
} UpeThermalCoupling;

// Initialization
void UpeCoupling_Init(UpeCouplingCoordinator *coordinator, uint32_t max_interfaces);
void UpeCoupling_Destroy(UpeCouplingCoordinator *coordinator);

// Interface management
void UpeCoupling_AddInterface(
    UpeCouplingCoordinator *coordinator,
    UpePhysicsDomain domain_a,
    UpePhysicsDomain domain_b,
    UpeCouplingStrategy strategy);

// Coupling execution
void UpeCoupling_Step(UpeCouplingCoordinator *coordinator, float dt);
void UpeCoupling_BuildExecutionOrder(UpeCouplingCoordinator *coordinator);

// Specific coupling implementations
void UpeCoupling_RigidFluid_Init(UpeRigidFluidCoupling *coupling);
void UpeCoupling_RigidFluid_Transfer(UpeRigidFluidCoupling *coupling, float dt);

void UpeCoupling_FSI_Init(UpeFSICoupling *fsi);
void UpeCoupling_FSI_Step(UpeFSICoupling *fsi, float dt);

void UpeCoupling_MHD_Init(UpeMHDCoupling *mhd);
void UpeCoupling_MHD_ApplyLorentzForce(UpeMHDCoupling *mhd);

void UpeCoupling_Thermal_Init(UpeThermalCoupling *thermal);
void UpeCoupling_Thermal_UpdateProperties(UpeThermalCoupling *thermal);

#endif // UPE_COUPLING_H
