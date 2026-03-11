#include "upe/upe_xpbd_solver.h"
#include "upe/upe_math.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Internal constraint types */
typedef enum {
    UPE_XPBD_CONTACT_CONSTRAINT = 0,
    UPE_XPBD_DISTANCE_CONSTRAINT = 1,
    UPE_XPBD_BENDING_CONSTRAINT = 2,
} UpeXpbdConstraintType;

typedef struct {
    UpeXpbdConstraintType type;
    uint32_t body_a;
    uint32_t body_b;
    uint32_t body_c;  /* For bending constraints */
    
    /* Contact/distance constraint data */
    UpeVec3 local_anchor_a;
    UpeVec3 local_anchor_b;
    UpeVec3 normal;
    float target_value;  /* penetration depth for contacts, rest length for distance */
    float friction_static;
    float friction_dynamic;
    
    /* Bending constraint data */
    float target_angle;
    
    /* Solver state */
    float compliance;
    float damping;
    float lagrange_multiplier;  /* Accumulated impulse for warm-starting */
    float tangent_multiplier;   /* For friction */
    
} UpeXpbdConstraint;

typedef struct {
    UpeVec3 velocity;
    UpeVec3 angular_velocity;
    float cached_inv_mass;  /* Cache for performance */
} UpeXpbdBodyState;

struct UpeXpbdSolver {
    UpeXpbdConstraint *constraints;
    uint32_t constraint_count;
    uint32_t constraint_capacity;
    
    UpeXpbdBodyState *body_states;
    UpeBody **bodies;
    uint32_t body_count;
    uint32_t body_capacity;
    
    UpeXpbdSolverConfig config;
    UpeXpbdSolverStats stats;
};

typedef struct {
    uint32_t contact_constraints;
    uint32_t distance_constraints;
    uint32_t bending_constraints;
} UpeConstraintStats;

UpeXpbdSolver *UpeXpbdSolver_Create(uint32_t max_bodies, uint32_t max_constraints) {
    UpeXpbdSolver *solver = (UpeXpbdSolver*)malloc(sizeof(UpeXpbdSolver));
    if (!solver) return nullptr;
    
    solver->constraints = (UpeXpbdConstraint*)malloc(max_constraints * sizeof(UpeXpbdConstraint));
    solver->body_states = (UpeXpbdBodyState*)malloc(max_bodies * sizeof(UpeXpbdBodyState));
    solver->bodies = (UpeBody**)malloc(max_bodies * sizeof(UpeBody*));
    
    if (!solver->constraints || !solver->body_states || !solver->bodies) {
        free(solver->constraints);
        free(solver->body_states);
        free(solver->bodies);
        free(solver);
        return nullptr;
    }
    
    solver->constraint_count = 0;
    solver->constraint_capacity = max_constraints;
    solver->body_count = 0;
    solver->body_capacity = max_bodies;
    
    /* Default config */
    solver->config.max_iterations = 4;
    solver->config.max_velocity_iterations = 1;
    solver->config.gravity_scale = 1.0f;
    solver->config.use_warm_start = 1;
    solver->config.use_velocity_phase = 1;
    
    memset(&solver->stats, 0, sizeof(solver->stats));
    
    return solver;
}

void UpeXpbdSolver_Destroy(UpeXpbdSolver *solver) {
    if (!solver) return;
    free(solver->constraints);
    free(solver->body_states);
    free(solver->bodies);
    free(solver);
}

void UpeXpbdSolver_SetConfig(UpeXpbdSolver *solver, const UpeXpbdSolverConfig *config) {
    if (solver && config) {
        solver->config = *config;
    }
}

void UpeXpbdSolver_PrepareFrame(UpeXpbdSolver *solver) {
    if (!solver) return;
    solver->constraint_count = 0;
    solver->body_count = 0;
    memset(&solver->stats, 0, sizeof(solver->stats));
}

void UpeXpbdSolver_AddBody(UpeXpbdSolver *solver, UpeBody *body) {
    if (!solver || !body || solver->body_count >= solver->body_capacity) {
        return;
    }
    
    solver->bodies[solver->body_count] = body;
    UpeXpbdBodyState *state = &solver->body_states[solver->body_count];
    state->velocity = body->velocity;
    state->angular_velocity = body->angular_velocity;
    state->cached_inv_mass = body->type == UPE_BODY_DYNAMIC ? (1.0f / body->mass) : 0.0f;
    
    solver->body_count++;
}

