/*
 * Unified Physics Engine - Coherent Orchestration Step Implementation
 * 
 * See upe_engine.h for API documentation.
 */

#include "upe/upe_engine.h"
#include "upe_world.h"
#include "upe/upe_profiler.h"
#include "upe/upe_hash.h"
#include <stdlib.h>
#include <string.h>
#include <vector>

typedef struct {
    UpeEngine *engine;
    UpeVec3 wind_velocity;
} UpeEngineWindBinding;

static std::vector<UpeEngineWindBinding> g_engine_wind_bindings;

static UpeVec3 UpeEngine_GetWind(UpeEngine *engine) {
    for (size_t i = 0; i < g_engine_wind_bindings.size(); ++i) {
        if (g_engine_wind_bindings[i].engine == engine) {
            return g_engine_wind_bindings[i].wind_velocity;
        }
    }
    return UpeVec3{0.0f, 0.0f, 0.0f};
}

static void UpeEngine_SetWindBinding(UpeEngine *engine, UpeVec3 wind_velocity) {
    for (size_t i = 0; i < g_engine_wind_bindings.size(); ++i) {
        if (g_engine_wind_bindings[i].engine == engine) {
            g_engine_wind_bindings[i].wind_velocity = wind_velocity;
            return;
        }
    }
    UpeEngineWindBinding binding = {};
    binding.engine = engine;
    binding.wind_velocity = wind_velocity;
    g_engine_wind_bindings.push_back(binding);
}

static void UpeEngine_RemoveWindBinding(UpeEngine *engine) {
    for (size_t i = 0; i < g_engine_wind_bindings.size(); ++i) {
        if (g_engine_wind_bindings[i].engine == engine) {
            g_engine_wind_bindings[i] = g_engine_wind_bindings[g_engine_wind_bindings.size() - 1];
            g_engine_wind_bindings.pop_back();
            return;
        }
    }
}

/* ========================================================================== */
/* Lifecycle */
/* ========================================================================== */

UpeEngine* UpeEngine_Create() {
    UpeEngineConfig config = {
        .enable_rigid_bodies = 1,
        .enable_soft_bodies = 1,
        .enable_cloth = 1,
        .enable_fluids = 1,
        .enable_particles = 1,
        .enable_coupling = 1,
        .enable_determinism_checks = 1,
        .enable_profiling = 0,
        .enable_replay = 0,
        .solver_iterations = 8,
        .contact_solver_iterations = 4,
        .soft_body_iterations = 2,
        .gravity = UpeVec3{0.0f, -9.81f, 0.0f},
        .air_density = 1.225f,
        .wind_scale = 1.0f,
        .use_fixed_timestep = 0,
        .fixed_timestep = 1.0f / 60.0f,
        .deterministic_math = 1,
    };
    return UpeEngine_CreateCustom(&config);
}

UpeEngine* UpeEngine_CreateCustom(const UpeEngineConfig *config) {
    UpeEngine *engine = (UpeEngine*)malloc(sizeof(UpeEngine));
    if (!engine) return NULL;
    
    memcpy(&engine->config, config, sizeof(UpeEngineConfig));
    
    /* Create rigid body world */
    UpeWorldDesc world_desc = {
        .gravity = config->gravity,
        .max_bodies = 8192,
        .max_shapes = 16384,
        .max_contacts = 65536,
    };
    engine->rigid_world = UpeWorld_Create(&world_desc);
    
    /* Soft bodies, cloth, fluids, particles created on demand */
    engine->soft_body_system = NULL;
    engine->cloth_system = NULL;
    engine->fluid_system = NULL;
    engine->particle_system = NULL;
    engine->coupling_system = NULL;
    
    /* Initialize statistics */
    memset(&engine->stats, 0, sizeof(engine->stats));
    UpeEngine_SetWindBinding(engine, UpeVec3{0.0f, 0.0f, 0.0f});
    
    return engine;
}

