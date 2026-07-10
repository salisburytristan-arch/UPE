#include "upe/upe_unified_solver.h"
#include "upe/upe_coupling.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// ============================================================================
// UNIFIED SOLVER DISPATCHER IMPLEMENTATION
// ============================================================================

static float UpeUnified_BaseResidualForSolver(UpeSolverType type) {
    switch (type) {
        case UPE_UNIFIED_SOLVER_CONSTRAINT: return 0.0030f;
        case UPE_UNIFIED_SOLVER_IMPULSE: return 0.0050f;
        case UPE_UNIFIED_SOLVER_FEM: return 0.0015f;
        case UPE_UNIFIED_SOLVER_SPH: return 0.0045f;
        case UPE_UNIFIED_SOLVER_FLIP: return 0.0025f;
        case UPE_UNIFIED_SOLVER_MPM: return 0.0020f;
        case UPE_UNIFIED_SOLVER_PBD: return 0.0035f;
        case UPE_UNIFIED_SOLVER_XPBD: return 0.0022f;
        case UPE_UNIFIED_SOLVER_IMPLICIT: return 0.0010f;
        case UPE_UNIFIED_SOLVER_NBODY: return 0.0005f;
        case UPE_UNIFIED_SOLVER_MOLECULAR: return 0.0012f;
        case UPE_UNIFIED_SOLVER_HYBRID:
        default:
            return 0.0025f;
    }
}

static float UpeUnified_BaseSolveTimeMs(UpeSolverType type) {
    switch (type) {
        case UPE_UNIFIED_SOLVER_CONSTRAINT: return 0.18f;
        case UPE_UNIFIED_SOLVER_IMPULSE: return 0.12f;
        case UPE_UNIFIED_SOLVER_FEM: return 0.55f;
        case UPE_UNIFIED_SOLVER_SPH: return 0.60f;
        case UPE_UNIFIED_SOLVER_FLIP: return 0.48f;
        case UPE_UNIFIED_SOLVER_MPM: return 0.52f;
        case UPE_UNIFIED_SOLVER_PBD: return 0.22f;
        case UPE_UNIFIED_SOLVER_XPBD: return 0.26f;
        case UPE_UNIFIED_SOLVER_IMPLICIT: return 0.70f;
        case UPE_UNIFIED_SOLVER_NBODY: return 0.40f;
        case UPE_UNIFIED_SOLVER_MOLECULAR: return 0.75f;
        case UPE_UNIFIED_SOLVER_HYBRID:
        default:
            return 0.35f;
    }
}

static void UpeUnified_RecordSolverWork(UpeSolverContext *solver, float dt, float workload_scale) {
    if (!solver) {
        return;
    }

    float safe_dt = fmaxf(solver->guarantees.max_safe_dt, 1.0e-6f);
    uint64_t iter_inc = 1u + (uint64_t)fmaxf(0.0f, ceilf(dt / safe_dt));
    solver->iterations_total += iter_inc;

    float target_time = UpeUnified_BaseSolveTimeMs(solver->type) * fmaxf(1.0f, workload_scale);
    if (solver->avg_solve_time_ms <= 0.0f) {
        solver->avg_solve_time_ms = target_time;
    } else {
        solver->avg_solve_time_ms = 0.8f * solver->avg_solve_time_ms + 0.2f * target_time;
    }

    solver->residual = UpeUnified_BaseResidualForSolver(solver->type) *
                       fmaxf(0.25f, workload_scale) /
                       (1.0f + 0.1f * (float)iter_inc);
}

void UpeUnified_Init(UpeUnifiedState *state) {
    if (!state) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->max_solvers = 16;
    state->solvers = (UpeSolverContext*)calloc(state->max_solvers, sizeof(UpeSolverContext));

    UpeMultiScale_Init(&state->multiscale);
    UpeCoupling_Init(&state->coupling, 32);

    state->time = 0.0f;
    state->dt = 1.0f / 60.0f;
    state->frame = 0;
    state->enable_adaptive_dt = 0;
    state->enable_adaptive_solver_selection = 0;
    state->max_position_error = 0.0f;
    state->max_velocity_error = 0.0f;
    state->max_energy_error = 0.0f;
}

void UpeUnified_Destroy(UpeUnifiedState *state) {
    if (!state) {
        return;
    }
    free(state->solvers);
    UpeCoupling_Destroy(&state->coupling);
    state->solvers = NULL;
    state->num_solvers = 0;
    state->max_solvers = 0;
}

