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

#ifdef __cplusplus
}
#endif
