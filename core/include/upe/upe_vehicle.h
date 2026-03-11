#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_joint.h"

struct UpeBody;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeVehicleDesc {
    uint32_t chassis_body;
    float wheel_radius;
    float wheel_width;
    float suspension_stiffness;
    float suspension_damping;
    uint32_t wheel_count;
} UpeVehicleDesc;

typedef struct UpeWheel {
    uint32_t body_id;
    UpeVec3 connection_point;        // Position on chassis (local space)
    UpeVec3 steering_axis;           // Steering direction
    UpeVec3 wheel_direction;         // Down direction for suspension raycast
    float radius;                    // Wheel radius
    float suspension_length;         // Max suspension travel
    float suspension_stiffness;      // Spring constant (N/m)
    float suspension_damping;        // Damping coefficient (Ns/m)
    float suspension_force;          // Current suspension force
    float suspension_velocity;       // Compression velocity (for damping)
    float compression_previous;      // Previous frame compression
    UpeVec3 contact_normal;
    UpeVec3 contact_point;           // World contact position
    int is_touching;                 // 1 if in contact with ground
    float grip_limit;                // Maximum grip force before slipping
    
    /* Tire model state (Pacejka) */
    float slip_angle;                // Lateral slip angle (radians)
    float slip_ratio;                // Longitudinal slip ratio (-1 to 1)
    float lateral_force;             // Lateral tire force (N)
    float longitudinal_force;        // Longitudinal tire force (N)
    float angular_velocity;          // Wheel spin rate (rad/s)
    float steering_angle;            // Current steering angle (radians)
} UpeWheel;

typedef struct UpeVehicle {
    uint32_t chassis_body;
    UpeWheel wheels[4];
    uint32_t wheel_count;
    float suspension_stiffness;      // Spring constant (N/m)
    float suspension_damping;        // Damping coefficient (Ns/m)
    float suspension_cfm;            // Soft constraint compliance (0=rigid, >0=soft)
    float wheel_friction;            // Friction coefficient for grip
    float drive_train_torque;        // Motor torque applied to wheels
    int use_cfm_suspension;          // 1=use CFM soft suspension, 0=rigid
} UpeVehicle;

void UpeVehicle_Init(UpeVehicle *vehicle, const UpeVehicleDesc *desc);
void UpeVehicle_UpdateSuspension(UpeVehicle *vehicle, UpeBody *chassis, float dt);
void UpeVehicle_ApplyInput(UpeVehicle *vehicle, float steering_angle, float drive_power, float brake_power);
float UpeVehicle_GetWheelCompressionRatio(const UpeVehicle *vehicle, uint32_t wheel_idx);

/* Tire model functions */
float UpeVehicle_CalculatePacejkaLateral(float slip_angle, float load, float friction);
float UpeVehicle_CalculatePacejkaLongitudinal(float slip_ratio, float load, float friction);
void UpeVehicle_UpdateTireForces(UpeVehicle *vehicle, UpeBody *chassis, float dt);
void UpeVehicle_ApplyAckermannSteering(UpeVehicle *vehicle, float steering_input, float wheelbase, float track_width);

#ifdef __cplusplus
}
#endif
