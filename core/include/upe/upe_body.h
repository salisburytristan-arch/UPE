#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum UpeBodyType {
    UPE_BODY_STATIC,
    UPE_BODY_KINEMATIC,
    UPE_BODY_DYNAMIC
} UpeBodyType;

typedef struct UpeBodyDesc {
    UpeBodyType type;
    UpeVec3 position;
    UpeQuat rotation;
    float mass;
    uint32_t collision_layer;   // Which layer this body is on (0-31)
    uint32_t collision_mask;    // Which layers this body collides with (bitmask)
} UpeBodyDesc;

typedef struct UpeBody {
    uint32_t id;
    UpeBodyType type;
    UpeVec3 position;
    UpeVec3 velocity;
    UpeQuat rotation;
    UpeVec3 angular_velocity;
    float mass;
    float inv_mass;
    UpeVec3 inv_inertia;
    UpeVec3 force;
    UpeVec3 torque;
    int awake;
    uint32_t collision_layer;   // Which layer this body is on
    uint32_t collision_mask;    // Collision filter bitmask
    uint32_t first_shape_index;  // Head of shape linked list (0xFFFFFFFF if none)
} UpeBody;

void UpeBody_Init(UpeBody *body, uint32_t id, const UpeBodyDesc *desc);
void UpeBody_ApplyForce(UpeBody *body, UpeVec3 force);
void UpeBody_ApplyTorque(UpeBody *body, UpeVec3 torque);
void UpeBody_ApplyImpulse(UpeBody *body, UpeVec3 impulse);
void UpeBody_ClearForces(UpeBody *body);

/* Rigid body rotational dynamics helpers */
UpeVec3 UpeBody_GyroscopicTorque(UpeVec3 omega, float Ix, float Iy, float Iz);
UpeVec3 UpeBody_CoriolisAccel(UpeVec3 vel, UpeVec3 omega_frame);

/* Utility setters */
void UpeBody_SetDensity(UpeBody *body, float density);   /* Recomputes mass with implicit unit volume */
int  UpeBody_ScaleUniform(UpeBody *body, float scale);   /* Scales inverse inertia for uniform size change */

#ifdef __cplusplus
}
#endif
