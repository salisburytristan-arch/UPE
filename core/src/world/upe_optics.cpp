#include "upe/upe_optics.h"

float UpeOptics_PhotoelectricEnergy(float frequency_Hz, float work_function_J) {
    const float H = 6.62607015e-34f;
    if (frequency_Hz <= 0.0f) return 0.0f;
    float E = H * frequency_Hz - work_function_J;
    return (E > 0.0f) ? E : 0.0f;
}

float UpeOptics_Albedo(float reflected_power, float incident_power) {
    if (incident_power <= 0.0f) return 0.0f;
    float a = reflected_power / incident_power;
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    return a;
}
