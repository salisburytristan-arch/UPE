#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeWorld UpeWorld;

typedef enum UpeGravityFieldType {
    UPE_GRAVITY_FIELD_POINT_ATTRACT = 0,
    UPE_GRAVITY_FIELD_POINT_REPEL = 1
} UpeGravityFieldType;

typedef struct UpeGravityFieldDesc {
    UpeVec3 position;
    float strength;
    float min_distance;
    float max_distance;
    float falloff_exponent;
    int enabled;
    UpeGravityFieldType type;
} UpeGravityFieldDesc;

typedef struct UpeGravityFieldInfo {
    uint32_t id;
    UpeGravityFieldDesc desc;
} UpeGravityFieldInfo;

UpeGravityFieldDesc UpeGravityFieldDesc_Default(void);

uint32_t UpeWorld_AddGravityField(UpeWorld *world, const UpeGravityFieldDesc *desc);
int UpeWorld_RemoveGravityField(UpeWorld *world, uint32_t field_id);
void UpeWorld_ClearGravityFields(UpeWorld *world);
uint32_t UpeWorld_GetGravityFieldCount(const UpeWorld *world);
uint32_t UpeWorld_GetGravityFields(const UpeWorld *world, UpeGravityFieldInfo *out_fields, uint32_t max_fields);

/* NFW dark matter halo density profile  rho(r) = rho_s / ((r/r_s)(1+r/r_s)^2) */
float UpeGravity_NFWDensity(float r, float rho_s, float r_s);
float UpeGravity_NFWEnclosedMass(float r, float rho_s, float r_s);

/* Point mass force with softening to avoid singularity */
float UpeGravity_SingularityForce(float m1, float m2, float r, float G, float softening);

/* Schwarzschild event horizon radius (alias convenience — avoids including relativistic header) */
float UpeGravity_SingularityEventHorizonRadius(float mass, float G, float c);

#ifdef __cplusplus
}
#endif
