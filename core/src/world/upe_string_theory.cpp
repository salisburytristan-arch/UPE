#include "upe/upe_string_theory.h"

#include <math.h>

static float UpeString_SafeLightSpeed(float light_speed) {
    return (light_speed > 0.0f) ? light_speed : 299792458.0f;
}

float UpeString_ReggeMassSquared(uint32_t excitation_level, float alpha_prime) {
    if (alpha_prime <= 0.0f) {
        return 0.0f;
    }
    return (float)excitation_level / alpha_prime;
}

float UpeString_ReggeMass(uint32_t excitation_level, float alpha_prime) {
    float mass_squared = UpeString_ReggeMassSquared(excitation_level, alpha_prime);
    if (mass_squared <= 0.0f) {
        return 0.0f;
    }
    return sqrtf(mass_squared);
}

float UpeString_CompactificationKKModeMass(uint32_t mode_number,
                                           float reduced_planck_constant,
                                           float compact_radius,
                                           float light_speed) {
    if (compact_radius <= 0.0f || reduced_planck_constant <= 0.0f) {
        return 0.0f;
    }
    float c = UpeString_SafeLightSpeed(light_speed);
    float n = (float)mode_number;
    return (n * reduced_planck_constant) / (compact_radius * c);
}

float UpeString_DBraneTension(uint32_t p_dimension,
                              float string_coupling,
                              float string_length) {
    if (string_coupling <= 0.0f || string_length <= 0.0f) {
        return 0.0f;
    }

    const float two_pi = 6.28318530717958647692f;
    float p_term = powf(two_pi, (float)p_dimension);
    float l_term = powf(string_length, (float)p_dimension + 1.0f);
    if (p_term <= 0.0f || l_term <= 0.0f) {
        return 0.0f;
    }

    return 1.0f / (p_term * string_coupling * l_term);
}
