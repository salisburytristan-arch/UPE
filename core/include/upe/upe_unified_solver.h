#ifndef UPE_UNIFIED_SOLVER_H
#define UPE_UNIFIED_SOLVER_H

#include "upe_math.h"
#include "upe_multiscale.h"
#include "upe_coupling.h"
#include <stdint.h>

// ============================================================================
// UNIFIED SOLVER DISPATCHER
//
// Central orchestrator that:
// 1. Determines which solver(s) to use based on simulation state
// 2. Routes physics to appropriate specialized solvers
// 3. Manages coupling between domains
// 4. Provides consistent guarantees across all systems
//
// Philosophy:
// - NOT a single monolithic solver (industry-impossible)
// - BUT a smart dispatcher with consistent interface
// - Specialized solvers for specialized tasks
// - Well-defined coupling interfaces
// - Consistent error handling and stability guarantees
// ============================================================================

typedef enum UpeSolverType {
    UPE_SOLVER_CONSTRAINT = 0,    // PGS/SI for rigid bodies
    UPE_SOLVER_IMPULSE = 1,       // Impulse-based for fast collisions
    UPE_SOLVER_FEM = 2,           // Co-rotational FEM for soft bodies
    UPE_SOLVER_SPH = 3,           // Smoothed Particle Hydrodynamics
    UPE_SOLVER_FLIP = 4,          // Grid-based fluids
    UPE_SOLVER_MPM = 5,           // Material Point Method
    UPE_SOLVER_PBD = 6,           // Position-Based Dynamics
    UPE_SOLVER_XPBD = 7,          // Extended PBD
    UPE_SOLVER_IMPLICIT = 8,      // Backward Euler (scientific mode)
    UPE_SOLVER_NBODY = 9,         // N-body gravity (planetary)
    UPE_SOLVER_MOLECULAR = 10,    // Molecular dynamics
    UPE_SOLVER_HYBRID = 11        // Multiple solvers active
} UpeSolverType;

// Guarantees that solvers must provide
typedef struct UpeSolverGuarantees {
    // Determinism
    uint32_t is_deterministic;
    uint32_t bit_exact_replay;
    
    // Stability
    uint32_t unconditionally_stable;
    float max_safe_dt;
    
    // Performance
    uint32_t supports_parallelism;
    uint32_t supports_simd;
    float complexity_order;  // e.g., 1.0 = O(N), 2.0 = O(N²)
    
    // Accuracy
    uint32_t energy_conserving;
    uint32_t momentum_conserving;
    float typical_accuracy;  // Relative error
} UpeSolverGuarantees;

// Solver context (encapsulates solver state)
typedef struct UpeSolverContext {
    UpeSolverType type;
    void *solver_data;
    UpeSolverGuarantees guarantees;
    
    // Performance tracking
    uint64_t iterations_total;
    float avg_solve_time_ms;
    float residual;
} UpeSolverContext;

// Unified simulation state
typedef struct UpeUnifiedState {
    // Active solvers
    UpeSolverContext *solvers;
    uint32_t num_solvers;
    uint32_t max_solvers;
    
    // Multi-scale config
    UpeMultiScaleConfig multiscale;
    
    // Coupling coordinator
    UpeCouplingCoordinator coupling;
    
    // Global time
    float time;
    float dt;
    uint32_t frame;
    
    // Adaptive control
    uint32_t enable_adaptive_dt;
    uint32_t enable_adaptive_solver_selection;
    
    // Error tracking
    float max_position_error;
    float max_velocity_error;
    float max_energy_error;
} UpeUnifiedState;

// Solver selection criteria
typedef struct UpeSolverSelectionCriteria {
    // Problem characteristics
    uint32_t num_objects;
    uint32_t num_contacts;
    float max_velocity;
    float max_deformation;
    
    // User requirements
    uint32_t require_determinism;
    uint32_t require_stability;
    float target_fps;
    float accuracy_tolerance;
    
    // Physics regime
    UpePhysicsScale scale;
    UpeFluidRegime fluid_regime;
    UpeFlowRegime flow_regime;
} UpeSolverSelectionCriteria;

// Initialization
void UpeUnified_Init(UpeUnifiedState *state);
void UpeUnified_Destroy(UpeUnifiedState *state);

// Solver management
UpeSolverType UpeUnified_SelectSolver(const UpeSolverSelectionCriteria *criteria);
void UpeUnified_AddSolver(UpeUnifiedState *state, UpeSolverType type);
void UpeUnified_RemoveSolver(UpeUnifiedState *state, UpeSolverType type);

// Simulation control
void UpeUnified_Step(UpeUnifiedState *state, float dt);
void UpeUnified_StepAdaptive(UpeUnifiedState *state); // Auto-determine dt

// Solver dispatch
void UpeUnified_DispatchRigidBodies(UpeUnifiedState *state, void *rigid_world);
void UpeUnified_DispatchFluids(UpeUnifiedState *state, void *fluid_system);
void UpeUnified_DispatchSoftBodies(UpeUnifiedState *state, void *fem_system);
void UpeUnified_DispatchParticles(UpeUnifiedState *state, void *particle_system);

// Guarantee verification
void UpeUnified_VerifyDeterminism(const UpeUnifiedState *state_a, const UpeUnifiedState *state_b);
void UpeUnified_VerifyStability(UpeUnifiedState *state);
void UpeUnified_VerifyConservation(UpeUnifiedState *state);

// Error analysis
void UpeUnified_ComputeErrors(UpeUnifiedState *state);
void UpeUnified_PrintErrorReport(const UpeUnifiedState *state);

// Performance analysis
void UpeUnified_PrintPerformanceReport(const UpeUnifiedState *state);

// Solver guarantees (per solver type)
UpeSolverGuarantees UpeUnified_GetGuarantees(UpeSolverType type);

#endif // UPE_UNIFIED_SOLVER_H
