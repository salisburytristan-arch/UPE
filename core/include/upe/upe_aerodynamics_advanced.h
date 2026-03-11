#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Advanced Aerodynamics
  
  Beyond basic drag/lift - full 6DOF flight dynamics:
  - Blade element theory (helicopters, propellers)
  - Control surfaces (ailerons, elevators, rudders)
  - Downforce and ground effect
  - Wake/vortex interactions
  - Supersonic effects
*/

typedef struct UpeAeroSurface {
    UpeVec3 position;            // Local position on body
    UpeVec3 normal;              // Surface normal
    float area;                  // m²
    float chord;                 // Chord length (m)
    float aspect_ratio;          // Wingspan² / wing_area
    float deflection_angle;      // Control surface deflection (rad)
    float lift_slope;            // dCL/dα (per radian)
    float zero_lift_angle;       // α₀ (rad)
    float stall_angle_positive;  // Positive stall angle
    float stall_angle_negative;  // Negative stall angle
} UpeAeroSurface;

typedef struct UpeAeroBody {
    uint32_t body_id;
    
    UpeAeroSurface *surfaces;    // Wings, control surfaces
    uint32_t surface_count;
    
    // Drag model
    float drag_coefficient;      // Cd
    float frontal_area;          // m²
    float side_area;
    
    // Propulsion
    UpeVec3 thrust_position;
    UpeVec3 thrust_direction;
    float thrust_force;          // N
    
    // Engine
    float engine_power;          // W
    float propeller_diameter;    // m
    float propeller_pitch;       // m (advance per revolution)
    float rpm;                   // Revolutions per minute
    
    // Ground effect
    float ground_effect_height;  // Height where ground effect starts
    float downforce_coefficient;
} UpeAeroBody;

typedef struct UpeAeroSystem {
    UpeAeroBody *bodies;
    uint32_t body_count;
    uint32_t body_capacity;
    
    // Environment
    float air_density;           // kg/m³ (1.225 at sea level)
    UpeVec3 wind_velocity;
    float speed_of_sound;        // m/s (343 at sea level)
    float temperature;           // K
    float pressure;              // Pa
} UpeAeroSystem;

void UpeAero_Init(UpeAeroSystem *aero, uint32_t max_bodies);
void UpeAero_Destroy(UpeAeroSystem *aero);

uint32_t UpeAero_RegisterBody(UpeAeroSystem *aero, uint32_t body_id);
void UpeAero_AddSurface(UpeAeroSystem *aero, uint32_t aero_id, const UpeAeroSurface *surface);

// Force calculation
void UpeAero_ComputeForces(UpeAeroSystem *aero, uint32_t aero_id,
                          UpeVec3 velocity, UpeVec3 angular_vel,
                          UpeVec3 *out_force, UpeVec3 *out_torque);

// Lift coefficient from angle of attack
float UpeAero_LiftCoefficient(const UpeAeroSurface *surface, float angle_of_attack);

// Induced drag (from lift)
float UpeAero_InducedDrag(float lift_coeff, float aspect_ratio);

// Propeller thrust (blade element theory approximation)
float UpeAero_PropellerThrust(float rpm, float diameter, float pitch, 
                             float air_density, float forward_velocity);

// Ground effect multiplier (increases downforce near ground)
float UpeAero_GroundEffectFactor(float height, float reference_height);

#ifdef __cplusplus
}
#endif
