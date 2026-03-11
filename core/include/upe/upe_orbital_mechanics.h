#pragma once

#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

float UpeOrbital_GravitationalParameter(float gravitational_constant, float central_mass);
float UpeOrbital_CircularOrbitSpeed(float gravitational_parameter, float radius);
float UpeOrbital_EscapeSpeed(float gravitational_parameter, float radius);
float UpeOrbital_VisVivaSpeed(float gravitational_parameter, float radius, float semi_major_axis);
float UpeOrbital_OrbitalPeriod(float gravitational_parameter, float semi_major_axis);
float UpeOrbital_HohmannDeltaV1(float gravitational_parameter, float radius_from, float radius_to);
float UpeOrbital_HohmannDeltaV2(float gravitational_parameter, float radius_from, float radius_to);
float UpeOrbital_MeanMotion(float gravitational_parameter, float semi_major_axis);
float UpeOrbital_SynodicPeriod(float period_a, float period_b);
float UpeOrbital_SphereOfInfluenceRadius(float primary_mass, float secondary_mass, float semi_major_axis);
float UpeOrbital_HillSphereRadius(float primary_mass, float secondary_mass, float semi_major_axis);

UpeVec3 UpeOrbital_CircularVelocityVector(UpeVec3 central_position,
                                          UpeVec3 body_position,
                                          UpeVec3 orbit_normal,
                                          float gravitational_parameter,
                                          int prograde);

int UpeOrbital_ComputeCircularState(UpeVec3 central_position,
                                    UpeVec3 body_position,
                                    UpeVec3 orbit_normal,
                                    float gravitational_parameter,
                                    int prograde,
                                    UpeVec3 *out_velocity);

/* Lagrange points: L1-L5 relative to primary (mu1) and secondary (mu2).
    out_L[5][3] receives [x,y,z] for L1..L5 in the rotating frame. */
void  UpeOrbital_LagrangePoints(float mu1, float mu2, float separation,
                                             float out_L[5][3]);

/* Roche limit: minimum orbital distance before tidal disruption [m] */
float UpeOrbital_RocheLimit(float primary_radius, float primary_density,
                                        float secondary_density);

/* Tidal locking torque — drives body toward synchronous rotation */
float UpeOrbital_TidalLockTorque(float G, float M_primary, float R_body,
                                              float a_orbit, float I_body, float omega);

/* Atmospheric drag decay rate  da/dt  [m/s] */
float UpeOrbital_DecayRate(float a, float mu, float B_star, float rho_atm);

/* Apsidal precession rate due to J2 oblateness [rad/s] */
float UpeOrbital_ApsidalPrecession(float a, float e, float J2,
                                                float R_planet, float n);

/* Euler nutation/precession rates for a free rigid body */
void  UpeOrbital_EulerNutation(UpeVec3 omega, float Ix, float Iy, float Iz,
                                            float *psi_dot, float *theta_dot);

#ifdef __cplusplus
}
#endif
