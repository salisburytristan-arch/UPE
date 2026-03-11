#include "upe/upe_mass.h"
#include <math.h>

static const float UPE_PI = 3.14159265359f;

UpeMassProperties UpeMassProperties_FromSphere(float density, float radius) {
    UpeMassProperties props = {};
    float volume = (4.0f / 3.0f) * UPE_PI * radius * radius * radius;
    props.mass = density * volume;
    props.center_of_mass = {0.0f, 0.0f, 0.0f};

    float i = (2.0f / 5.0f) * props.mass * radius * radius;
    props.inertia_tensor = UpeMat3_Identity();
    props.inertia_tensor.m[0][0] = i;
    props.inertia_tensor.m[1][1] = i;
    props.inertia_tensor.m[2][2] = i;
    return props;
}

UpeMassProperties UpeMassProperties_FromBox(float density, UpeVec3 half_extents) {
    UpeMassProperties props = {};
    float volume = 8.0f * half_extents.x * half_extents.y * half_extents.z;
    props.mass = density * volume;
    props.center_of_mass = {0.0f, 0.0f, 0.0f};

    float dx2 = half_extents.x * half_extents.x;
    float dy2 = half_extents.y * half_extents.y;
    float dz2 = half_extents.z * half_extents.z;

    props.inertia_tensor = UpeMat3_Identity();
    props.inertia_tensor.m[0][0] = (1.0f / 3.0f) * props.mass * (dy2 + dz2);
    props.inertia_tensor.m[1][1] = (1.0f / 3.0f) * props.mass * (dx2 + dz2);
    props.inertia_tensor.m[2][2] = (1.0f / 3.0f) * props.mass * (dx2 + dy2);
    return props;
}

UpeMassProperties UpeMassProperties_FromCapsule(float density, float radius, float height) {
    UpeMassProperties props = {};
    float cyl_volume = UPE_PI * radius * radius * height;
    float sphere_volume = (4.0f / 3.0f) * UPE_PI * radius * radius * radius;
    float volume = cyl_volume + sphere_volume;
    props.mass = density * volume;
    props.center_of_mass = {0.0f, 0.0f, 0.0f};

    float r2 = radius * radius;
    float h2 = height * height;
    float i_x = (1.0f / 12.0f) * props.mass * (3.0f * r2 + h2);
    float i_y = 0.5f * props.mass * r2;

    props.inertia_tensor = UpeMat3_Identity();
    props.inertia_tensor.m[0][0] = i_x;
    props.inertia_tensor.m[1][1] = i_y;
    props.inertia_tensor.m[2][2] = i_x;
    return props;
}
