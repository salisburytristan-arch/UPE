#pragma once

#include "upe/upe_joint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeJointLimit {
    float min_value;
    float max_value;
    float stiffness;
    int enabled;
} UpeJointLimit;

typedef struct UpeJointMotor {
    float target_velocity;
    float max_force;
    int enabled;
} UpeJointMotor;

void UpeJointLimit_Init(UpeJointLimit *limit, float min_val, float max_val);
void UpeJointMotor_Init(UpeJointMotor *motor, float target_vel, float max_force);
void UpeJointLimit_Apply(const UpeJointLimit *limit, float current, float *correction);
void UpeJointMotor_Apply(const UpeJointMotor *motor, float current_vel, float *force);

#ifdef __cplusplus
}
#endif
