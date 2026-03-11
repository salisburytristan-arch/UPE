#include "upe/upe_phase_change.h"
#include <math.h>
#include <string.h>

/* Universal gas constants */
#define UPE_R_GAS          8.314f     /* J/(mol·K) */
#define UPE_SQRT2PI        2.506628f

/* -----------------------------------------------------------------------
   Presets
   ----------------------------------------------------------------------- */

UpePhaseProfile UpePhase_ProfileWater(void) {
    UpePhaseProfile p;
    p.melt_point_K           = 273.15f;
    p.boil_point_K           = 373.15f;
    p.latent_fusion_J        = 334000.0f;
    p.latent_vaporization_J  = 2260000.0f;
    p.specific_heat_solid    = 2090.0f;
    p.specific_heat_liquid   = 4186.0f;
    p.specific_heat_gas      = 1996.0f;
    p.density_solid_kg_m3    = 917.0f;
    p.density_liquid_kg_m3   = 1000.0f;
    return p;
}

UpePhaseProfile UpePhase_ProfileIron(void) {
    UpePhaseProfile p;
    p.melt_point_K           = 1811.0f;
    p.boil_point_K           = 3134.0f;
    p.latent_fusion_J        = 247000.0f;
    p.latent_vaporization_J  = 6088000.0f;
    p.specific_heat_solid    = 450.0f;
    p.specific_heat_liquid   = 750.0f;
    p.specific_heat_gas      = 600.0f;
    p.density_solid_kg_m3    = 7870.0f;
    p.density_liquid_kg_m3   = 6980.0f;
    return p;
}

UpePhaseProfile UpePhase_ProfileWax(void) {
    UpePhaseProfile p;
    p.melt_point_K           = 330.0f;
    p.boil_point_K           = 620.0f;
    p.latent_fusion_J        = 175000.0f;
    p.latent_vaporization_J  = 390000.0f;
    p.specific_heat_solid    = 2500.0f;
    p.specific_heat_liquid   = 3000.0f;
    p.specific_heat_gas      = 1400.0f;
    p.density_solid_kg_m3    = 900.0f;
    p.density_liquid_kg_m3   = 760.0f;
    return p;
}

/* -----------------------------------------------------------------------
   Enthalpy boundaries for a material
   H < H_melt_start  => solid
   H_melt_start <= H <= H_melt_end  => mushy zone (transition)
   H_melt_end < H < H_boil_start  => liquid
   H_boil_start <= H <= H_boil_end  => boiling zone
   H > H_boil_end  => gas
   ----------------------------------------------------------------------- */

static void phase_enthalpy_bounds(const UpePhaseProfile *mat,
                                   float *Hms, float *Hme,
                                   float *Hbs, float *Hbe) {
    /* Enthalpy of solid at melt point (reference: solid at 0 K = 0) */
    *Hms = mat->specific_heat_solid  * mat->melt_point_K;
    *Hme = *Hms + mat->latent_fusion_J;
    /* Enthalpy of liquid at boil point */
    *Hbs = *Hme + mat->specific_heat_liquid * (mat->boil_point_K - mat->melt_point_K);
    *Hbe = *Hbs + mat->latent_vaporization_J;
}

UpePhaseState UpePhase_FromEnthalpy(const UpePhaseProfile *mat, float h) {
    if (!mat) return UPE_PHASE_SOLID;
    float Hms, Hme, Hbs, Hbe;
    phase_enthalpy_bounds(mat, &Hms, &Hme, &Hbs, &Hbe);
    if (h < Hms) return UPE_PHASE_SOLID;
    if (h < Hme) return UPE_PHASE_LIQUID; /* mid-fusion: treat as liquid */
    if (h < Hbs) return UPE_PHASE_LIQUID;
    if (h < Hbe) return UPE_PHASE_GAS;    /* boiling zone */
    return UPE_PHASE_GAS;
}

float UpePhase_TempFromEnthalpy(const UpePhaseProfile *mat, float h) {
    if (!mat) return 0.0f;
    float Hms, Hme, Hbs, Hbe;
    phase_enthalpy_bounds(mat, &Hms, &Hme, &Hbs, &Hbe);
    if (h < Hms) {
        return (mat->specific_heat_solid > 0.0f) ? h / mat->specific_heat_solid : 0.0f;
    }
    if (h < Hme) return mat->melt_point_K;   /* latent plateau: T constant */
    if (h < Hbs) {
        return mat->melt_point_K + (h - Hme) /
               (mat->specific_heat_liquid > 0.0f ? mat->specific_heat_liquid : 1.0f);
    }
    if (h < Hbe) return mat->boil_point_K;   /* boiling plateau */
    return mat->boil_point_K + (h - Hbe) /
           (mat->specific_heat_gas > 0.0f ? mat->specific_heat_gas : 1.0f);
}

