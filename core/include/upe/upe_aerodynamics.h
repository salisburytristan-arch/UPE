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

#ifdef __cplusplus
}
#endif
