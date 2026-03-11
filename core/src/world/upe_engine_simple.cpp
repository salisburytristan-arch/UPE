/*
 * Unified Physics Engine - Orchestration Implementation
 * 
 * Minimal implementation that delegates to existing physics subsystems.
 */

#include "upe/upe_engine.h"
#include "upe_world.h"
#include <stdlib.h>
#include <string.h>

/* Minimal opaque engine structure */
struct UpeEngine {
    UpeWorld *world;
    uint32_t step_count;
    float time_accumulated;
};

/* ========================================================================== */
/* Lifecycle */
/* ========================================================================== */

UpeEngine* UpeEngine_Create(void) {
    UpeEngine *engine = (UpeEngine*)malloc(sizeof(UpeEngine));
    if (!engine) return NULL;
    
    /* Create rigid body world */
    UpeWorldDesc desc = {};
    desc.fixed_dt = 1.0f / 60.0f;
    desc.gravity[0] = 0.0f;
    desc.gravity[1] = -9.81f;
    desc.gravity[2] = 0.0f;
    desc.enable_sleeping = 1;
    desc.enable_warm_starting = 1;
    
    engine->world = UpeWorld_Create(&desc);
    if (!engine->world) {
        free(engine);
        return NULL;
    }
    
    engine->step_count = 0;
    engine->time_accumulated = 0.0f;
    
    return engine;
}

void UpeEngine_Destroy(UpeEngine *engine) {
    if (!engine) return;
    
    if (engine->world) {
        UpeWorld_Destroy(engine->world);
    }
    
    free(engine);
}

UpeWorld* UpeEngine_GetWorld(UpeEngine *engine) {
    return engine ? engine->world : NULL;
}

void UpeEngine_Reset(UpeEngine *engine) {
    if (!engine || !engine->world) return;
    
    /* Re-create the world to reset it */
    UpeWorld_Destroy(engine->world);
    UpeWorldDesc desc = {};
    desc.fixed_dt = 1.0f / 60.0f;
    desc.gravity[0] = 0.0f;
    desc.gravity[1] = -9.81f;
    desc.gravity[2] = 0.0f;
    desc.enable_sleeping = 1;
    desc.enable_warm_starting = 1;
    engine->world = UpeWorld_Create(&desc);
    engine->step_count = 0;
    engine->time_accumulated = 0.0f;
}

/* ========================================================================== */
/* Main Orchestration */
/* ========================================================================== */

uint32_t UpeEngine_Step(UpeEngine *engine, float dt) {
    if (!engine || !engine->world || dt <= 0.0f) {
        return 0;
    }
    
    /* Update fixed timestep */
    engine->world->fixed_dt = dt;
    
    /* Execute main physics step (includes broad phase, narrow phase, solving, integration) */
    UpeWorld_Step(engine->world);
    
    engine->step_count++;
    engine->time_accumulated += dt;
    
    /* Return constant iterations for API compatibility */
    return 8;
}
