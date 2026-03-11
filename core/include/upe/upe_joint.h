#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum UpeJointType {
    UPE_JOINT_HINGE,
    UPE_JOINT_SLIDER,
    UPE_JOINT_BALL,
    UPE_JOINT_6DOF,
    UPE_JOINT_DISTANCE,
    UPE_JOINT_SPRING,
    UPE_JOINT_CONE_TWIST,
    UPE_JOINT_GEAR,
    UPE_JOINT_PULLEY,
    UPE_JOINT_UNIVERSAL   /* Two perpendicular hinge axes - no axial rotation */
} UpeJointType;

struct UpeJoint;
typedef void (*UpeJointBreakCallback)(struct UpeJoint *joint, void *user_data);

typedef struct UpeJointDesc {
    UpeJointType type;
    uint32_t body_a;
    uint32_t body_b;
    UpeVec3 anchor_a;
    UpeVec3 anchor_b;
    UpeVec3 axis;
    float motor_target_velocity;
    float motor_max_force;
    int enable_motor;
    float lower_limit;
    float upper_limit;
    int enable_limit;
    // Distance/Spring joint parameters
    float distance;         // Rest distance for distance/spring joints
    float min_distance;     // Minimum distance for distance joint
    float max_distance;     // Maximum distance for distance joint
    float stiffness;        // Spring constant (k) for spring joints
    float damping;          // Damping coefficient (c) for spring joints
    float cone_angle;       // Cone swing limit (radians) for cone-twist
    float twist_min;        // Twist lower limit (radians)
    float twist_max;        // Twist upper limit (radians)
    float gear_ratio;       // Gear ratio for gear joints
    float pulley_ratio;     // Pulley ratio for pulley joints
    float pulley_length;    // Total rope length for pulley joints
    float max_impulse;       // Break threshold (0 disables breakage)
    UpeJointBreakCallback on_break;
    void *break_user_data;
} UpeJointDesc;

typedef struct UpeJoint {
    UpeJointType type;
    uint32_t body_a;
    uint32_t body_b;
    UpeVec3 anchor_a;
    UpeVec3 anchor_b;
    UpeVec3 axis;
    float lambda;
    float motor_target_velocity;
    float motor_max_force;
    int enable_motor;
    float lower_limit;
    float upper_limit;
    int enable_limit;
    float current_angle;
    // Distance/Spring joint parameters
    float distance;         // Rest distance
    float min_distance;     // Minimum distance for distance joint
    float max_distance;     // Maximum distance for distance joint
    float stiffness;        // Spring constant (k)
    float damping;          // Damping coefficient (c)
    float lambda_distance;  // Accumulated impulse for distance constraint
    float cone_angle;
    float twist_min;
    float twist_max;
    float gear_ratio;
    float pulley_ratio;
    float pulley_length;
    float max_impulse;       // Break threshold (0 disables breakage)
    float accumulated_impulse;
    int broken;
    UpeJointBreakCallback on_break;
    void *break_user_data;
} UpeJoint;

void UpeJoint_Init(UpeJoint *joint, const UpeJointDesc *desc);
void UpeJoint_Solve(UpeJoint *joint, struct UpeBodyState *bodies, uint32_t body_count);

#ifdef __cplusplus
}
#endif