void UpeXpbdSolver_AddContactConstraint(UpeXpbdSolver *solver,
    uint32_t body_a_idx, uint32_t body_b_idx,
    UpeVec3 contact_point_a, UpeVec3 contact_point_b, UpeVec3 normal, float penetration,
    float friction_static, float friction_dynamic) {
    
    if (!solver || solver->constraint_count >= solver->constraint_capacity) {
        return;
    }
    
    UpeXpbdConstraint *c = &solver->constraints[solver->constraint_count++];
    c->type = UPE_XPBD_CONTACT_CONSTRAINT;
    c->body_a = body_a_idx;
    c->body_b = body_b_idx;
    c->local_anchor_a = contact_point_a;
    c->local_anchor_b = contact_point_b;
    c->normal = normal;
    c->target_value = penetration;  /* Positive penetration depth */
    c->friction_static = friction_static;
    c->friction_dynamic = friction_dynamic;
    c->compliance = 0.0f;  /* Rigid contact */
    c->damping = 0.9f;  /* High damping for contacts */
    c->lagrange_multiplier = 0.0f;
    c->tangent_multiplier = 0.0f;
    
    solver->stats.contact_constraints++;
}

void UpeXpbdSolver_AddDistanceConstraint(UpeXpbdSolver *solver,
    uint32_t body_a_idx, uint32_t body_b_idx,
    UpeVec3 anchor_a, UpeVec3 anchor_b, float rest_length,
    const UpeXpbdConstraintConfig *config) {
    
    if (!solver || solver->constraint_count >= solver->constraint_capacity) {
        return;
    }
    
    UpeXpbdConstraint *c = &solver->constraints[solver->constraint_count++];
    c->type = UPE_XPBD_DISTANCE_CONSTRAINT;
    c->body_a = body_a_idx;
    c->body_b = body_b_idx;
    c->local_anchor_a = anchor_a;
    c->local_anchor_b = anchor_b;
    c->target_value = rest_length;
    c->compliance = config ? config->compliance : 0.0f;
    c->damping = config ? config->damping : 1.0f;
    c->lagrange_multiplier = 0.0f;
    
    solver->stats.distance_constraints++;
}

void UpeXpbdSolver_AddBendingConstraint(UpeXpbdSolver *solver,
    uint32_t body_a_idx, uint32_t body_b_idx, uint32_t body_c_idx,
    float target_angle, const UpeXpbdConstraintConfig *config) {
    
    if (!solver || solver->constraint_count >= solver->constraint_capacity) {
        return;
    }
    
    UpeXpbdConstraint *c = &solver->constraints[solver->constraint_count++];
    c->type = UPE_XPBD_BENDING_CONSTRAINT;
    c->body_a = body_a_idx;
    c->body_b = body_b_idx;
    c->body_c = body_c_idx;
    c->target_angle = target_angle;
    c->compliance = config ? config->compliance : 0.0f;
    c->damping = config ? config->damping : 1.0f;
    c->lagrange_multiplier = 0.0f;
    
    solver->stats.bending_constraints++;
}

