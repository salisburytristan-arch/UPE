#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

float UpeString_ReggeMassSquared(uint32_t excitation_level, float alpha_prime);
float UpeString_ReggeMass(uint32_t excitation_level, float alpha_prime);
float UpeString_CompactificationKKModeMass(uint32_t mode_number,
                                           float reduced_planck_constant,
                                           float compact_radius,
                                           float light_speed);
float UpeString_DBraneTension(uint32_t p_dimension,
                              float string_coupling,
                              float string_length);

#ifdef __cplusplus
}
#endif
