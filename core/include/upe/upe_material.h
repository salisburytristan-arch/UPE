#pragma once

#include "upe/upe_body.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeMaterial {
    float static_friction;
    float dynamic_friction;
    float restitution;
} UpeMaterial;

void UpeMaterial_Init(UpeMaterial *material, float static_friction, float dynamic_friction, float restitution);
void UpeMaterial_Apply(const UpeMaterial *material, UpeBody *body_a, UpeBody *body_b);

#ifdef __cplusplus
}
#endif
