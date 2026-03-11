#include "upe/upe_body.h"

void UpeBody_Init(UpeBody *body, uint32_t id, const UpeBodyDesc *desc) {
    if (!body || !desc) {
        return;
    }
    body->id = id;
    body->type = desc->type;
    body->position = desc->position;
    body->velocity = {0.0f, 0.0f, 0.0f};
    body->rotation = desc->rotation;
    body->angular_velocity = {0.0f, 0.0f, 0.0f};
    body->mass = desc->mass;
    body->inv_mass = (desc->mass > 0.0f && desc->type == UPE_BODY_DYNAMIC) ? (1.0f / desc->mass) : 0.0f;
    body->inv_inertia = {1.0f, 1.0f, 1.0f};
    body->force = {0.0f, 0.0f, 0.0f};
    body->torque = {0.0f, 0.0f, 0.0f};
    body->awake = 1;
    body->collision_layer = desc->collision_layer;
    body->collision_mask = desc->collision_mask > 0 ? desc->collision_mask : 0xFFFFFFFF;
    body->first_shape_index = 0xFFFFFFFF;
}

void UpeBody_ApplyForce(UpeBody *body, UpeVec3 force) {
    if (!body) {
        return;
    }
    body->force = UpeVec3_Add(body->force, force);
}

void UpeBody_ApplyTorque(UpeBody *body, UpeVec3 torque) {
    if (!body) {
        return;
    }
    body->torque = UpeVec3_Add(body->torque, torque);
}

void UpeBody_ApplyImpulse(UpeBody *body, UpeVec3 impulse) {
    if (!body || body->type != UPE_BODY_DYNAMIC) {
        return;
    }
    body->velocity = UpeVec3_Add(body->velocity, UpeVec3_Scale(impulse, body->inv_mass));
}

void UpeBody_ClearForces(UpeBody *body) {
    if (!body) {
        return;
    }
    body->force = {0.0f, 0.0f, 0.0f};
    body->torque = {0.0f, 0.0f, 0.0f};
}
