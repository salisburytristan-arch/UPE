#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Advanced Vehicle Systems
  
  - Tank tracks (continuous tracks)
  - Differential steering
  - Advanced drivetrains (AWD, torque vectoring)
  - Suspension tuning (anti-roll bars)
*/

// ========================
// Tank Tracks
// ========================

typedef struct UpeTrackSegment {
    UpeVec3 position;
    UpeVec3 velocity;
    float angle;                 // Rotation around track
    uint32_t ground_contact;     // 1 if touching ground
} UpeTrackSegment;

typedef struct UpeTrackedVehicle {
    // Left track
    UpeTrackSegment *left_segments;
    uint32_t left_segment_count;
    float left_velocity;         // Linear velocity of track
    
    // Right track
    UpeTrackSegment *right_segments;
    uint32_t right_segment_count;
    float right_velocity;
    
    // Vehicle body (rigid body index or pointer)
    uint32_t body_id;
    
    // Track properties
    float track_width;           // Distance between tracks
    float segment_length;
    float friction_coefficient;
    float max_velocity;
    
} UpeTrackedVehicle;

void UpeTrackedVehicle_Init(UpeTrackedVehicle *vehicle, uint32_t segments_per_track, float track_width);
void UpeTrackedVehicle_Destroy(UpeTrackedVehicle *vehicle);

void UpeTrackedVehicle_SetThrottle(UpeTrackedVehicle *vehicle, float left_throttle, float right_throttle);
void UpeTrackedVehicle_Step(UpeTrackedVehicle *vehicle, float dt);

// Track forces (traction, friction)
void UpeTrackedVehicle_ComputeForces(UpeTrackedVehicle *vehicle, UpeVec3 *force, UpeVec3 *torque);

// ========================
// Advanced Drivetrain
// ========================

typedef enum UpeDriveMode {
    UPE_DRIVE_FWD = 0,           // Front-wheel drive
    UPE_DRIVE_RWD = 1,           // Rear-wheel drive
    UPE_DRIVE_AWD = 2,           // All-wheel drive
    UPE_DRIVE_4WD = 3            // Four-wheel drive (lockable)
} UpeDriveMode;

typedef struct UpeDrivetrain {
    UpeDriveMode mode;
    
    // Torque distribution
    float front_bias;            // 0..1 (0 = all rear, 1 = all front)
    float left_bias;             // For torque vectoring
    
    // Center differential
    float center_diff_lock;      // 0..1 (0 = open, 1 = locked)
    
    // Transmission
    float gear_ratios[8];
    uint32_t gear_count;
    uint32_t current_gear;
    float clutch_engagement;     // 0..1
    
    // Engine
    float engine_rpm;
    float engine_torque;         // Current torque output
    float max_torque;
    float max_rpm;
    float idle_rpm;
    
} UpeDrivetrain;

void UpeDrivetrain_Init(UpeDrivetrain *drivetrain);
void UpeDrivetrain_SetMode(UpeDrivetrain *drivetrain, UpeDriveMode mode);
void UpeDrivetrain_SetThrottle(UpeDrivetrain *drivetrain, float throttle);
void UpeDrivetrain_ShiftGear(UpeDrivetrain *drivetrain, int32_t delta);  // +1 or -1

// Compute wheel torques
void UpeDrivetrain_ComputeWheelTorques(const UpeDrivetrain *drivetrain, float wheel_rpms[4], float out_torques[4]);

// Engine simulation
void UpeDrivetrain_UpdateEngine(UpeDrivetrain *drivetrain, float dt, float throttle_input);

// ========================
// Advanced Suspension
// ========================

typedef struct UpeSuspensionWheel {
    UpeVec3 attachment_point;    // Relative to chassis
    UpeVec3 suspension_axis;     // Direction of travel
    
    float spring_stiffness;
    float damping;
    float max_travel;
    
    float compression;           // Current compression (0..max_travel)
    float compression_velocity;
    
    // Anti-roll bar connection
    uint32_t antiroll_pair;      // Index of opposite wheel (-1 if none)
    float antiroll_stiffness;
    
} UpeSuspensionWheel;

typedef struct UpeAdvancedSuspension {
    UpeSuspensionWheel wheels[4];
    
    // Global suspension settings
    float ride_height;
    float roll_stiffness;        // Anti-roll bar stiffness
    float pitch_stiffness;
    
} UpeAdvancedSuspension;

void UpeSuspension_Init(UpeAdvancedSuspension *susp);
void UpeSuspension_ComputeForces(const UpeAdvancedSuspension *susp, UpeVec3 wheel_forces[4]);
void UpeSuspension_UpdateCompression(UpeAdvancedSuspension *susp, float dt, const float ground_heights[4]);

#ifdef __cplusplus
}
#endif
