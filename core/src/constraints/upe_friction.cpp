#include "upe/upe_friction.h"

static float UpeClamp(float v, float min_v, float max_v) {
    if (v < min_v) return min_v;
    if (v > max_v) return max_v;
    return v;
}

void UpeFriction_Solve(UpeBodyState *bodies, uint32_t body_count,
                       UpeFrictionConstraint *constraints, uint32_t constraint_count) {
    if (!bodies || !constraints) {
        return;
    }

    for (uint32_t i = 0; i < constraint_count; ++i) {
        UpeFrictionConstraint &c = constraints[i];
        if (c.body_a >= body_count || c.body_b >= body_count) {
            continue;
        }

        UpeBodyState &a = bodies[c.body_a];
        UpeBodyState &b = bodies[c.body_b];

        UpeVec3 rel_vel = UpeVec3_Sub(b.velocity, a.velocity);
        float vel_along_tangent = UpeVec3_Dot(rel_vel, c.tangent);

        float effective_mass = a.inv_mass + b.inv_mass;
        if (effective_mass < 1.0e-6f) {
            continue;
        }

        float lambda = -vel_along_tangent / effective_mass;
        float old_lambda = c.lambda;
        float max_friction = c.friction_coeff * 100.0f;
        c.lambda = UpeClamp(old_lambda + lambda, -max_friction, max_friction);
        float delta_lambda = c.lambda - old_lambda;

        UpeVec3 impulse = UpeVec3_Scale(c.tangent, delta_lambda);
        a.velocity = UpeVec3_Sub(a.velocity, UpeVec3_Scale(impulse, a.inv_mass));
        b.velocity = UpeVec3_Add(b.velocity, UpeVec3_Scale(impulse, b.inv_mass));
    }
}

float UpeFriction_RollingResistanceTorque(float normal_force, float Crr, float wheel_radius) {
    if (normal_force <= 0.0f || Crr <= 0.0f || wheel_radius <= 0.0f) return 0.0f;
    return normal_force * Crr * wheel_radius;
}