/* Helper: Solve single constraint iteration */
static void UpeXpbdSolver_SolveContactConstraint(UpeXpbdSolver *solver, 
    UpeXpbdConstraint *c, float dt) {
    
    UpeBody *body_a = solver->bodies[c->body_a];
    UpeBody *body_b = solver->bodies[c->body_b];
    UpeXpbdBodyState *state_a = &solver->body_states[c->body_a];
    UpeXpbdBodyState *state_b = &solver->body_states[c->body_b];
    
    /* World-space anchor positions */
    UpeVec3 r_a = UpeVec3_Sub(c->local_anchor_a, body_a->position);
    UpeVec3 r_b = UpeVec3_Sub(c->local_anchor_b, body_b->position);
    
    /* Relative velocity at contact */
    UpeVec3 v_a = UpeVec3_Add(state_a->velocity, UpeVec3_Cross(state_a->angular_velocity, r_a));
    UpeVec3 v_b = UpeVec3_Add(state_b->velocity, UpeVec3_Cross(state_b->angular_velocity, r_b));
    UpeVec3 rel_vel = UpeVec3_Sub(v_b, v_a);
    
    /* Project velocity onto normal */
    float vel_along_normal = UpeVec3_Dot(rel_vel, c->normal);
    
    /* Restitution: prevent overlap */
    float bias = c->target_value > 0.0f ? c->target_value * 0.1f : 0.0f;  /* Small position bias */
    
    /* Effective mass calculation */
    float inv_mass_a = state_a->cached_inv_mass;
    float inv_mass_b = state_b->cached_inv_mass;
    
    UpeVec3 r_a_cross_n = UpeVec3_Cross(r_a, c->normal);
    UpeVec3 r_b_cross_n = UpeVec3_Cross(r_b, c->normal);
    float inertia_term_a = UpeVec3_Dot(r_a_cross_n, r_a_cross_n) * 0.1f;  /* Approx inertia */
    float inertia_term_b = UpeVec3_Dot(r_b_cross_n, r_b_cross_n) * 0.1f;
    
    float effective_mass = inv_mass_a + inv_mass_b + inertia_term_a + inertia_term_b;
    if (fabsf(effective_mass) < 1e-6f) effective_mass = 1e-6f;
    effective_mass = 1.0f / effective_mass;
    
    /* Calculate impulse (separated into normal and tangential) */
    float impulse_normal = -(vel_along_normal + bias) * effective_mass * c->damping;
    
    /* Clamp to prevent restitution (one-way contact) */
    if (impulse_normal < 0.0f) {
        impulse_normal = 0.0f;
    }
    
    /* Accumulate for warm-starting next frame */
    c->lagrange_multiplier += impulse_normal;
    
    /* Apply impulse to velocities */
    UpeVec3 impulse_vec = UpeVec3_Scale(c->normal, impulse_normal);
    
    if (inv_mass_a > 0.0f) {
        state_a->velocity = UpeVec3_Sub(state_a->velocity, UpeVec3_Scale(impulse_vec, inv_mass_a));
        state_a->angular_velocity = UpeVec3_Sub(state_a->angular_velocity, 
            UpeVec3_Scale(UpeVec3_Cross(r_a, impulse_vec), 0.1f * inv_mass_a));
    }
    if (inv_mass_b > 0.0f) {
        state_b->velocity = UpeVec3_Add(state_b->velocity, UpeVec3_Scale(impulse_vec, inv_mass_b));
        state_b->angular_velocity = UpeVec3_Add(state_b->angular_velocity,
            UpeVec3_Scale(UpeVec3_Cross(r_b, impulse_vec), 0.1f * inv_mass_b));
    }
}

static void UpeXpbdSolver_SolveDistanceConstraint(UpeXpbdSolver *solver,
    UpeXpbdConstraint *c, float dt) {
    
    UpeBody *body_a = solver->bodies[c->body_a];
    UpeBody *body_b = solver->bodies[c->body_b];
    UpeXpbdBodyState *state_a = &solver->body_states[c->body_a];
    UpeXpbdBodyState *state_b = &solver->body_states[c->body_b];
    
    /* World-space positions */
    UpeVec3 pos_a = UpeVec3_Add(body_a->position, c->local_anchor_a);
    UpeVec3 pos_b = UpeVec3_Add(body_b->position, c->local_anchor_b);
    
    UpeVec3 delta = UpeVec3_Sub(pos_b, pos_a);
    float dist = UpeVec3_Length(delta);
    
    if (dist < 1e-6f) return;  /* Degenerate case */
    
    UpeVec3 dir = UpeVec3_Scale(delta, 1.0f / dist);
    
    /* Constraint error (positive when too stretched) */
    float error = dist - c->target_value;
    
    /* Compliance adjustment (soften constraint) */
    float alpha = c->compliance / (dt * dt);
    
    /* Effective mass */
    float inv_mass_a = state_a->cached_inv_mass;
    float inv_mass_b = state_b->cached_inv_mass;
    float effective_mass = (inv_mass_a + inv_mass_b) / (1.0f + alpha * dt * dt);
    
    if (fabsf(effective_mass) < 1e-6f) effective_mass = 1e-6f;
    
    /* Impulse for constraint violation */
    float impulse = -error / effective_mass;
    
    /* Accumulate for diagnostics */
    c->lagrange_multiplier += fabsf(impulse);
    
    /* Apply position correction (velocity-based) */
    UpeVec3 correction = UpeVec3_Scale(dir, impulse);
    
    if (inv_mass_a > 0.0f) {
        state_a->velocity = UpeVec3_Sub(state_a->velocity, UpeVec3_Scale(correction, inv_mass_a));
    }
    if (inv_mass_b > 0.0f) {
        state_b->velocity = UpeVec3_Add(state_b->velocity, UpeVec3_Scale(correction, inv_mass_b));
    }
}

