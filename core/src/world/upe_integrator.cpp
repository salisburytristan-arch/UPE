#include "upe/upe_integrator.h"
#include <math.h>

void UpeIntegrator_Euler(UpeBody *body, float dt, UpeVec3 gravity) {
    if (!body || body->type != UPE_BODY_DYNAMIC) {
        return;
    }

    UpeVec3 accel = UpeVec3_Add(gravity, UpeVec3_Scale(body->force, body->inv_mass));
    body->position = UpeVec3_Add(body->position, UpeVec3_Scale(body->velocity, dt));
    body->velocity = UpeVec3_Add(body->velocity, UpeVec3_Scale(accel, dt));

    UpeVec3 ang_accel = {
        body->torque.x * body->inv_inertia.x,
        body->torque.y * body->inv_inertia.y,
        body->torque.z * body->inv_inertia.z
    };
    body->angular_velocity = UpeVec3_Add(body->angular_velocity, UpeVec3_Scale(ang_accel, dt));

    float angle = UpeVec3_Length(body->angular_velocity) * dt;
    if (angle > 1.0e-6f) {
        UpeVec3 axis = UpeVec3_Normalize(body->angular_velocity);
        float half_angle = angle * 0.5f;
        UpeQuat delta_q;
        delta_q.x = axis.x * sinf(half_angle);
        delta_q.y = axis.y * sinf(half_angle);
        delta_q.z = axis.z * sinf(half_angle);
        delta_q.w = cosf(half_angle);
        body->rotation = UpeQuat_Mul(delta_q, body->rotation);
        float len = sqrtf(body->rotation.x * body->rotation.x +
                          body->rotation.y * body->rotation.y +
                          body->rotation.z * body->rotation.z +
                          body->rotation.w * body->rotation.w);
        if (len > 1.0e-6f) {
            body->rotation.x /= len;
            body->rotation.y /= len;
            body->rotation.z /= len;
            body->rotation.w /= len;
        }
    }
}

void UpeIntegrator_SemiImplicit(UpeBody *body, float dt, UpeVec3 gravity) {
    if (!body || body->type != UPE_BODY_DYNAMIC) {
        return;
    }

    UpeVec3 accel = UpeVec3_Add(gravity, UpeVec3_Scale(body->force, body->inv_mass));
    body->velocity = UpeVec3_Add(body->velocity, UpeVec3_Scale(accel, dt));
    body->position = UpeVec3_Add(body->position, UpeVec3_Scale(body->velocity, dt));

    UpeVec3 ang_accel = {
        body->torque.x * body->inv_inertia.x,
        body->torque.y * body->inv_inertia.y,
        body->torque.z * body->inv_inertia.z
    };
    body->angular_velocity = UpeVec3_Add(body->angular_velocity, UpeVec3_Scale(ang_accel, dt));

    float angle = UpeVec3_Length(body->angular_velocity) * dt;
    if (angle > 1.0e-6f) {
        UpeVec3 axis = UpeVec3_Normalize(body->angular_velocity);
        float half_angle = angle * 0.5f;
        UpeQuat delta_q;
        delta_q.x = axis.x * sinf(half_angle);
        delta_q.y = axis.y * sinf(half_angle);
        delta_q.z = axis.z * sinf(half_angle);
        delta_q.w = cosf(half_angle);
        body->rotation = UpeQuat_Mul(delta_q, body->rotation);
        float len = sqrtf(body->rotation.x * body->rotation.x +
                          body->rotation.y * body->rotation.y +
                          body->rotation.z * body->rotation.z +
                          body->rotation.w * body->rotation.w);
        if (len > 1.0e-6f) {
            body->rotation.x /= len;
            body->rotation.y /= len;
            body->rotation.z /= len;
            body->rotation.w /= len;
        }
    }
}
