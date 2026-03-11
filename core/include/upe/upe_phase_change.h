#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Phase Change Physics (Environmental — Phase 4)

  Models thermodynamic phase transitions:
    - Melting / Freezing  (solid ↔ liquid)  via latent heat of fusion
    - Evaporation / Condensation  (liquid ↔ gas)  via Clausius-Clapeyron
    - Sublimation  (solid ↔ gas)
    - Boiling point elevation / depression (colligative)
    - Supercooling / Superheating nucleation delay
    - Stefan's solidification front (1-D thickness vs. time)

  All functions are pure math utilities operating on per-cell or per-particle
  temperature and enthalpy state.  No allocations; callers own storage.
*/

/* ---- Phase state ---- */
typedef enum UpePhaseState {
    UPE_PHASE_SOLID  = 0,
    UPE_PHASE_LIQUID = 1,
    UPE_PHASE_GAS    = 2,
    UPE_PHASE_PLASMA = 3
} UpePhaseState;

/* ---- Per-material thermal profile ---- */
typedef struct UpePhaseProfile {
    float melt_point_K;          /* normal melting/freezing point */
    float boil_point_K;          /* normal boiling point (1 atm) */
    float latent_fusion_J;       /* J per kg */
    float latent_vaporization_J; /* J per kg */
    float specific_heat_solid;   /* J / (kg·K) */
    float specific_heat_liquid;  /* J / (kg·K) */
    float specific_heat_gas;     /* J / (kg·K) */
    float density_solid_kg_m3;
    float density_liquid_kg_m3;
} UpePhaseProfile;

/* ---- Single-cell enthalpy state ---- */
typedef struct UpePhaseCell {
    float enthalpy_J_kg;         /* total specific enthalpy */
    float temperature_K;         /* derived from enthalpy each step */
    UpePhaseState phase;
    float supercool_offset_K;    /* undercooling delay before nucleation */
    float superheat_offset_K;    /* overheating delay before nucleation */
} UpePhaseCell;

/* --- Phase profile presets --- */
UpePhaseProfile UpePhase_ProfileWater(void);
UpePhaseProfile UpePhase_ProfileIron(void);
UpePhaseProfile UpePhase_ProfileWax(void);

/* --- Core state update ---
   Given absorbed heat_added_J_kg this timestep, update enthalpy and phase.
   Returns new phase (may differ from old if transition occurred). */
UpePhaseState UpePhase_UpdateCell(UpePhaseCell *cell,
                                   const UpePhaseProfile *mat,
                                   float heat_added_J_kg);

/* --- Phase determination from enthalpy ---
   Returns the equilibrium phase for a given specific enthalpy. */
UpePhaseState UpePhase_FromEnthalpy(const UpePhaseProfile *mat, float enthalpy_J_kg);

/* --- Temperature from enthalpy (given known phase) --- */
float UpePhase_TempFromEnthalpy(const UpePhaseProfile *mat, float enthalpy_J_kg);

/* --- Clausius-Clapeyron: saturation pressure at temperature T (Pa) ---
   ln(P/P0) = (L/R)(1/T0 - 1/T),  L = molar latent heat, R = gas constant
   Provide P0 = reference pressure, T0 = reference temperature. */
float UpePhase_SaturationPressure(float P0_Pa, float T0_K, float T_K, float L_J_mol);

/* --- Boiling point at arbitrary pressure (K) --- */
float UpePhase_BoilingPointAtPressure(float P_Pa, float P0_Pa, float T0_K, float L_J_mol);

/* --- Stefan solidification: thickness of ice/crust front over time ---
   x(t) = C * sqrt(t),  C = sqrt(2*k*T_offset/(rho*L))
   Returns solidification front depth in meters. */
float UpePhase_StefanFrontDepth(float thermal_diffusivity,  /* m²/s */
                                 float T_offset_K,            /* Tmelt - Tsurface */
                                 float latent_heat_J_kg,
                                 float density_kg_m3,
                                 float time_s);

/* --- Supercooling nucleation check ---
   Returns 1 if nucleation should occur given undercooling depth and random seed. */
int UpePhase_NucleationCheck(float undercooling_K, float nucleation_rate_per_K, uint32_t seed);

/* --- Evaporation mass flux (kg / m² / s) via Hertz-Knudsen equation --- */
float UpePhase_EvapMassFlux(float sat_pressure_Pa, float ambient_pressure_Pa,
                             float molar_mass_kg_mol, float temperature_K);

/* --- Density change on phase transition ---
   Returns new density given old density and phase transition direction. */
float UpePhase_DensityAfterTransition(const UpePhaseProfile *mat,
                                       UpePhaseState from, UpePhaseState to);

#ifdef __cplusplus
}
#endif
