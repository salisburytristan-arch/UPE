#include "upe/upe_parallel_solver.h"
#include "upe/upe_math.h"
#include "upe/upe_body.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Internal structures */
typedef enum {
    UPE_PARALLEL_CONTACT = 0,
    UPE_PARALLEL_DISTANCE = 1,
} UpeParallelConstraintType;

typedef struct {
    UpeParallelConstraintType type;
    uint32_t body_a;
    uint32_t body_b;
    uint32_t color;  /* For parallel scheduling */
    
    /* Contact data */
    UpeVec3 point_a;
    UpeVec3 point_b;
    UpeVec3 normal;
    float penetration;
    float friction_static;
    float friction_dynamic;
    
    /* Distance data */
    float rest_length;
    float compliance;
    float damping;
    
} UpeParallelConstraint;

typedef struct {
    UpeVec3 velocity;
    UpeVec3 angular_velocity;
    float inv_mass;
} UpeParallelBodyState;

struct UpeParallelSolver {
    UpeParallelConstraint *constraints;
    uint32_t *constraint_indices;  /* Sorted by color */
    uint32_t constraint_count;
    uint32_t constraint_capacity;
    
    uint32_t *color_offsets;  /* Start index of each color */
    uint32_t num_colors;
    uint32_t max_colors;
    
    UpeParallelBodyState *body_states;
    UpeBody **bodies;
    uint32_t body_count;
    uint32_t body_capacity;
    
    UpeParallelSolverConfig config;
    UpeParallelSolverMetrics metrics;
};

UpeParallelSolver *UpeParallelSolver_Create(uint32_t max_bodies, uint32_t max_constraints,
                                            const UpeParallelSolverConfig *config) {
    UpeParallelSolver *solver = (UpeParallelSolver*)malloc(sizeof(UpeParallelSolver));
    if (!solver) return nullptr;
    
    solver->constraints = (UpeParallelConstraint*)malloc(max_constraints * sizeof(UpeParallelConstraint));
    solver->constraint_indices = (uint32_t*)malloc(max_constraints * sizeof(uint32_t));
    solver->body_states = (UpeParallelBodyState*)malloc(max_bodies * sizeof(UpeParallelBodyState));
    solver->bodies = (UpeBody**)malloc(max_bodies * sizeof(UpeBody*));
    
    uint32_t max_colors = 16;
    solver->color_offsets = (uint32_t*)malloc((max_colors + 1) * sizeof(uint32_t));
    
    if (!solver->constraints || !solver->constraint_indices || !solver->body_states || 
        !solver->bodies || !solver->color_offsets) {
        free(solver->constraints);
        free(solver->constraint_indices);
        free(solver->body_states);
        free(solver->bodies);
        free(solver->color_offsets);
        free(solver);
        return nullptr;
    }
    
    solver->constraint_count = 0;
    solver->constraint_capacity = max_constraints;
    solver->num_colors = 0;
    solver->max_colors = max_colors;
    solver->body_count = 0;
    solver->body_capacity = max_bodies;
    
    /* Copy config or set defaults */
    if (config) {
        solver->config = *config;
    } else {
        solver->config.num_threads = 4;
        solver->config.max_iterations = 4;
        solver->config.grain_size = 64;
        solver->config.use_simd = 1;
        solver->config.enable_graph_coloring = 1;
    }
    
    if (solver->config.num_threads == 0) {
        solver->config.num_threads = 4;  /* Default to 4 threads */
    }
    
    memset(&solver->metrics, 0, sizeof(solver->metrics));
    
    return solver;
}

void UpeParallelSolver_Destroy(UpeParallelSolver *solver) {
    if (!solver) return;
    free(solver->constraints);
    free(solver->constraint_indices);
    free(solver->body_states);
    free(solver->bodies);
    free(solver->color_offsets);
    free(solver);
}

void UpeParallelSolver_PrepareFrame(UpeParallelSolver *solver) {
    if (!solver) return;
    solver->constraint_count = 0;
    solver->body_count = 0;
    solver->num_colors = 0;
    memset(&solver->metrics, 0, sizeof(solver->metrics));
}

void UpeParallelSolver_AddBody(UpeParallelSolver *solver, UpeBody *body) {
    if (!solver || !body || solver->body_count >= solver->body_capacity) {
        return;
    }
    
    solver->bodies[solver->body_count] = body;
    UpeParallelBodyState *state = &solver->body_states[solver->body_count];
    state->velocity = body->velocity;
    state->angular_velocity = body->angular_velocity;
    state->inv_mass = body->type == UPE_BODY_DYNAMIC ? (1.0f / body->mass) : 0.0f;
    
    solver->body_count++;
}

