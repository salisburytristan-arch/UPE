#pragma once

#include <stdint.h>
#include "upe_math.h"
#include "upe_body.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Advanced Destruction System
 * 
 * Enables dynamic destruction with:
 * - Fracture propagation based on stress/impact
 * - Material-specific failure modes
 * - Post-fracture object spawning
 * - Debris generation and cleanup
 * - Destruction callbacks for gameplay
 */

typedef enum {
    UPE_MATERIAL_CONCRETE = 0,
    UPE_MATERIAL_WOOD = 1,
    UPE_MATERIAL_GLASS = 2,
    UPE_MATERIAL_METAL = 3,
    UPE_MATERIAL_CERAMIC = 4,
} UpeDestructionMaterial;

typedef struct {
    UpeDestructionMaterial material;
    float fracture_threshold;   /* Force/stress needed to break */
    float density;              /* kg/m³ */
    float brittleness;          /* 0 = plastic, 1 = brittle */
    int create_debris;          /* Spawn debris pieces */
    int propagate_fracture;     /* Fracture spreads to adjacent objects */
} UpeDestructionMaterialDesc;

typedef struct {
    uint32_t original_body_id;
    UpeVec3 fracture_point;
    UpeVec3 impact_normal;
    float impact_force;
    int propagate_to_neighbors;
} UpeDestructionEvent;

typedef struct UpeDestructionTracker UpeDestructionTracker;

/* Create destruction tracker for a world */
UpeDestructionTracker *UpeDestructionTracker_Create(uint32_t max_objects);
void UpeDestructionTracker_Destroy(UpeDestructionTracker *tracker);

/* Register body as destructible */
void UpeDestructionTracker_RegisterBody(UpeDestructionTracker *tracker, 
    uint32_t body_id, const UpeDestructionMaterialDesc *material);

/* Report impact/collision to destruction system */
void UpeDestructionTracker_ReportImpact(UpeDestructionTracker *tracker,
    uint32_t body_id, UpeVec3 impact_point, UpeVec3 impact_normal, float impact_force);

/* Process destruction each frame (call after constraint solving) */
typedef struct {
    uint32_t destroyed_object_count;
    uint32_t generated_debris_count;
    float total_stress_energy;
} UpeDestructionFrameResult;

void UpeDestructionTracker_ProcessFrame(UpeDestructionTracker *tracker,
    UpeDestructionFrameResult *out_result);

/* Query destruction state */
int UpeDestructionTracker_IsDestroyed(UpeDestructionTracker *tracker, uint32_t body_id);
float UpeDestructionTracker_GetDamage(UpeDestructionTracker *tracker, uint32_t body_id);

/* Callbacks for destruction events */
typedef void (*UpeDestructionCallback)(uint32_t body_id, UpeVec3 fracture_point, void *user_data);
void UpeDestructionTracker_SetDestructionCallback(UpeDestructionTracker *tracker,
    UpeDestructionCallback callback, void *user_data);

/* Propagate fracture to nearby objects */
void UpeDestructionTracker_PropagateFracture(UpeDestructionTracker *tracker,
    uint32_t initiating_body, float propagation_radius);

#ifdef __cplusplus
}
#endif
