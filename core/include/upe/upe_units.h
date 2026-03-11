#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeUnits {
    float length_scale;
    float mass_scale;
    float time_scale;
} UpeUnits;

UpeUnits UpeUnits_SI();
UpeUnits UpeUnits_FromScales(float length_scale, float mass_scale, float time_scale);

#ifdef __cplusplus
}
#endif
