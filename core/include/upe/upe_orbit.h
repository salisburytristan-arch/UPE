#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ORBITAL MECHANICS HEADER
//
// - Two-body Kepler solver with eccentric anomaly
// - Orbital elements representation (a, e, i, Ω, ω, M)
// - N-body gravity with direct summation
// - Symplectic Leapfrog integrator for energy conservation
// ============================================================================

typedef struct UpeOrbitalElements {
    float semi_major_axis;      // a (meters)
    float eccentricity;         // e (0 = circular, <1 = ellipse, 1 = parabola)
    float inclination;          // i (radians)
    float longitude_ascending;  // Ω (radians)
    float argument_periapsis;   // ω (radians)
    float mean_anomaly;         // M (radians)
} UpeOrbitalElements;

typedef struct UpeOrbitalBody {
    UpeVec3 position;
    UpeVec3 velocity;
    float mass;
    float radius;
} UpeOrbitalBody;

typedef struct UpeNBodySystem {
    UpeOrbitalBody *bodies;
    uint32_t body_count;
    uint32_t body_capacity;
    float dt;
    float softening;
} UpeNBodySystem;

// Legacy API (backward compatible)
typedef struct UpeOrbit {
    UpeVec3 position;
    UpeVec3 velocity;
    float semi_major_axis;
    float eccentricity;
    float inclination;
} UpeOrbit;

void UpeOrbit_Init(UpeOrbit *orbit, UpeVec3 position, UpeVec3 velocity);
void UpeOrbit_Step(UpeOrbit *orbit, float dt, float central_mass);
void UpeOrbit_FromElements(UpeOrbit *orbit, float a, float e, float i);

// Kepler solver: Elements → Position/Velocity
void UpeOrbit_ElementsToStateVector(const UpeOrbitalElements *elem, float mass_primary,
                                     UpeVec3 *out_pos, UpeVec3 *out_vel);

// N-body system management
void UpeNBody_Init(UpeNBodySystem *system, uint32_t max_bodies);
void UpeNBody_Destroy(UpeNBodySystem *system);
uint32_t UpeNBody_AddBody(UpeNBodySystem *system, UpeVec3 pos, UpeVec3 vel, float mass, float radius);

// N-body simulation
void UpeNBody_Step(UpeNBodySystem *system, float dt);

// Conservation tracking
float UpeNBody_ComputeEnergy(const UpeNBodySystem *system);
UpeVec3 UpeNBody_ComputeAngularMomentum(const UpeNBodySystem *system);

#ifdef __cplusplus
}
#endif
