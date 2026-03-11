#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_body.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeAerodynamics {
    float drag_coefficient;
    float lift_coefficient;
    float reference_area;
    float air_density;
} UpeAerodynamics;

void UpeAerodynamics_Init(UpeAerodynamics *aero);
UpeVec3 UpeAerodynamics_ComputeForce(const UpeAerodynamics *aero, UpeVec3 velocity, UpeVec3 wind);

/* Bernoulli — result is pressure at station 2 given station 1 and velocity */
float UpeAero_BernoulliPressure(float rho, float v1, float P1, float v2);

/* ISA barometric formula  P = P0 * exp(-g*M*h / (R*T0))  */
float UpeAero_BarometricPressure(float P0, float altitude_m, float T0_K);

/* Isentropic relation at Mach: P_static = P_total / (1 + 0.2*M^2)^3.5  */
float UpeAero_StaticPressureAtMach(float P_total, float mach);

#ifdef __cplusplus
}
#endif
