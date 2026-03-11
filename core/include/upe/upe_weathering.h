#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Surface Weathering & Aging (Exotic/Visual — Phase 9)

  Procedural surface degradation driven by environmental physics:
    - Oxidation rate (Arrhenius activation energy model)
    - Corrosion layer growth (parabolic rate law: x² = k_p * t)
    - UV photo-degradation (polymer chain scission rate)
    - Mechanical wear accumulation (Archard's wear law: V = k * F * s / H)
    - Thermal fatigue cracking (Manson-Coffin low-cycle fatigue)
    - Biological growth (logistic surface colonization by lichens etc.)
    - Surface roughness evolution (Ra increases with wear)
    - Rain pitting / abrasion from erosion module coupling
    - Visual damage level [0,1] from combined weathering state

  All per-surface, no allocations — callers maintain state structs.
*/

/* ---- Weathering state for a single surface patch ---- */
typedef struct UpeWeatherState {
    float oxidation_depth_m;    /* oxide layer thickness */
    float corrosion_depth_m;    /* corrosion/rust layer */
    float uv_damage;            /* [0,1] photo-degradation fraction */
    float wear_volume_m3;       /* removed material (Archard) */
    float thermal_fatigue;      /* Manson-Coffin cumulative fatigue fraction */
    float bio_coverage;         /* [0,1] biological colonization */
    float surface_roughness_Ra; /* avg roughness in meters */
    float age_s;                /* total simulation age in seconds */
} UpeWeatherState;

/* ---- Material weathering properties ---- */
typedef struct UpeWeatherMaterial {
    float activation_energy_J;  /* Arrhenius E_a for oxidation (J/mol) */
    float parabolic_rate_const; /* k_p  (m²/s) for oxide growth */
    float archard_k;            /* dimensionless wear coefficient */
    float hardness_Pa;          /* Vickers hardness in Pa (~3 GPa for steel) */
    float uv_degradation_rate;  /* fraction per J/m² UV dose */
    float bio_growth_rate;      /* logistic r (per second) */
    float bio_capacity;         /* K (max surface fraction) */
    float fatigue_strength_MPa; /* endurance limit */
    float fatigue_ductility;    /* Manson-Coffin exponent c */
} UpeWeatherMaterial;

/* --- Step a weathering state forward by dt seconds ---
   temperature_K: current surface temp.   uv_dose_J_m2: UV irradiance * dt.
   load_N: applied contact force.         sliding_dist_m: slip distance this step.
   thermal_strain_range: cyclic strain amplitude. */
void UpeWeather_Step(UpeWeatherState    *state,
                     const UpeWeatherMaterial *mat,
                     float temperature_K,
                     float uv_dose_J_m2,
                     float load_N,
                     float sliding_dist_m,
                     float thermal_strain_range,
                     float dt);

/* --- Oxidation rate (m/s): Arrhenius ---
   dx/dt = A * exp(-Ea / (R*T))   where A is empirical scaling. */
float UpeWeather_OxidationRate(const UpeWeatherMaterial *mat, float temperature_K);

/* --- Parabolic corrosion layer depth after time t (m) ---
   x = sqrt(k_p * t) */
float UpeWeather_CorrosionDepth(const UpeWeatherMaterial *mat, float time_s);

/* --- Archard wear volume (m³) ---
   V = k * F * s / H */
float UpeWeather_ArchardWear(const UpeWeatherMaterial *mat, float load_N, float sliding_dist_m);

/* --- Thermal fatigue cycles to failure (Manson-Coffin) ---
   Nf = (eps_f / delta_eps) ^ (1/c) */
float UpeWeather_FatigueCyclesToFailure(const UpeWeatherMaterial *mat,
                                         float strain_amplitude);

/* --- Logistic bio-growth ---
   dN/dt = r * N * (1 - N/K)  integrated exactly for dt. */
float UpeWeather_BioGrowth(float current_coverage, const UpeWeatherMaterial *mat, float dt);

/* --- Combined visual damage level [0,1] --- */
float UpeWeather_VisualDamage(const UpeWeatherState *state);

/* --- Surface friction coefficient after weathering (rougher = higher μ up to limit) --- */
float UpeWeather_WornFriction(float base_friction, const UpeWeatherState *state);

/* --- Reset state to pristine --- */
void UpeWeather_Reset(UpeWeatherState *state);

#ifdef __cplusplus
}
#endif