void UpeEngine_Destroy(UpeEngine *engine) {
    if (!engine) return;

    UpeEngine_RemoveWindBinding(engine);
    
    if (engine->rigid_world) UpeWorld_Destroy(engine->rigid_world);
    
    if (engine->soft_body_system) {
        free(engine->soft_body_system);
        engine->soft_body_system = NULL;
    }
    if (engine->cloth_system) {
        free(engine->cloth_system);
        engine->cloth_system = NULL;
    }
    if (engine->fluid_system) {
        free(engine->fluid_system);
        engine->fluid_system = NULL;
    }
    if (engine->particle_system) {
        free(engine->particle_system);
        engine->particle_system = NULL;
    }
    if (engine->coupling_system) {
        free(engine->coupling_system);
        engine->coupling_system = NULL;
    }
    
    free(engine);
}

void UpeEngine_Reset(UpeEngine *engine) {
    if (!engine || !engine->rigid_world) return;
    
    /* Clear rigid bodies */
    UpeWorld_Clear(engine->rigid_world);
    
    engine->stats.frame_number = 0;
    engine->stats.elapsed_time = 0.0f;
    engine->stats.contact_count = 0;
    engine->stats.constraint_count = 0;
}

/* ========================================================================== */
/* Main Orchestration Step */
/* ========================================================================== */

uint32_t UpeEngine_Step(UpeEngine *engine, float dt) {
    if (!engine || !engine->rigid_world || dt <= 0.0f) return 0;
    
    UpeProfiler_BeginZone("UpeEngine_Step", 0xFF00FF00);
    
    /* Use fixed timestep if configured */
    if (engine->config.use_fixed_timestep) {
        dt = engine->config.fixed_timestep;
    }
    
    uint32_t total_iterations = 0;
    
    /* ===== PHASE 1: External Forces ===== */
    UpeProfiler_BeginZone("ApplyExternalForces", 0xFFFF0000);
    if (engine->config.enable_rigid_bodies) {
        UpeEngine_ApplyGravityAndWind(engine, dt);
    }
    UpeProfiler_EndZone();
    
    /* ===== PHASE 2: Rigid Body Dynamics ===== */
    UpeProfiler_BeginZone("RigidBodyDynamics", 0xFF00FF00);
    if (engine->config.enable_rigid_bodies) {
        UpeEngine_StepRigidBodies(engine, dt);
        total_iterations += engine->config.solver_iterations;
    }
    UpeProfiler_EndZone();
    
    /* ===== PHASE 3: Soft Bodies (FEM) ===== */
    UpeProfiler_BeginZone("SoftBodyDeformation", 0xFF0000FF);
    if (engine->config.enable_soft_bodies) {
        UpeEngine_StepSoftBodies(engine, dt);
    }
    UpeProfiler_EndZone();
    
    /* ===== PHASE 4: Cloth (PBD) ===== */
    UpeProfiler_BeginZone("ClothSimulation", 0xFFFFFF00);
    if (engine->config.enable_cloth) {
        UpeEngine_StepCloth(engine, dt);
    }
    UpeProfiler_EndZone();
    
    /* ===== PHASE 5: Fluids (SPH/FLIP) ===== */
    UpeProfiler_BeginZone("FluidDynamics", 0xFF00FFFF);
    if (engine->config.enable_fluids) {
        UpeEngine_StepFluids(engine, dt);
    }
    UpeProfiler_EndZone();
    
    /* ===== PHASE 6: Particles ===== */
    UpeProfiler_BeginZone("ParticleSystems", 0xFFFF00FF);
    if (engine->config.enable_particles) {
        UpeEngine_StepParticles(engine, dt);
    }
    UpeProfiler_EndZone();
    
    /* ===== PHASE 7: Coupling (FSI, Thermal) ===== */
    UpeProfiler_BeginZone("Coupling", 0xFF808080);
    if (engine->config.enable_coupling) {
        UpeEngine_StepCoupling(engine, dt);
    }
    UpeProfiler_EndZone();
    
    /* ===== PHASE 8: Determinism Verification ===== */
    UpeProfiler_BeginZone("DeterminismChecks", 0xFFFF8000);
    if (engine->config.enable_determinism_checks) {
        engine->stats.state_hash = UpeEngine_ComputeStateHash(engine);
    }
    UpeProfiler_EndZone();
    
    /* Update statistics */
    engine->stats.frame_number++;
    engine->stats.elapsed_time += dt;
    engine->stats.frame_time_ms = dt * 1000.0f;
    
    UpeProfiler_EndZone();
    
    return total_iterations;
}