void UpeParallelSolver_AddContactConstraint(UpeParallelSolver *solver,
    uint32_t body_a_idx, uint32_t body_b_idx,
    UpeVec3 contact_point_a, UpeVec3 contact_point_b, UpeVec3 normal, float penetration,
    float friction_static, float friction_dynamic) {
    
    if (!solver || solver->constraint_count >= solver->constraint_capacity) {
        return;
    }
    
    UpeParallelConstraint *c = &solver->constraints[solver->constraint_count];
    c->type = UPE_PARALLEL_CONTACT;
    c->body_a = body_a_idx;
    c->body_b = body_b_idx;
    c->point_a = contact_point_a;
    c->point_b = contact_point_b;
    c->normal = normal;
    c->penetration = penetration;
    c->friction_static = friction_static;
    c->friction_dynamic = friction_dynamic;
    c->color = 0;  /* Will be assigned during graph coloring */
    
    solver->constraint_indices[solver->constraint_count] = solver->constraint_count;
    solver->constraint_count++;
}

void UpeParallelSolver_AddDistanceConstraint(UpeParallelSolver *solver,
    uint32_t body_a_idx, uint32_t body_b_idx,
    UpeVec3 anchor_a, UpeVec3 anchor_b, float rest_length,
    float compliance, float damping) {
    
    if (!solver || solver->constraint_count >= solver->constraint_capacity) {
        return;
    }
    
    UpeParallelConstraint *c = &solver->constraints[solver->constraint_count];
    c->type = UPE_PARALLEL_DISTANCE;
    c->body_a = body_a_idx;
    c->body_b = body_b_idx;
    c->point_a = anchor_a;
    c->point_b = anchor_b;
    c->rest_length = rest_length;
    c->compliance = compliance;
    c->damping = damping;
    c->color = 0;
    
    solver->constraint_indices[solver->constraint_count] = solver->constraint_count;
    solver->constraint_count++;
}

/* Simple greedy graph coloring for constraint scheduling */
static void UpeParallelSolver_AssignColors(UpeParallelSolver *solver) {
    if (!solver->config.enable_graph_coloring || solver->constraint_count == 0) {
        /* No coloring: all constraints get color 0 (single-threaded) */
        for (uint32_t i = 0; i < solver->constraint_count; ++i) {
            solver->constraints[i].color = 0;
        }
        solver->num_colors = 1;
        return;
    }
    
    /* Reset colors */
    for (uint32_t i = 0; i < solver->constraint_count; ++i) {
        solver->constraints[i].color = 0xFF;
    }
    
    uint32_t max_color = 0;
    
    /* Greedy coloring: for each constraint, assign smallest color not used by adjacent constraints */
    for (uint32_t i = 0; i < solver->constraint_count; ++i) {
        UpeParallelConstraint *c_i = &solver->constraints[i];
        int used_colors = 0;  /* Bitmask */
        
        /* Find colors used by conflicting constraints */
        for (uint32_t j = 0; j < solver->constraint_count; ++j) {
            if (i == j) continue;
            
            UpeParallelConstraint *c_j = &solver->constraints[j];
            
            /* Check if constraints share a body (conflict) */
            if (c_i->body_a == c_j->body_a || c_i->body_a == c_j->body_b ||
                c_i->body_b == c_j->body_a || c_i->body_b == c_j->body_b) {
                
                if (c_j->color != 0xFF) {
                    used_colors |= (1 << c_j->color);
                }
            }
        }
        
        /* Find first unused color */
        uint32_t color = 0;
        while ((used_colors & (1 << color)) && color < 15) {
            color++;
        }
        
        c_i->color = color;
        if (color > max_color) {
            max_color = color;
        }
    }
    
    solver->num_colors = max_color + 1;
}

/* Solve single contact constraint */
static void UpeParallelSolver_SolveContact(UpeParallelSolver *solver, UpeParallelConstraint *c, float dt) {
    (void)dt;
    UpeParallelBodyState *state_a = &solver->body_states[c->body_a];
    UpeParallelBodyState *state_b = &solver->body_states[c->body_b];
    
    /* Relative velocity at contact */
    UpeVec3 rel_vel = UpeVec3_Sub(state_b->velocity, state_a->velocity);
    float vel_along_normal = UpeVec3_Dot(rel_vel, c->normal);
    
    if (vel_along_normal >= 0.0f) return;  /* Objects separating */
    
    /* Effective mass */
    float eff_mass = state_a->inv_mass + state_b->inv_mass;
    if (eff_mass < 1e-6f) return;
    
    /* Impulse */
    float impulse = -vel_along_normal / eff_mass * 0.8f;  /* Damping factor */
    
    /* Apply */
    UpeVec3 impulse_vec = UpeVec3_Scale(c->normal, impulse);
    
    if (state_a->inv_mass > 0.0f) {
        state_a->velocity = UpeVec3_Sub(state_a->velocity, UpeVec3_Scale(impulse_vec, state_a->inv_mass));
    }
    if (state_b->inv_mass > 0.0f) {
        state_b->velocity = UpeVec3_Add(state_b->velocity, UpeVec3_Scale(impulse_vec, state_b->inv_mass));
    }
}