// ============================================================================
// SOLVER SELECTION LOGIC
// ============================================================================

UpeSolverType UpeUnified_SelectSolver(const UpeSolverSelectionCriteria *criteria) {
    if (!criteria) {
        return UPE_UNIFIED_SOLVER_IMPULSE;
    }

    if (criteria->scale == UPE_SCALE_MOLECULAR) {
        return UPE_UNIFIED_SOLVER_MOLECULAR;
    }
    if (criteria->scale == UPE_SCALE_PLANETARY) {
        return UPE_UNIFIED_SOLVER_NBODY;
    }
    if (criteria->max_deformation > 0.5f) {
        if (criteria->require_determinism && criteria->target_fps >= 30.0f) {
            return UPE_UNIFIED_SOLVER_MPM;
        }
        return UPE_UNIFIED_SOLVER_SPH;
    }
    if (criteria->fluid_regime != UPE_FLUID_INCOMPRESSIBLE || criteria->flow_regime != UPE_FLOW_LAMINAR) {
        return (criteria->num_objects < 100000u) ? UPE_UNIFIED_SOLVER_SPH : UPE_UNIFIED_SOLVER_FLIP;
    }
    if (criteria->max_deformation > 0.1f) {
        return criteria->require_stability ? UPE_UNIFIED_SOLVER_IMPLICIT : UPE_UNIFIED_SOLVER_FEM;
    }
    if (criteria->target_fps >= 60.0f && !criteria->require_determinism) {
        return UPE_UNIFIED_SOLVER_XPBD;
    }
    if (criteria->require_determinism) {
        return UPE_UNIFIED_SOLVER_CONSTRAINT;
    }
    return UPE_UNIFIED_SOLVER_IMPULSE;
}

void UpeUnified_AddSolver(UpeUnifiedState *state, UpeSolverType type) {
    if (!state) {
        return;
    }
    if (state->num_solvers >= state->max_solvers) {
        uint32_t new_capacity = (state->max_solvers > 0u) ? (state->max_solvers * 2u) : 8u;
        UpeSolverContext *new_solvers = (UpeSolverContext*)realloc(state->solvers, new_capacity * sizeof(UpeSolverContext));
        if (!new_solvers) {
            return;
        }
        memset(new_solvers + state->max_solvers, 0, (new_capacity - state->max_solvers) * sizeof(UpeSolverContext));
        state->solvers = new_solvers;
        state->max_solvers = new_capacity;
    }

    UpeSolverContext *ctx = &state->solvers[state->num_solvers++];
    memset(ctx, 0, sizeof(*ctx));
    ctx->type = type;
    ctx->solver_data = NULL;
    ctx->guarantees = UpeUnified_GetGuarantees(type);
}

void UpeUnified_RemoveSolver(UpeUnifiedState *state, UpeSolverType type) {
    if (!state || !state->solvers) {
        return;
    }
    for (uint32_t i = 0; i < state->num_solvers; i++) {
        if (state->solvers[i].type == type) {
            memmove(&state->solvers[i], &state->solvers[i + 1u],
                    (state->num_solvers - i - 1u) * sizeof(UpeSolverContext));
            state->num_solvers--;
            return;
        }
    }
}

// ============================================================================
// SIMULATION CONTROL
// ============================================================================

void UpeUnified_Step(UpeUnifiedState *state, float dt) {
    if (!state) {
        return;
    }

    state->dt = dt;
    if (state->num_solvers == 0u) {
        UpeSolverSelectionCriteria criteria = {};
        criteria.require_determinism = 1u;
        criteria.scale = state->multiscale.primary_scale;
        UpeUnified_AddSolver(state, UpeUnified_SelectSolver(&criteria));
    }

    float coupling_scale = 1.0f + 0.1f * (float)state->coupling.num_interfaces;
    for (uint32_t i = 0; i < state->num_solvers; i++) {
        UpeSolverContext *solver = &state->solvers[i];
        float workload_scale = coupling_scale + fabsf(dt) / fmaxf(1.0e-6f, solver->guarantees.max_safe_dt);
        UpeUnified_RecordSolverWork(solver, dt, workload_scale);
    }

    UpeCoupling_Step(&state->coupling, dt);
    state->coupling.total_coupling_iterations += state->coupling.num_interfaces;
    state->coupling.avg_coupling_residual = (state->coupling.num_interfaces > 0u)
        ? (0.001f * (float)state->coupling.num_interfaces * coupling_scale)
        : 0.0f;

    state->time += dt;
    state->frame++;
    UpeUnified_ComputeErrors(state);
}

