#pragma once

#include <stdint.h>
#include "upe_math.h"
#include "upe_body.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Parallel Constraint Solver
 * 
 * Uses color-coding (graph coloring) to identify constraint groups that can be solved
 * in parallel without data races. Multiple threads solve independent groups simultaneously.
 * 
 * Benefits:
 * - Linear scaling with thread count (up to core count)
 * - No synchronization overhead between independent constraints
 * - Better cache locality per thread
 * - Minimal memory bandwidth contention
 */

typedef struct UpeParallelSolver UpeParallelSolver;

typedef struct {
    uint32_t num_threads;           /* Number of worker threads (0 = auto-detect) */
    uint32_t max_iterations;        /* Solver iterations per frame */
    uint32_t grain_size;            /* Min constraints per thread batch */
    int use_simd;                   /* Enable SIMD vectorization */
    int enable_graph_coloring;      /* Group constraints by color for parallel solving */
} UpeParallelSolverConfig;

/* Create parallel solver */
UpeParallelSolver *UpeParallelSolver_Create(uint32_t max_bodies, uint32_t max_constraints,
                                            const UpeParallelSolverConfig *config);
void UpeParallelSolver_Destroy(UpeParallelSolver *solver);

/* Frame lifecycle */
void UpeParallelSolver_PrepareFrame(UpeParallelSolver *solver);
void UpeParallelSolver_AddBody(UpeParallelSolver *solver, UpeBody *body);

/* Constraint addition (same as other solvers) */
void UpeParallelSolver_AddContactConstraint(UpeParallelSolver *solver,
    uint32_t body_a_idx, uint32_t body_b_idx,
    UpeVec3 contact_point_a, UpeVec3 contact_point_b, UpeVec3 normal, float penetration,
    float friction_static, float friction_dynamic);

void UpeParallelSolver_AddDistanceConstraint(UpeParallelSolver *solver,
    uint32_t body_a_idx, uint32_t body_b_idx,
    UpeVec3 anchor_a, UpeVec3 anchor_b, float rest_length,
    float compliance, float damping);

/* Solve all constraints in parallel */
void UpeParallelSolver_Solve(UpeParallelSolver *solver, float dt);

/* Performance profiling */
typedef struct {
    float graph_coloring_time_ms;   /* Time spent in graph coloring phase */
    float parallel_solve_time_ms;   /* Actual constraint solving time */
    float total_time_ms;
    uint32_t num_colors;            /* Number of constraint groups */
    float speedup_factor;           /* Experimental speedup vs single-threaded */
} UpeParallelSolverMetrics;

void UpeParallelSolver_GetMetrics(UpeParallelSolver *solver, UpeParallelSolverMetrics *out_metrics);

#ifdef __cplusplus
}
#endif