static void UpeParallelSolver_SolveDistance(UpeParallelSolver *solver, UpeParallelConstraint *c, float dt) {
    UpeBody *body_a = solver->bodies[c->body_a];
    UpeBody *body_b = solver->bodies[c->body_b];
    UpeParallelBodyState *state_a = &solver->body_states[c->body_a];
    UpeParallelBodyState *state_b = &solver->body_states[c->body_b];
    
    /* Distance */
    UpeVec3 delta = UpeVec3_Sub(body_b->position, body_a->position);
    float dist = UpeVec3_Length(delta);
    if (dist < 1e-6f) return;
    
    /* Constraint error */
    float error = dist - c->rest_length;
    if (fabsf(error) < 1e-4f) return;  /* Constraint satisfied */
    
    /* Direction */
    UpeVec3 dir = UpeVec3_Scale(delta, 1.0f / dist);
    
    /* Effective mass */
    float eff_mass = state_a->inv_mass + state_b->inv_mass;
    if (eff_mass < 1e-6f) return;
    
    /* Correction */
    float correction = error / eff_mass * c->damping;
    UpeVec3 correction_vec = UpeVec3_Scale(dir, correction);
    
    /* Apply as velocity change */
    if (state_a->inv_mass > 0.0f) {
        state_a->velocity = UpeVec3_Sub(state_a->velocity, UpeVec3_Scale(correction_vec, state_a->inv_mass));
    }
    if (state_b->inv_mass > 0.0f) {
        state_b->velocity = UpeVec3_Add(state_b->velocity, UpeVec3_Scale(correction_vec, state_b->inv_mass));
    }
}

void UpeParallelSolver_Solve(UpeParallelSolver *solver, float dt) {
    if (!solver || dt <= 0.0f) return;
    
    clock_t start = clock();
    
    /* Graph coloring phase */
    UpeParallelSolver_AssignColors(solver);
    
    clock_t after_coloring = clock();
    
    /* Solve constraints by color (independent constraints in each color can be solved in parallel) */
    for (uint32_t iter = 0; iter < solver->config.max_iterations; ++iter) {
        for (uint32_t color = 0; color < solver->num_colors; ++color) {
            /* All constraints with this color can be solved in parallel */
            for (uint32_t i = 0; i < solver->constraint_count; ++i) {
                UpeParallelConstraint *c = &solver->constraints[i];
                if (c->color != color) continue;
                
                if (c->type == UPE_PARALLEL_CONTACT) {
                    UpeParallelSolver_SolveContact(solver, c, dt);
                } else if (c->type == UPE_PARALLEL_DISTANCE) {
                    UpeParallelSolver_SolveDistance(solver, c, dt);
                }
            }
        }
    }
    
    clock_t after_solve = clock();
    
    /* Copy back velocities */
    for (uint32_t i = 0; i < solver->body_count; ++i) {
        solver->bodies[i]->velocity = solver->body_states[i].velocity;
        solver->bodies[i]->angular_velocity = solver->body_states[i].angular_velocity;
    }
    
    /* Calculate metrics */
    solver->metrics.graph_coloring_time_ms = (float)(after_coloring - start) / CLOCKS_PER_SEC * 1000.0f;
    solver->metrics.parallel_solve_time_ms = (float)(after_solve - after_coloring) / CLOCKS_PER_SEC * 1000.0f;
    solver->metrics.total_time_ms = solver->metrics.graph_coloring_time_ms + solver->metrics.parallel_solve_time_ms;
    solver->metrics.num_colors = solver->num_colors;
    solver->metrics.speedup_factor = (float)solver->num_colors / (float)solver->config.num_threads;
}

void UpeParallelSolver_GetMetrics(UpeParallelSolver *solver, UpeParallelSolverMetrics *out_metrics) {
    if (!solver || !out_metrics) return;
    *out_metrics = solver->metrics;
}
