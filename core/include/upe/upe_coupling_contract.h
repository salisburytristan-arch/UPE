#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Multiphysics Coupling Contract (1.0.0.0)
  
  Define strict "who writes what, when" rules so fluids/cloth/thermal/EM can't fight.
  
  Include:
  - Force aggregation order
  - State access permissions  
  - Deterministic reductions
  - Coupling declarations per module
*/

// Domain IDs (bitfield-friendly)
#define UPE_DOMAIN_RIGID         (1 << 0)
#define UPE_DOMAIN_CLOTH         (1 << 1)
#define UPE_DOMAIN_FLUID_SPH     (1 << 2)
#define UPE_DOMAIN_FLUID_FLIP    (1 << 3)
#define UPE_DOMAIN_THERMAL       (1 << 4)
#define UPE_DOMAIN_EM            (1 << 5)
#define UPE_DOMAIN_ACOUSTIC      (1 << 6)
#define UPE_DOMAIN_CHEMISTRY     (1 << 7)
#define UPE_DOMAIN_GRANULAR      (1 << 8)
#define UPE_DOMAIN_GAS           (1 << 9)
#define UPE_DOMAIN_VEGETATION    (1 << 10)
#define UPE_DOMAIN_FRACTURE      (1 << 11)
#define UPE_DOMAIN_FIRE          (1 << 12)
#define UPE_DOMAIN_WIND          (1 << 13)
#define UPE_DOMAIN_EROSION       (1 << 14)
#define UPE_DOMAIN_CROWD         (1 << 15)

// Access modes
typedef enum UpeCouplingAccess {
    UPE_ACCESS_NONE = 0,
    UPE_ACCESS_READ = 1,
    UPE_ACCESS_WRITE = 2,
    UPE_ACCESS_READ_WRITE = 3
} UpeCouplingAccess;

// Coupling stage (when interaction happens)
typedef enum UpeCouplingStage {
    UPE_STAGE_PRE_STEP = 0,      // Before any domain updates
    UPE_STAGE_FORCES = 1,        // During force accumulation
    UPE_STAGE_CONSTRAINTS = 2,   // During constraint solve
    UPE_STAGE_POST_STEP = 3,     // After all updates
    UPE_STAGE_QUERY = 4          // Read-only queries (anytime)
} UpeCouplingStage;

// Force reduction mode
typedef enum UpeForceReduction {
    UPE_REDUCE_SUM = 0,          // Add all forces
   UPE_REDUCE_MAX = 1,          // Use max magnitude
    UPE_REDUCE_WEIGHTED = 2,     // Weighted average
    UPE_REDUCE_CUSTOM = 3
} UpeForceReduction;

// Coupling declaration (what a domain needs from/gives to others)
typedef struct UpeCouplingDeclaration {
    uint32_t domain_id;          // Which domain is declaring
    
    // What fields this domain reads (bitfield of domain IDs)
    uint32_t reads_from_domains;
    
    // What fields this domain writes (bitfield of domain IDs)  
    uint32_t writes_to_domains;
    
    // What fields this domain reads (field names)
    const char **read_fields;
    uint32_t read_field_count;
    
    // What fields this domain writes
    const char **write_fields;
    uint32_t write_field_count;
    
    // When this domain operates
    UpeCouplingStage stage;
    
    // Execution order (lower = earlier, for determinism)
    uint32_t priority;
    
    // Force reduction mode (if multiple domains write forces)
    UpeForceReduction force_reduction;
    
} UpeCouplingDeclaration;

// Coupling contract (validated set of declarations)
typedef struct UpeCouplingContract {
    UpeCouplingDeclaration *declarations;
    uint32_t declaration_count;
    uint32_t declaration_capacity;
    
    // Validated execution order (sorted by stage, then priority)
    uint32_t *execution_order;   // Indices into declarations
    
    // Conflict detection results
    int has_conflicts;
    char conflict_message[256];
    
} UpeCouplingContract;

