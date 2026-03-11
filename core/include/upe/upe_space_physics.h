#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Orbital Mechanics & Space Physics
  
  - Keplerian orbits
  - N-body gravity
  - Patched conics (sphere of influence transitions)
  - Atmospheric drag and re-entry heating
  - Lagrange points
  - Tidal forces
*/

typedef struct UpeOrbitElements {
    float semi_major_axis;       // a (m)
    float eccentricity;          // e (0 = circle, 0-1 = ellipse, 1 = parabola, >1 = hyperbola)
    float inclination;           // i (rad)
    float longitude_of_ascending_node; // Ω (rad)
    float argument_of_periapsis; // ω (rad)
    float true_anomaly;          // ν (rad) - position in orbit
} UpeOrbitElements;

typedef struct UpeCelestialBody {
    UpeVec3 position;
    UpeVec3 velocity;
    float mass;                  // kg
    float radius;                // m
    float gravitational_parameter; // μ = GM (m³/s²)
    float sphere_of_influence;   // Radius where this body's gravity dominates
    float atmosphere_height;     // km
    float atmosphere_density_sea_level; // kg/m³
    float temperature_surface;   // K
    uint32_t parent_body;        // For hierarchical systems (0xFFFFFFFF = none)
} UpeCelestialBody;

typedef struct UpeSpaceSystem {
    UpeCelestialBody *bodies;
    uint32_t body_count;
    uint32_t body_capacity;
    
    // Universal constants
    float gravitational_constant; // G = 6.674e-11 m³/(kg·s²)
    float speed_of_light;        // c = 299792458 m/s
    
    // Spacecraft tracking
    uint32_t *spacecraft_ids;
    uint32_t spacecraft_count;
    
} UpeSpaceSystem;

void UpeSpace_Init(UpeSpaceSystem *space, uint32_t max_bodies);
void UpeSpace_Destroy(UpeSpaceSystem *space);

// Body management
uint32_t UpeSpace_AddBody(UpeSpaceSystem *space, UpeVec3 position, UpeVec3 velocity, float mass, float radius);
void UpeSpace_SetParentBody(UpeSpaceSystem *space, uint32_t body_id, uint32_t parent_id);

// Orbital mechanics
UpeOrbitElements UpeSpace_ComputeOrbitElements(UpeVec3 position, UpeVec3 velocity, float mu);
void UpeSpace_OrbitElementsToState(const UpeOrbitElements *elements, float mu,
                                   UpeVec3 *out_position, UpeVec3 *out_velocity);

// Gravity
UpeVec3 UpeSpace_GravityAt(const UpeSpaceSystem *space, UpeVec3 position);
void UpeSpace_ComputeNBodyGravity(UpeSpaceSystem *space);

// Atmospheric drag
float UpeSpace_AtmosphericDensity(const UpeCelestialBody *planet, float altitude);
UpeVec3 UpeSpace_DragForce(float density, UpeVec3 velocity, float drag_coeff, float cross_section_area);

// Re-entry heating (simplified)
float UpeSpace_ReentryHeatingRate(float velocity, float density); // Watts

// Lagrange points
void UpeSpace_ComputeLagrangePoints(const UpeCelestialBody *primary, const UpeCelestialBody *secondary,
                                   UpeVec3 lagrange_points[5]);

// Sphere of influence test
uint32_t UpeSpace_GetDominantBody(const UpeSpaceSystem *space, UpeVec3 position);

// Transfer orbits (Hohmann, bi-elliptic)
typedef struct UpeTransferOrbit {
    float delta_v_1;             // First burn (m/s)
    float delta_v_2;             // Second burn (m/s)
    float transfer_time;         // seconds
    UpeVec3 burn_1_position;
    UpeVec3 burn_2_position;
} UpeTransferOrbit;

UpeTransferOrbit UpeSpace_PlanHohmannTransfer(float r1, float r2, float mu);

#ifdef __cplusplus
}
#endif
