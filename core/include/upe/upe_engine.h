/*
 * Unified Physics Engine - Main Orchestration API
 * 
 * Single entry point: UpeEngine_Step(engine, dt)
 * Coordinates all physics systems for deterministic simulation.
 */

#ifndef UPE_ENGINE_H
#define UPE_ENGINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct UpeWorld UpeWorld;
typedef struct UpeEngine UpeEngine;

/**
 * Create physics engine with default configuration.
 * @return Engine instance (must be freed with UpeEngine_Destroy)
 */
UpeEngine* UpeEngine_Create(void);

/**
 * Destroy engine and all subsystems.
 * @param engine Engine to destroy
 */
void UpeEngine_Destroy(UpeEngine *engine);

/**
 * Get the underlying rigid body world.
 * @param engine Engine instance
 * @return Rigid body world pointer
 */
UpeWorld* UpeEngine_GetWorld(UpeEngine *engine);

/**
 * Reset engine to initial state.
 * @param engine Engine to reset
 */
void UpeEngine_Reset(UpeEngine *engine);

/**
 * Main physics orchestration - call once per frame.
 * 
 * Executes physics in order:
 *   1. Broad phase collision detection
 *   2. Narrow phase collision detection
 *   3. Constraint solving (8 iterations)
 *   4. Rigid body integration
 * 
 * @param engine Physics engine
 * @param dt Delta time (seconds)
 * @return Solver iterations performed
 */
uint32_t UpeEngine_Step(UpeEngine *engine, float dt);

#ifdef __cplusplus
}
#endif

#endif /* UPE_ENGINE_H */
