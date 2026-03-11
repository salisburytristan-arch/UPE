#include "upe/upe_weathering.h"
#include <math.h>
#include <string.h>

#define UPE_R_GAS  8.314f

float UpeWeather_OxidationRate(const UpeWeatherMaterial *mat, float temperature_K) {
    if (!mat || temperature_K <= 0.0f) return 0.0f;
    const float A = 1.0e-6f;  /* empirical pre-exponential factor (m/s) */
    float exponent = -mat->activation_energy_J / (UPE_R_GAS * temperature_K);
    if (exponent < -80.0f) return 0.0f;
    return A * expf(exponent);
}

float UpeWeather_CorrosionDepth(const UpeWeatherMaterial *mat, float time_s) {
    if (!mat || time_s <= 0.0f) return 0.0f;
    return sqrtf(mat->parabolic_rate_const * time_s);
}

float UpeWeather_ArchardWear(const UpeWeatherMaterial *mat,
                              float load_N, float sliding_dist_m) {
    if (!mat || mat->hardness_Pa <= 0.0f) return 0.0f;
    return mat->archard_k * load_N * sliding_dist_m / mat->hardness_Pa;
}

float UpeWeather_FatigueCyclesToFailure(const UpeWeatherMaterial *mat,
                                         float strain_amplitude) {
    if (!mat || strain_amplitude <= 0.0f || mat->fatigue_ductility >= 0.0f) {
        /* c is negative in Manson-Coffin; guard */
        return 1e10f;
    }
    /* eps_f ~ 0.35 for steel (ductility coefficient) */
    const float eps_f = 0.35f;
    float c = mat->fatigue_ductility;  /* should be negative, e.g. -0.5 */
    /* Nf = (eps_f / delta_eps)^(1/c) */
    float ratio = eps_f / strain_amplitude;
    if (ratio <= 0.0f) return 0.0f;
    return powf(ratio, 1.0f / c);
}

float UpeWeather_BioGrowth(float current_coverage, const UpeWeatherMaterial *mat, float dt) {
    if (!mat || mat->bio_capacity <= 0.0f) return current_coverage;
    float r = mat->bio_growth_rate;
    float K = mat->bio_capacity;
    float N = current_coverage;
    if (N <= 0.0f) N = 1e-4f;  /* seed nucleus */
    /* Exact logistic solution: N(t+dt) = K / (1 + ((K/N) - 1)*exp(-r*dt)) */
    float exp_term = expf(-r * dt);
    float new_N = K / (1.0f + (K/N - 1.0f) * exp_term);
    if (new_N > K) new_N = K;
    return new_N;
}

void UpeWeather_Step(UpeWeatherState    *state,
                     const UpeWeatherMaterial *mat,
                     float temperature_K,
                     float uv_dose_J_m2,
                     float load_N,
                     float sliding_dist_m,
                     float thermal_strain_range,
                     float dt) {
    if (!state || !mat || dt <= 0.0f) return;

    state->age_s += dt;

    /* Oxidation */
    state->oxidation_depth_m += UpeWeather_OxidationRate(mat, temperature_K) * dt;

    /* Corrosion (parabolic total — recompute from total age) */
    state->corrosion_depth_m = UpeWeather_CorrosionDepth(mat, state->age_s);

    /* UV degradation */
    state->uv_damage += mat->uv_degradation_rate * uv_dose_J_m2;
    if (state->uv_damage > 1.0f) state->uv_damage = 1.0f;

    /* Archard wear */
    state->wear_volume_m3 += UpeWeather_ArchardWear(mat, load_N, sliding_dist_m);

    /* Thermal fatigue: accumulate 1/Nf per cycle */
    if (thermal_strain_range > 0.0f) {
        float Nf = UpeWeather_FatigueCyclesToFailure(mat, thermal_strain_range * 0.5f);
        if (Nf > 0.0f) state->thermal_fatigue += 1.0f / Nf * dt;
        if (state->thermal_fatigue > 1.0f) state->thermal_fatigue = 1.0f;
    }

    /* Bio growth */
    state->bio_coverage = UpeWeather_BioGrowth(state->bio_coverage, mat, dt);

    /* Surface roughness increases with wear cube root */
    float base_Ra = 1e-6f;  /* 1 μm initial Ra */
    state->surface_roughness_Ra = base_Ra * (1.0f + 1000.0f * cbrtf(state->wear_volume_m3));
}

float UpeWeather_VisualDamage(const UpeWeatherState *state) {
    if (!state) return 0.0f;
    float d = 0.0f;
    d += fminf(state->oxidation_depth_m / 1e-3f, 1.0f) * 0.2f;   /* 1mm = full */
    d += state->uv_damage * 0.2f;
    d += fminf(state->wear_volume_m3 / 1e-6f, 1.0f) * 0.2f;      /* 1cm³ = full */
    d += state->thermal_fatigue * 0.2f;
    d += state->bio_coverage * 0.2f;
    return fminf(d, 1.0f);
}

float UpeWeather_WornFriction(float base_friction, const UpeWeatherState *state) {
    if (!state) return base_friction;
    /* Rougher surface → slightly higher friction, but rust/bio can be slippery */
    float wear_factor  = 1.0f + 0.3f * fminf(state->surface_roughness_Ra / 1e-4f, 1.0f);
    float slip_factor  = 1.0f - 0.2f * state->bio_coverage;
    return base_friction * wear_factor * slip_factor;
}

void UpeWeather_Reset(UpeWeatherState *state) {
    if (!state) return;
    memset(state, 0, sizeof(*state));
}