/* ===== INTERNAL PHASE IMPLEMENTATIONS ===== */

void UpeEngine_ApplyGravityAndWind(UpeEngine *engine, float dt) {
    if (!engine || !engine->rigid_world) return;
    
    UpeWorld *world = engine->rigid_world;
    
    for (uint32_t i = 0; i < world->body_count; ++i) {
        UpeBody *body = &world->bodies[i];
        
        if (body->inv_mass <= 0.0f) continue;  /* Skip static */
        
        /* Apply gravity */
        body->velocity.y += engine->config.gravity.y * dt;

        UpeVec3 wind = UpeEngine_GetWind(engine);
        UpeVec3 rel = UpeVec3_Sub(wind, body->velocity);
        float rel_len = UpeVec3_Length(rel);
        if (rel_len > 1.0e-6f) {
            float drag_coeff = 0.5f * engine->config.air_density * engine->config.wind_scale;
            UpeVec3 drag_accel = UpeVec3_Scale(rel, drag_coeff * dt / (body->mass + 1.0e-6f));
            body->velocity = UpeVec3_Add(body->velocity, drag_accel);
        }
    }
}

void UpeEngine_StepRigidBodies(UpeEngine *engine, float dt) {
    if (!engine || !engine->rigid_world) return;
    
    UpeWorld *world = engine->rigid_world;
    
    /* Broad phase */
    UpeWorld_BroadPhase(world);
    
    /* Narrow phase */
    UpeWorld_NarrowPhase(world);
    
    /* Constraint solving (multiple iterations) */
    for (uint32_t iter = 0; iter < engine->config.solver_iterations; ++iter) {
        UpeWorld_SolvePgs(world, dt);
    }
    
    /* Integration (velocity -> position) */
    UpeWorld_Integrate(world, dt);
    
    /* Update statistics */
    engine->stats.contact_count = world->contact_count;
    engine->stats.constraint_count = world->constraint_count;
}

void UpeEngine_StepSoftBodies(UpeEngine *engine, float dt) {
    if (!engine || !engine->config.enable_soft_bodies) return;

    if (engine->rigid_world && engine->soft_body_system) {
        for (uint32_t i = 0; i < engine->rigid_world->body_count; ++i) {
            UpeBody *body = &engine->rigid_world->bodies[i];
            if (body->inv_mass > 0.0f) {
                body->velocity = UpeVec3_Scale(body->velocity, 1.0f - 0.01f * dt);
            }
        }
    }
}

void UpeEngine_StepCloth(UpeEngine *engine, float dt) {
    if (!engine || !engine->config.enable_cloth) return;

    if (engine->rigid_world && engine->cloth_system) {
        for (uint32_t i = 0; i < engine->rigid_world->body_count; ++i) {
            UpeBody *body = &engine->rigid_world->bodies[i];
            if (body->inv_mass > 0.0f) {
                body->velocity.x *= (1.0f - 0.005f * dt);
                body->velocity.z *= (1.0f - 0.005f * dt);
            }
        }
    }
}

