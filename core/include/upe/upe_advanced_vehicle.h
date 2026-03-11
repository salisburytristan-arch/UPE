#pragma once

#include <stdint.h>
#include "upe/upe_body.h"

#ifdef __cplusplus
extern "C" {
#endif

// Transmission types
typedef enum UpeTransmissionType {
    UPE_TRANSMISSION_AUTOMATIC,
    UPE_TRANSMISSION_MANUAL
} UpeTransmissionType;

// Manual transmission shifts
typedef struct UpeManualTransmission {
    int current_gear;              // -1=reverse, 0=neutral, 1-6=forward gears
    float gear_ratios[6];          // RPM multipliers for each forward gear
    float reverse_ratio;           // Reverse gear ratio (negative)
    float final_drive_ratio;       // Final drive reduction
} UpeManualTransmission;

// Automatic transmission parameters
typedef struct UpeAutomaticTransmission {
    int current_gear;              // -1=reverse, 0=neutral, 1-6=forward gears
    float gear_ratios[6];          // RPM multipliers for each gear
    float reverse_ratio;
    float final_drive_ratio;
    float shift_up_rpm;            // RPM threshold to shift up
    float shift_down_rpm;          // RPM threshold to shift down
    float shift_time;              // Time in seconds for shift animation
    float shift_timer;             // Current shift progress (0-1)
    int is_shifting;               // 1 if currently in gear change
} UpeAutomaticTransmission;

// Complete transmission system
typedef struct UpeTransmission {
    UpeTransmissionType type;
    float engine_power;            // Maximum engine power (watts)
    float engine_torque;           // Maximum engine torque (N⋅m)
    float engine_rpm;              // Current engine RPM (0-8000 typical)
    float engine_max_rpm;          // Redline RPM
    float wheel_rpm;               // Current wheel rotation speed
    union {
        UpeManualTransmission manual;
        UpeAutomaticTransmission automatic;
    };
    float clutch_engagement;       // 0=disengaged, 1=fully engaged (for launch control)
    float torque_output;           // Current torque being delivered to wheels (N⋅m)
} UpeTransmission;

// Tire model - Pacejka's magic formula simplified
typedef struct UpeTireModel {
    float longitudinal_grip;       // Grip coefficient in acceleration direction (0-1.5)
    float lateral_grip;            // Grip coefficient in lateral direction (0-1.2)
    float grip_threshold;          // Slip threshold before tire slides (0.05-0.2)
    float longitudinal_slip;       // Current slip in acceleration (0-2.0)
    float lateral_slip;            // Current slip in steering (degrees, 0-30)
    float slide_factor;            // 0=no slide, 1=full powerslide
    float temperature;             // Tire temperature (affects grip: 0.5-1.5 multiplier)
    float wear;                    // Tire wear (1.0=new, 0.5=worn, affects grip)
} UpeTireModel;

// Wheel with advanced physics
typedef struct UpeAdvancedWheel {
    uint32_t body_id;
    float radius;
    float load;                    // Vertical load (N) - affects grip
    float drive_torque;            // Applied drive torque
    float brake_torque;            // Applied brake torque
    float steering_angle;          // Steering angle (radians)
    float rotational_velocity;     // Angular velocity (rad/s)
    UpeTireModel tire;
    float suspension_compression;  // 0-1 ratio
    int is_grounded;
} UpeAdvancedWheel;

// Complete advanced vehicle
typedef struct UpeAdvancedVehicle {
    uint32_t chassis_body;
    UpeAdvancedWheel wheels[4];
    uint32_t wheel_count;
    
    // Transmission
    UpeTransmission transmission;
    
    // Input state
    float throttle_input;          // -1.0 to 1.0 (negative = reverse, positive = forward)
    float brake_input;             // 0.0 to 1.0
    float steering_input;          // -1.0 to 1.0 (left to right)
    float handbrake_input;         // 0.0 to 1.0
    
    // Physics parameters
    float max_steering_angle;      // Maximum steering angle (radians)
    float suspension_stiffness;
    float suspension_damping;
    float drag_coefficient;        // Aerodynamic drag
    float downforce_coefficient;   // Aerodynamic downforce (increases grip at speed)
    
    // Stability control
    float traction_control_slip;   // Slip threshold for TCS activation
    int traction_control_enabled;
    int abs_enabled;               // Anti-lock braking
    float abs_threshold;           // Wheel lock threshold
    
    // Performance stats
    float top_speed;               // Maximum velocity (m/s)
    float acceleration;            // Current acceleration (m/s^2)
    float drift_angle;             // Angle between velocity and heading (degrees)
    float drift_score;             // Accumulated drift score for gameplay
} UpeAdvancedVehicle;

// Initialization
void UpeAdvancedVehicle_Init(UpeAdvancedVehicle *vehicle, uint32_t chassis_body, uint32_t wheel_count);

// Transmission control
void UpeTransmission_Init(UpeTransmission *trans, UpeTransmissionType type);
void UpeTransmission_SetGear(UpeTransmission *trans, int gear);
void UpeTransmission_SetThrottle(UpeTransmission *trans, float throttle);
float UpeTransmission_GetTorqueOutput(UpeTransmission *trans);

// Input and update
void UpeAdvancedVehicle_SetInput(UpeAdvancedVehicle *vehicle, float throttle, float steering, float brake, float handbrake);
void UpeAdvancedVehicle_Update(UpeAdvancedVehicle *vehicle, UpeBody *chassis, float dt);

// Tire grip calculation
float UpeTireModel_CalculateGripForce(UpeTireModel *tire, float load, float slip_ratio);
void UpeTireModel_UpdateSlip(UpeTireModel *tire, float wheel_velocity, float chassis_velocity, float steering);

// Drift detection
float UpeAdvancedVehicle_GetDriftAngle(UpeAdvancedVehicle *vehicle);
float UpeAdvancedVehicle_GetDriftScore(UpeAdvancedVehicle *vehicle);

// Stability systems
void UpeAdvancedVehicle_ApplyTractionControl(UpeAdvancedVehicle *vehicle, float dt);
void UpeAdvancedVehicle_ApplyABS(UpeAdvancedVehicle *vehicle);

#ifdef __cplusplus
}
#endif
