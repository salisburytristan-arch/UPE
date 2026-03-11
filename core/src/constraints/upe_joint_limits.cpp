#include "upe/upe_joint_limits.h"

void UpeJointLimit_Init(UpeJointLimit *limit, float min_val, float max_val) {
    if (!limit) {
        return;
    }
    limit->min_value = min_val;
    limit->max_value = max_val;
    limit->stiffness = 1.0f;
    limit->enabled = 1;
}

void UpeJointMotor_Init(UpeJointMotor *motor, float target_vel, float max_force) {
    if (!motor) {
        return;
    }
    motor->target_velocity = target_vel;
    motor->max_force = max_force;
    motor->enabled = 1;
}

void UpeJointLimit_Apply(const UpeJointLimit *limit, float current, float *correction) {
    if (!limit || !limit->enabled || !correction) {
        return;
    }
    if (current < limit->min_value) {
        *correction = (limit->min_value - current) * limit->stiffness;
    } else if (current > limit->max_value) {
        *correction = (limit->max_value - current) * limit->stiffness;
    } else {
        *correction = 0.0f;
    }
}

void UpeJointMotor_Apply(const UpeJointMotor *motor, float current_vel, float *force) {
    if (!motor || !motor->enabled || !force) {
        return;
    }
    float error = motor->target_velocity - current_vel;
    *force = error;
    if (*force > motor->max_force) {
        *force = motor->max_force;
    }
    if (*force < -motor->max_force) {
        *force = -motor->max_force;
    }
}