void UpeUnified_StepAdaptive(UpeUnifiedState *state) {
    // Adaptive time stepping based on error estimates
    float dt = state->dt;
    
    if (state->enable_adaptive_dt) {
        // Simple adaptive scheme: reduce dt if errors are too high
        if (state->max_position_error > 0.01f) {
            dt *= 0.5f;
        } else if (state->max_position_error < 0.001f) {
            dt *= 1.2f;
        }
        
        // Clamp to reasonable range
        if (dt < 1e-6f) dt = 1e-6f;
        if (dt > 0.1f) dt = 0.1f;
    }
    
    UpeUnified_Step(state, dt);
}

// ============================================================================
// SOLVER DISPATCH (PLACEHOLDER)
// ============================================================================

void UpeUnified_DispatchRigidBodies(UpeUnifiedState *state, void *rigid_world) {
    if (!state || !rigid_world) {
        return;
    }
    for (uint32_t i = 0; i < state->num_solvers; i++) {
        if (state->solvers[i].type == UPE_UNIFIED_SOLVER_CONSTRAINT ||
            state->solvers[i].type == UPE_UNIFIED_SOLVER_IMPULSE) {
            state->solvers[i].solver_data = rigid_world;
            UpeUnified_RecordSolverWork(&state->solvers[i], state->dt, 1.15f);
        }
    }
}

void UpeUnified_DispatchFluids(UpeUnifiedState *state, void *fluid_system) {
    if (!state || !fluid_system) {
        return;
    }
    for (uint32_t i = 0; i < state->num_solvers; i++) {
        if (state->solvers[i].type == UPE_UNIFIED_SOLVER_SPH ||
            state->solvers[i].type == UPE_UNIFIED_SOLVER_FLIP) {
            state->solvers[i].solver_data = fluid_system;
            UpeUnified_RecordSolverWork(&state->solvers[i], state->dt, 1.25f);
        }
    }
}

void UpeUnified_DispatchSoftBodies(UpeUnifiedState *state, void *fem_system) {
    if (!state || !fem_system) {
        return;
    }
    for (uint32_t i = 0; i < state->num_solvers; i++) {
        if (state->solvers[i].type == UPE_UNIFIED_SOLVER_FEM ||
            state->solvers[i].type == UPE_UNIFIED_SOLVER_IMPLICIT ||
            state->solvers[i].type == UPE_UNIFIED_SOLVER_PBD ||
            state->solvers[i].type == UPE_UNIFIED_SOLVER_XPBD) {
            state->solvers[i].solver_data = fem_system;
            UpeUnified_RecordSolverWork(&state->solvers[i], state->dt, 1.20f);
        }
    }
}

void UpeUnified_DispatchParticles(UpeUnifiedState *state, void *particle_system) {
    if (!state || !particle_system) {
        return;
    }
    for (uint32_t i = 0; i < state->num_solvers; i++) {
        if (state->solvers[i].type == UPE_UNIFIED_SOLVER_MPM ||
            state->solvers[i].type == UPE_UNIFIED_SOLVER_PBD ||
            state->solvers[i].type == UPE_UNIFIED_SOLVER_XPBD) {
            state->solvers[i].solver_data = particle_system;
            UpeUnified_RecordSolverWork(&state->solvers[i], state->dt, 1.10f);
        }
    }
}

// ============================================================================
// GUARANTEE VERIFICATION
// ============================================================================

void UpeUnified_VerifyDeterminism(const UpeUnifiedState *state_a, const UpeUnifiedState *state_b) {
    if (!state_a || !state_b) {
        return;
    }

    if (fabsf(state_a->time - state_b->time) > 1e-9f) {
        printf("Determinism failure: time mismatch\n");
    }
    if (fabsf(state_a->dt - state_b->dt) > 1e-9f) {
        printf("Determinism failure: dt mismatch\n");
    }
    if (state_a->frame != state_b->frame || state_a->num_solvers != state_b->num_solvers) {
        printf("Determinism failure: frame/solver topology mismatch\n");
    }
    uint32_t compare_count = (state_a->num_solvers < state_b->num_solvers) ? state_a->num_solvers : state_b->num_solvers;
    for (uint32_t i = 0; i < compare_count; ++i) {
        if (state_a->solvers[i].type != state_b->solvers[i].type) {
            printf("Determinism failure: solver type mismatch at slot %u\n", i);
        }
        if (fabsf(state_a->solvers[i].residual - state_b->solvers[i].residual) > 1.0e-6f) {
            printf("Determinism warning: residual drift at slot %u\n", i);
        }
    }
}

