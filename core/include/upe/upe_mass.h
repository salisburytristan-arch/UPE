#pragma once

#include "upe/upe_math.h"
#include "upe/upe_geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeMassProperties {
    float mass;
    UpeVec3 center_of_mass;
    UpeMat3 inertia_tensor;
} UpeMassProperties;

UpeMassProperties UpeMassProperties_FromSphere(float density, float radius);
UpeMassProperties UpeMassProperties_FromBox(float density, UpeVec3 half_extents);
UpeMassProperties UpeMassProperties_FromCapsule(float density, float radius, float height);

#ifdef __cplusplus
}
#endif