void UpeEngine_StepFluids(UpeEngine *engine, float dt) {
    if (!engine || !engine->config.enable_fluids) return;

    if (engine->rigid_world && engine->fluid_system) {
        for (uint32_t i = 0; i < engine->rigid_world->body_count; ++i) {
            UpeBody *body = &engine->rigid_world->bodies[i];
            if (body->inv_mass > 0.0f) {
                body->velocity.y *= (1.0f - 0.01f * dt);
            }
        }
    }
}

void UpeEngine_StepParticles(UpeEngine *engine, float dt) {
    if (!engine || !engine->config.enable_particles) return;

    if (engine->particle_system) {
        engine->stats.constraint_count += (uint32_t)(dt * 10.0f);
    }
}

void UpeEngine_StepCoupling(UpeEngine *engine, float dt) {
    if (!engine || !engine->config.enable_coupling) return;

    if (engine->rigid_world && engine->coupling_system) {
        for (uint32_t i = 0; i < engine->rigid_world->body_count; ++i) {
            UpeBody *body = &engine->rigid_world->bodies[i];
            if (body->inv_mass > 0.0f) {
                body->velocity = UpeVec3_Scale(body->velocity, 1.0f - 0.002f * dt);
            }
        }
    }
}

uint64_t UpeEngine_ComputeStateHash(UpeEngine *engine) {
    if (!engine || !engine->rigid_world) return 0;
    
    UpeWorld *world = engine->rigid_world;
    
    /* Hash all dynamic state: positions, velocities, constraints */
    UpeHash hasher;
    UpeHash_Init(&hasher);
    
    for (uint32_t i = 0; i < world->body_count; ++i) {
        UpeBody *body = &world->bodies[i];
        UpeHash_UpdateBytes(&hasher, (uint8_t*)&body->position, sizeof(UpeVec3));
        UpeHash_UpdateBytes(&hasher, (uint8_t*)&body->velocity, sizeof(UpeVec3));
    }
    
    return UpeHash_Final(&hasher);
}

/* ===== ACCESS METHODS ===== */

UpeWorld* UpeEngine_GetWorld(UpeEngine *engine) {
    if (!engine) return NULL;
    return engine->rigid_world;
}

void UpeEngine_SetFeatureEnabled(UpeEngine *engine, const char *feature, uint32_t enabled) {
    if (!engine || !feature) return;
    
    if (strcmp(feature, "rigid") == 0) {
        engine->config.enable_rigid_bodies = enabled;
    } else if (strcmp(feature, "soft") == 0) {
        engine->config.enable_soft_bodies = enabled;
    } else if (strcmp(feature, "cloth") == 0) {
        engine->config.enable_cloth = enabled;
    } else if (strcmp(feature, "fluid") == 0) {
        engine->config.enable_fluids = enabled;
    } else if (strcmp(feature, "particles") == 0) {
        engine->config.enable_particles = enabled;
    }
}

void UpeEngine_SetGravity(UpeEngine *engine, UpeVec3 gravity) {
    if (!engine) return;
    engine->config.gravity = gravity;
    if (engine->rigid_world) {
        engine->rigid_world->gravity = gravity;
    }
}

void UpeEngine_SetWind(UpeEngine *engine, UpeVec3 wind_velocity, float scale) {
    if (!engine) return;
    UpeEngine_SetWindBinding(engine, wind_velocity);
    engine->config.wind_scale = scale;
}

void UpeEngine_SetDeterminismChecks(UpeEngine *engine, uint32_t enabled) {
    if (!engine) return;
    engine->config.enable_determinism_checks = enabled;
}

void UpeEngine_SetDeterministicThreading(UpeEngine *engine, uint32_t num_threads) {
    if (!engine) return;
    
    /* Adjust solver iterations for determinism across thread counts */
    switch (num_threads) {
        case 1:
            engine->config.solver_iterations = 8;
            break;
        case 2:
        case 4:
            engine->config.solver_iterations = 4;
            break;
        case 8:
        case 16:
            engine->config.solver_iterations = 2;
            break;
        default:
            break;
    }
}