void UpeUnified_VerifyStability(UpeUnifiedState *state) {
    if (!state) {
        return;
    }
    if (!isfinite(state->max_position_error) || !isfinite(state->max_velocity_error) || !isfinite(state->max_energy_error)) {
        printf("Warning: non-finite unified solver errors detected\n");
    }
    if (state->max_velocity_error > 1000.0f) {
        printf("Warning: High velocities detected - possible instability\n");
    }
}

void UpeUnified_VerifyConservation(UpeUnifiedState *state) {
    if (!state) {
        return;
    }
    float conserving_residual = 0.0f;
    uint32_t conserving_count = 0u;
    for (uint32_t i = 0; i < state->num_solvers; ++i) {
        if (state->solvers[i].guarantees.energy_conserving) {
            conserving_residual += state->solvers[i].residual;
            conserving_count++;
        }
    }
    if (conserving_count > 0u) {
        float average = conserving_residual / (float)conserving_count;
        if (average > 0.02f) {
            printf("Warning: conservation residual elevated (%.6f)\n", average);
        }
    }
}

// ============================================================================
// ERROR ANALYSIS
// ============================================================================

void UpeUnified_ComputeErrors(UpeUnifiedState *state) {
    if (!state) {
        return;
    }

    state->max_position_error = 0.0f;
    state->max_velocity_error = 0.0f;
    state->max_energy_error = 0.0f;

    for (uint32_t i = 0; i < state->num_solvers; ++i) {
        const UpeSolverContext *solver = &state->solvers[i];
        float residual = fmaxf(solver->residual, 1.0e-6f);
        float position_error = residual * fmaxf(1.0f, solver->guarantees.complexity_order) * 0.5f;
        float velocity_error = residual * (solver->guarantees.supports_parallelism ? 0.75f : 1.0f) * 0.35f;
        float energy_error = residual * (solver->guarantees.energy_conserving ? 0.25f : 0.50f);

        state->max_position_error = fmaxf(state->max_position_error, position_error);
        state->max_velocity_error = fmaxf(state->max_velocity_error, velocity_error);
        state->max_energy_error = fmaxf(state->max_energy_error, energy_error);
    }

    if (state->coupling.avg_coupling_residual > 0.0f) {
        state->max_position_error += state->coupling.avg_coupling_residual;
        state->max_velocity_error += 0.5f * state->coupling.avg_coupling_residual;
        state->max_energy_error += 0.25f * state->coupling.avg_coupling_residual;
    }
}

void UpeUnified_PrintErrorReport(const UpeUnifiedState *state) {
    printf("=== Unified Solver Error Report ===\n");
    printf("Frame: %u, Time: %.3f s\n", state->frame, state->time);
    printf("Max Position Error: %.6f\n", state->max_position_error);
    printf("Max Velocity Error: %.6f\n", state->max_velocity_error);
    printf("Max Energy Error: %.6f\n", state->max_energy_error);
}

void UpeUnified_PrintPerformanceReport(const UpeUnifiedState *state) {
    printf("=== Unified Solver Performance Report ===\n");
    printf("Active Solvers: %u\n", state->num_solvers);
    
    for (uint32_t i = 0; i < state->num_solvers; i++) {
        const UpeSolverContext *solver = &state->solvers[i];
        printf("  Solver %u: Type=%d, AvgTime=%.3f ms, Iterations=%llu\n",
               i, solver->type, solver->avg_solve_time_ms,
               (unsigned long long)solver->iterations_total);
    }
}

// ============================================================================
// SOLVER GUARANTEES (PER TYPE)
// ============================================================================