void UpeXpbdSolver_Solve(UpeXpbdSolver *solver, float dt) {
    if (!solver || dt <= 0.0f) return;
    
    /* Iteration loop: solve all constraints multiple times */
    for (uint32_t iter = 0; iter < solver->config.max_iterations; ++iter) {
        for (uint32_t c = 0; c < solver->constraint_count; ++c) {
            UpeXpbdConstraint *constraint = &solver->constraints[c];
            
            switch (constraint->type) {
                case UPE_XPBD_CONTACT_CONSTRAINT:
                    UpeXpbdSolver_SolveContactConstraint(solver, constraint, dt);
                    break;
                case UPE_XPBD_DISTANCE_CONSTRAINT:
                    UpeXpbdSolver_SolveDistanceConstraint(solver, constraint, dt);
                    break;
                default:
                    break;
            }
        }
    }
    
    /* Apply solved velocities back to bodies */
    for (uint32_t i = 0; i < solver->body_count; ++i) {
        UpeBody *body = solver->bodies[i];
        UpeXpbdBodyState *state = &solver->body_states[i];
        
        body->velocity = state->velocity;
        body->angular_velocity = state->angular_velocity;
    }
}

UpeVec3 UpeXpbdSolver_GetVelocity(UpeXpbdSolver *solver, uint32_t body_idx) {
    if (!solver || body_idx >= solver->body_count) {
        return {0.0f, 0.0f, 0.0f};
    }
    return solver->body_states[body_idx].velocity;
}

UpeVec3 UpeXpbdSolver_GetAngularVelocity(UpeXpbdSolver *solver, uint32_t body_idx) {
    if (!solver || body_idx >= solver->body_count) {
        return {0.0f, 0.0f, 0.0f};
    }
    return solver->body_states[body_idx].angular_velocity;
}

/* Compliance presets based on material properties */
UpeXpbdConstraintConfig UpeXpbdConstraintConfig_GetPreset(UpeCompliancePreset preset) {
    UpeXpbdConstraintConfig config = {};
    
    switch (preset) {
        case UPE_COMPLIANCE_RIGID:
            config.compliance = 0.0f;
            config.damping = 1.0f;
            config.bias_coefficient = 0.2f;
            break;
        case UPE_COMPLIANCE_STIFF:
            config.compliance = 0.001f;
            config.damping = 0.95f;
            config.bias_coefficient = 0.15f;
            break;
        case UPE_COMPLIANCE_NORMAL:
            config.compliance = 0.01f;
            config.damping = 0.85f;
            config.bias_coefficient = 0.1f;
            break;
        case UPE_COMPLIANCE_SOFT:
            config.compliance = 0.1f;
            config.damping = 0.7f;
            config.bias_coefficient = 0.05f;
            break;
        case UPE_COMPLIANCE_VERY_SOFT:
            config.compliance = 0.5f;
            config.damping = 0.5f;
            config.bias_coefficient = 0.02f;
            break;
    }
    
    return config;
}

void UpeXpbdSolver_GetStats(UpeXpbdSolver *solver, UpeXpbdSolverStats *out_stats) {
    if (!solver || !out_stats) return;
    
    out_stats->total_constraints = solver->constraint_count;
    out_stats->contact_constraints = solver->stats.contact_constraints;
    out_stats->distance_constraints = solver->stats.distance_constraints;
    out_stats->bending_constraints = solver->stats.bending_constraints;
    out_stats->avg_iterations_to_convergence = (float)solver->config.max_iterations;
}