UpePhaseState UpePhase_UpdateCell(UpePhaseCell *cell,
                                   const UpePhaseProfile *mat,
                                   float heat_added_J_kg) {
    if (!cell || !mat) return UPE_PHASE_SOLID;
    cell->enthalpy_J_kg += heat_added_J_kg;
    cell->temperature_K  = UpePhase_TempFromEnthalpy(mat, cell->enthalpy_J_kg);
    cell->phase          = UpePhase_FromEnthalpy(mat, cell->enthalpy_J_kg);
    return cell->phase;
}

/* -----------------------------------------------------------------------
   Clausius-Clapeyron
   ----------------------------------------------------------------------- */

float UpePhase_SaturationPressure(float P0_Pa, float T0_K, float T_K, float L_J_mol) {
    if (T0_K <= 0.0f || T_K <= 0.0f) return 0.0f;
    float exponent = (L_J_mol / UPE_R_GAS) * (1.0f/T0_K - 1.0f/T_K);
    /* Guard against extreme values */
    if (exponent < -80.0f) return P0_Pa * 1.8e-35f;
    if (exponent >  80.0f) return P0_Pa * 5.5e34f;
    return P0_Pa * expf(exponent);
}

float UpePhase_BoilingPointAtPressure(float P_Pa, float P0_Pa,
                                       float T0_K, float L_J_mol) {
    if (P_Pa <= 0.0f || P0_Pa <= 0.0f || T0_K <= 0.0f || L_J_mol <= 0.0f)
        return T0_K;
    /* 1/T = 1/T0 - (R/L)*ln(P/P0) */
    float ln_ratio = logf(P_Pa / P0_Pa);
    float inv_T = 1.0f/T0_K - (UPE_R_GAS / L_J_mol) * ln_ratio;
    if (inv_T <= 0.0f) return 1e6f;
    return 1.0f / inv_T;
}

/* -----------------------------------------------------------------------
   Stefan solidification front: x(t) = sqrt(2 * alpha * ΔT / L_star * t)
   where L_star = L / cp  (Stefan number denominator)
   ----------------------------------------------------------------------- */

float UpePhase_StefanFrontDepth(float thermal_diffusivity,
                                 float T_offset_K,
                                 float latent_heat_J_kg,
                                 float density_kg_m3,
                                 float time_s) {
    if (thermal_diffusivity <= 0.0f || latent_heat_J_kg <= 0.0f ||
        density_kg_m3 <= 0.0f || time_s < 0.0f || T_offset_K <= 0.0f)
        return 0.0f;
    /* Simplified: x = sqrt(2 * k_eff * T_offset / (rho * L) * t)
       where k_eff = alpha (thermal diffusivity already includes rho*cp) */
    float factor = 2.0f * thermal_diffusivity * T_offset_K / latent_heat_J_kg;
    if (factor < 0.0f) return 0.0f;
    return sqrtf(factor * time_s);
}

/* -----------------------------------------------------------------------
   Nucleation check (classical nucleation probability)
   ----------------------------------------------------------------------- */

int UpePhase_NucleationCheck(float undercooling_K, float nucleation_rate_per_K, uint32_t seed) {
    if (undercooling_K <= 0.0f) return 0;
    /* LCG to get a deterministic random [0,1] */
    uint32_t s = seed * 1664525u + 1013904223u;
    float rand01 = (float)(s >> 8) / (float)(1 << 24);
    float prob = 1.0f - expf(-nucleation_rate_per_K * undercooling_K);
    return (rand01 < prob) ? 1 : 0;
}

/* -----------------------------------------------------------------------
   Hertz-Knudsen evaporation mass flux
   J = alpha * (P_sat - P_amb) / sqrt(2 * pi * M * R * T)
   ----------------------------------------------------------------------- */

float UpePhase_EvapMassFlux(float sat_pressure_Pa, float ambient_pressure_Pa,
                             float molar_mass_kg_mol, float temperature_K) {
    if (temperature_K <= 0.0f || molar_mass_kg_mol <= 0.0f) return 0.0f;
    float dP = sat_pressure_Pa - ambient_pressure_Pa;
    float denom = sqrtf(2.0f * 3.14159265f * molar_mass_kg_mol * UPE_R_GAS * temperature_K);
    if (denom < 1e-30f) return 0.0f;
    return dP / denom;   /* kg/(m²·s) */
}

/* -----------------------------------------------------------------------
   Density change on transition
   ----------------------------------------------------------------------- */

float UpePhase_DensityAfterTransition(const UpePhaseProfile *mat,
                                       UpePhaseState from, UpePhaseState to) {
    if (!mat) return 1000.0f;
    (void)from;
    if (to == UPE_PHASE_SOLID)  return mat->density_solid_kg_m3;
    if (to == UPE_PHASE_LIQUID) return mat->density_liquid_kg_m3;
    /* Gas: approximate via ideal gas; caller must use P,T directly */
    return mat->density_liquid_kg_m3 * 0.001f;
}