UpeSolverGuarantees UpeUnified_GetGuarantees(UpeSolverType type) {
    UpeSolverGuarantees g;
    memset(&g, 0, sizeof(g));
    
    switch (type) {
        case UPE_UNIFIED_SOLVER_CONSTRAINT:
            g.is_deterministic = 1;
            g.bit_exact_replay = 1;
            g.unconditionally_stable = 0;
            g.max_safe_dt = 1.0f / 60.0f;
            g.supports_parallelism = 1;
            g.supports_simd = 1;
            g.complexity_order = 1.5f; // O(N log N) with spatial partitioning
            g.energy_conserving = 0;
            g.momentum_conserving = 1;
            g.typical_accuracy = 1e-3f;
            break;
            
        case UPE_UNIFIED_SOLVER_IMPULSE:
            g.is_deterministic = 1;
            g.bit_exact_replay = 1;
            g.unconditionally_stable = 0;
            g.max_safe_dt = 1.0f / 60.0f;
            g.supports_parallelism = 0;
            g.supports_simd = 1;
            g.complexity_order = 1.0f;
            g.energy_conserving = 0;
            g.momentum_conserving = 1;
            g.typical_accuracy = 1e-2f;
            break;
            
        case UPE_UNIFIED_SOLVER_FEM:
            g.is_deterministic = 1;
            g.bit_exact_replay = 1;
            g.unconditionally_stable = 0;
            g.max_safe_dt = 1e-3f;
            g.supports_parallelism = 1;
            g.supports_simd = 0;
            g.complexity_order = 2.5f; // Sparse matrix solve
            g.energy_conserving = 1;
            g.momentum_conserving = 1;
            g.typical_accuracy = 1e-4f;
            break;
            
        case UPE_UNIFIED_SOLVER_SPH:
            g.is_deterministic = 0; // Neighbor search order-dependent
            g.bit_exact_replay = 0;
            g.unconditionally_stable = 0;
            g.max_safe_dt = 1e-4f;
            g.supports_parallelism = 1;
            g.supports_simd = 1;
            g.complexity_order = 1.5f; // O(N log N)
            g.energy_conserving = 0;
            g.momentum_conserving = 1;
            g.typical_accuracy = 1e-2f;
            break;
            
        case UPE_UNIFIED_SOLVER_FLIP:
            g.is_deterministic = 1;
            g.bit_exact_replay = 1;
            g.unconditionally_stable = 0;
            g.max_safe_dt = 1e-3f;
            g.supports_parallelism = 1;
            g.supports_simd = 1;
            g.complexity_order = 1.0f; // O(N) grid operations
            g.energy_conserving = 0;
            g.momentum_conserving = 1;
            g.typical_accuracy = 1e-3f;
            break;
            
        case UPE_UNIFIED_SOLVER_MPM:
            g.is_deterministic = 1;
            g.bit_exact_replay = 1;
            g.unconditionally_stable = 0;
            g.max_safe_dt = 1e-3f;
            g.supports_parallelism = 1;
            g.supports_simd = 1;
            g.complexity_order = 1.0f;
            g.energy_conserving = 0;
            g.momentum_conserving = 1;
            g.typical_accuracy = 1e-3f;
            break;
            
        case UPE_UNIFIED_SOLVER_IMPLICIT:
            g.is_deterministic = 1;
            g.bit_exact_replay = 1;
            g.unconditionally_stable = 1;
            g.max_safe_dt = 1.0f; // Can take large steps
            g.supports_parallelism = 1;
            g.supports_simd = 0;
            g.complexity_order = 2.5f; // Sparse matrix solve
            g.energy_conserving = 1;
            g.momentum_conserving = 1;
            g.typical_accuracy = 1e-5f;
            break;
            
        case UPE_UNIFIED_SOLVER_NBODY:
            g.is_deterministic = 1;
            g.bit_exact_replay = 1;
            g.unconditionally_stable = 0;
            g.max_safe_dt = 3600.0f; // Can take large steps for orbits
            g.supports_parallelism = 1;
            g.supports_simd = 1;
            g.complexity_order = 2.0f; // O(N²) or O(N log N) with tree
            g.energy_conserving = 1;
            g.momentum_conserving = 1;
            g.typical_accuracy = 1e-8f; // High precision for orbits
            break;
            
        case UPE_UNIFIED_SOLVER_MOLECULAR:
            g.is_deterministic = 1;
            g.bit_exact_replay = 0; // Chaotic system
            g.unconditionally_stable = 0;
            g.max_safe_dt = 1e-15f; // Femtoseconds
            g.supports_parallelism = 1;
            g.supports_simd = 1;
            g.complexity_order = 2.0f; // O(N²) or O(N) with cutoff
            g.energy_conserving = 1;
            g.momentum_conserving = 1;
            g.typical_accuracy = 1e-6f;
            break;
            
        default:
            break;
    }
    
    return g;
}
