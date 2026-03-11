#pragma once

#include <stdint.h>
#include "upe/upe_body.h"
#include "upe/upe_shape.h"
#include "upe/upe_joint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeWorld UpeWorld;
typedef struct UpeReplayRecorder UpeReplayRecorder;
typedef struct UpeProfiler UpeProfiler;

uint32_t UpeWorld_CreateBody(UpeWorld *world, const UpeBodyDesc *desc);
void UpeWorld_DestroyBody(UpeWorld *world, uint32_t body_id);
UpeBody *UpeWorld_GetBody(UpeWorld *world, uint32_t body_id);
uint32_t UpeWorld_CreateShape(UpeWorld *world, const UpeShapeDesc *desc);
void UpeWorld_DestroyShape(UpeWorld *world, uint32_t shape_id);

void UpeWorld_SetReplayRecorder(UpeWorld *world, UpeReplayRecorder *replay);
void UpeWorld_SetProfiler(UpeWorld *world, UpeProfiler *profiler);
void UpeWorld_ApplyForce(UpeWorld *world, uint32_t body_id, UpeVec3 force);
void UpeWorld_ApplyTorque(UpeWorld *world, uint32_t body_id, UpeVec3 torque);
void UpeWorld_ApplyImpulse(UpeWorld *world, uint32_t body_id, UpeVec3 impulse);

/* Joint/constraint helpers (articulation, ragdoll, etc) */
uint32_t UpeWorld_AddJoint(UpeWorld *world, const UpeJointDesc *desc);
uint32_t UpeWorld_GetJointCount(UpeWorld *world);

#ifdef __cplusplus
}
#endif
