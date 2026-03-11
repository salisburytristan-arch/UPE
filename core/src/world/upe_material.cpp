#include "upe/upe_material.h"

void UpeMaterial_Init(UpeMaterial *material, float static_friction, float dynamic_friction, float restitution) {
    if (!material) {
        return;
    }
    material->static_friction = static_friction;
    material->dynamic_friction = dynamic_friction;
    material->restitution = restitution;
}

void UpeMaterial_Apply(const UpeMaterial *material, UpeBody *body_a, UpeBody *body_b) {
    if (!material || !body_a || !body_b) {
        return;
    }
}
