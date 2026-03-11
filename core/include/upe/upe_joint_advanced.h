#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Advanced Joint Types
  
  Extended beyond basic hinges/sliders to support:
  - Ragdolls (cone-twist, swing-twist limits)
  - Vehicles (differentials, steering, suspension)
  - Machinery (gears, rack-pinion, pulleys)
*/

typedef enum UpeJointType {
    UPE_JOINT_FIXED = 0,
    UPE_JOINT_HINGE,
    UPE_JOINT_SLIDER,
    UPE_JOINT_BALL_AND_SOCKET,
    UPE_JOINT_UNIVERSAL,
    UPE_JOINT_CONE_TWIST,        // Ragdoll shoulders
    UPE_JOINT_SWING_TWIST,       // Ragdoll hips
    UPE_JOINT_GEAR,              // Gear ratio constraint
    UPE_JOINT_RACK_PINION,       // Linear-rotational coupling
    UPE_JOINT_PULLEY,            // Cable/rope constraint
    UPE_JOINT_DISTANCE,          // Distance limit
    UPE_JOINT_SPRING,            // Spring-damper
    UPE_JOINT_MOTOR              // Active motor with target
} UpeJointType;

typedef struct UpeJointLimits {
    float min_angle;             // Radians (for angular joints)
    float max_angle;
    float min_distance;          // Meters (for linear joints)
    float max_distance;
    float stiffness;             // Limit stiffness (soft limits)
    float damping;
} UpeJointLimits;

typedef struct UpeJointMotor {
    int enabled;
    float target_velocity;       // rad/s or m/s
    float max_force;             // Maximum motor force/torque
    float target_position;       // For position servos
    float position_gain;         // PD control gain
    float velocity_gain;
} UpeJointMotor;

typedef struct UpeJointDesc {
    UpeJointType type;
    uint32_t body_a;             // Body handle (0xFFFFFFFF = world)
    uint32_t body_b;
    
    UpeVec3 anchor_a;            // Local anchor on body A
    UpeVec3 anchor_b;            // Local anchor on body B
    UpeVec3 axis_a;              // Local axis on body A
    UpeVec3 axis_b;              // Local axis on body B (for universal/etc)
    
    UpeJointLimits limits;
    UpeJointMotor motor;
    
    float breaking_force;        // Force threshold for breakable joints
    float breaking_torque;
    
    // Type-specific parameters
    union {
        struct {
            float gear_ratio;
        } gear;
        
        struct {
            float ratio;         // Pulley ratio
            float cable_length;
        } pulley;
        
        struct {
            float cone_angle;    // Swing cone limit (radians)
            float twist_min;     // Twist limits
            float twist_max;
        } cone_twist;
        
        struct {
            float swing_y_min;   // Asymmetric swing limits
            float swing_y_max;
            float swing_z_min;
            float swing_z_max;
            float twist_min;
            float twist_max;
        } swing_twist;
        
        struct {
            float stiffness;
            float damping;
            float rest_length;
        } spring;
    };
} UpeJointDesc;

typedef struct UpeJointState {
    float current_angle[3];      // Current joint angles (XYZ)
    float current_distance;      // Current linear distance
    float current_velocity[3];   // Angular or linear velocity
    float applied_force[3];      // Forces applied by joint
    float applied_torque[3];
    int broken;                  // 1 if joint exceeded breaking threshold
} UpeJointState;

// Joint management (world-level, implementation in upe_world.cpp)
uint32_t UpeJoint_Create(void *world, const UpeJointDesc *desc);
void UpeJoint_Destroy(void *world, uint32_t joint_id);
void UpeJoint_GetState(void *world, uint32_t joint_id, UpeJointState *state);
void UpeJoint_SetMotor(void *world, uint32_t joint_id, const UpeJointMotor *motor);
void UpeJoint_SetLimits(void *world, uint32_t joint_id, const UpeJointLimits *limits);

#ifdef __cplusplus
}
#endif