// Force aggregation buffer (for deterministic multi-writer)
typedef struct UpeForceAggregator {
    UpeVec3 *body_forces;        // Per rigid body
    UpeVec3 *body_torques;
    uint32_t body_count;
    
    float *particle_forces_x;    // Per particle (for cloth/fluids)
    float *particle_forces_y;
    float *particle_forces_z;
    uint32_t particle_count;
    
    // Writer tracking (which domains contributed)
    uint32_t *body_writers;      // Bitfield per body
    uint32_t *particle_writers;  // Bitfield per particle
    
    UpeForceReduction reduction_mode;
    
} UpeForceAggregator;

// State snapshot (for rollback-safe coupling)
typedef struct UpeCouplingSnapshot {
    void *rigid_state;
    void *cloth_state;
    void *fluid_state;
    void *thermal_state;
    // ... etc
    
    uint64_t state_hash;         // Deterministic verification
    uint32_t frame_number;
    
} UpeCouplingSnapshot;

// Initialization
void UpeCoupling_InitContract(UpeCouplingContract *contract);
void UpeCoupling_DestroyContract(UpeCouplingContract *contract);

// Declaration registration
int UpeCoupling_RegisterDomain(UpeCouplingContract *contract, const UpeCouplingDeclaration *decl);

// Contract validation
int UpeCoupling_Validate(UpeCouplingContract *contract);
int UpeCoupling_CheckConflicts(UpeCouplingContract *contract);
void UpeCoupling_PrintContract(const UpeCouplingContract *contract);

// Execution order computation
void UpeCoupling_ComputeExecutionOrder(UpeCouplingContract *contract);

// Force aggregation
void UpeForce_InitAggregator(UpeForceAggregator *agg, uint32_t body_count, uint32_t particle_count);
void UpeForce_DestroyAggregator(UpeForceAggregator *agg);

void UpeForce_AddBodyForce(UpeForceAggregator *agg, uint32_t body_id, UpeVec3 force, uint32_t writer_domain);
void UpeForce_AddBodyTorque(UpeForceAggregator *agg, uint32_t body_id, UpeVec3 torque, uint32_t writer_domain);
void UpeForce_AddParticleForce(UpeForceAggregator *agg, uint32_t particle_id, UpeVec3 force, uint32_t writer_domain);

void UpeForce_Reduce(UpeForceAggregator *agg);      // Apply reduction mode
void UpeForce_Clear(UpeForceAggregator *agg);       // Reset for next step

// State snapshots (for rollback)
void UpeCoupling_TakeSnapshot(UpeCouplingSnapshot *snap);
void UpeCoupling_RestoreSnapshot(const UpeCouplingSnapshot *snap);
int UpeCoupling_VerifySnapshot(const UpeCouplingSnapshot *snap);

// Common coupling patterns (helpers)

// Fluid → Rigid: Buoyancy + drag
void UpeCoupling_FluidToRigid_Buoyancy(UpeForceAggregator *agg, uint32_t body_id, 
                                       float fluid_density, float body_volume, UpeVec3 gravity);
void UpeCoupling_FluidToRigid_Drag(UpeForceAggregator *agg, uint32_t body_id,
                                   UpeVec3 body_velocity, UpeVec3 fluid_velocity, 
                                   float drag_coeff, float cross_section);

// Rigid → Fluid: Displacement
void UpeCoupling_RigidToFluid_Displacement(uint32_t body_id, float *fluid_density_field,
                                           UpeVec3 body_pos, float body_radius);

// Thermal → Rigid: Thermal expansion force
void UpeCoupling_ThermalToRigid_Expansion(UpeForceAggregator *agg, uint32_t body_id,
                                          float temperature_delta, float expansion_coeff, float stiffness);

// Fluid → Thermal: Convection heat transfer
void UpeCoupling_FluidToThermal_Convection(float *temperature_field, const UpeVec3 *velocity_field,
                                           uint32_t grid_res, float dt);

// EM → Rigid: Lorentz force
void UpeCoupling_EMToRigid_Lorentz(UpeForceAggregator *agg, uint32_t body_id,
                                   float charge, UpeVec3 velocity, UpeVec3 magnetic_field);

#ifdef __cplusplus
}
#endif
