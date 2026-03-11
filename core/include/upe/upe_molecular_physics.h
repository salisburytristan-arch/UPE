#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Microphysics / Molecular Physics
  Scientific-approximation tier.

  Covers:
  - Brownian motion (seeded stochastic diffusion forces)
  - Van der Waals forces (Lennard-Jones 6-12 potential)
  - Electron cloud probability density
  - Lattice vibrations (phonon dispersion)
  - Micro-fluidics and capillary action (Young-Laplace)
  - Ion channel gating (Hodgkin-Huxley-inspired stub)
  - Nuclear fission/fusion (Q-value energy release)
  - Quantum tunneling (WKB barrier penetration)
  - Pauli exclusion (short-range hard repulsion)
  - Covalent bond tension (Morse potential)

  All stochastic features are deterministic per seed via upe_rng.
  Cross-platform bit-exact not claimed for stochastic paths.
*/

/* -----------------------------------------------------------------------
   Brownian Motion
   Stochastic diffusion force on a particle.
   Force ~ sqrt(2 * k_B * T * gamma / dt) * N(0,1) per axis.
   ----------------------------------------------------------------------- */

typedef struct UpeBrownianParams {
    float temperature;          /* Kelvin */
    float viscosity;            /* Pa*s (dynamic viscosity of medium) */
    float particle_radius;      /* metres */
    uint32_t rng_seed;          /* Set once; advances deterministically */
} UpeBrownianParams;

/* Compute Stokes drag coefficient: gamma = 6 * pi * eta * r */
float UpeBrownian_DragCoefficient(float viscosity, float particle_radius);

/* Generate Brownian random force for one timestep. Returns force vector in Newtons. */
UpeVec3 UpeBrownian_RandomForce(UpeBrownianParams *params, float dt, uint32_t *rng_state);

/* Apply Brownian impulse in-place to velocity array. n_particles: number of particles. */
void UpeBrownian_ApplyToParticles(UpeVec3 *velocities, const float *inv_masses,
                                   uint32_t n_particles, UpeBrownianParams *params,
                                   float dt, uint32_t *rng_state);


/* -----------------------------------------------------------------------
   Van der Waals Forces — Lennard-Jones 6-12 Potential
   V(r) = 4*epsilon * [(sigma/r)^12 - (sigma/r)^6]
   F(r) = -dV/dr
   ----------------------------------------------------------------------- */

typedef struct UpeLJParams {
    float epsilon;              /* Well depth (Joules) */
    float sigma;                /* Distance at V=0 (metres; ~collision diameter) */
    float cutoff_distance;      /* Truncate interaction beyond this (metres) */
} UpeLJParams;

/* Potential energy at separation r */
float UpeLJ_Potential(const UpeLJParams *p, float r);

/* Scalar force magnitude (positive = repulsive, negative = attractive) */
float UpeLJ_ForceMagnitude(const UpeLJParams *p, float r);

/* Force vector on particle A from particle B */
UpeVec3 UpeLJ_ForceVector(const UpeLJParams *p, UpeVec3 pos_a, UpeVec3 pos_b);

/* Presets: noble gases at standard conditions */
void UpeLJ_PresetArgon(UpeLJParams *p);
void UpeLJ_PresetHelium(UpeLJParams *p);
void UpeLJ_PresetWater(UpeLJParams *p);


/* -----------------------------------------------------------------------
   Electron Cloud Probability Density
   Simplified hydrogen-like radial distribution:
   P(r) ~ r^2 * exp(-2r/a0)  (1s orbital)
   ----------------------------------------------------------------------- */

/* Returns unnormalized radial probability density at radius r (metres).
   a0: Bohr radius or effective orbital radius scale. */
float UpeElectron_RadialProbability(float r, float bohr_radius);

/* Sample probability density on a grid cell for collision weighting */
float UpeElectron_GridDensity(UpeVec3 sample_pos, UpeVec3 nucleus_pos,
                               float bohr_radius);


/* -----------------------------------------------------------------------
   Lattice Vibrations (Phonons)
   1D diatomic chain dispersion: omega(k) = 2*sqrt(K/m) * |sin(k*a/2)|
   ----------------------------------------------------------------------- */

typedef struct UpeLatticeParams {
    float spring_constant;      /* N/m interatomic spring */
    float atom_mass;            /* kg */
    float lattice_spacing;      /* metres */
} UpeLatticeParams;

/* Angular frequency for wavevector k (rad/m) */
float UpeLattice_PhononFrequency(const UpeLatticeParams *p, float k);

/* Phase velocity at wavevector k */
float UpeLattice_PhaseVelocity(const UpeLatticeParams *p, float k);

/* Group velocity (dω/dk) at wavevector k */
float UpeLattice_GroupVelocity(const UpeLatticeParams *p, float k);


/* -----------------------------------------------------------------------
   Micro-fluidics / Capillary Action
   Young-Laplace pressure: dP = 2 * gamma * cos(theta) / r
   ----------------------------------------------------------------------- */

/* Capillary pressure rise in a tube of radius r,
   contact_angle in radians, surface_tension in N/m */
float UpeCapillary_PressureDifference(float surface_tension,
                                       float contact_angle,
                                       float tube_radius);

/* Height of capillary rise: h = 2*gamma*cos(theta) / (rho * g * r) */
float UpeCapillary_RiseHeight(float surface_tension, float contact_angle,
                               float tube_radius, float fluid_density, float gravity);

/* Flow velocity in a capillary channel (Poiseuille): v = r^2 * dP / (8 * eta * L) */
float UpeCapillary_PoiseuillveVelocity(float tube_radius, float pressure_diff,
                                        float viscosity, float channel_length);


/* -----------------------------------------------------------------------
   Ion Channel Gating (Hodgkin-Huxley stub)
   Gate state probability as a function of membrane voltage.
   ----------------------------------------------------------------------- */

typedef struct UpeIonChannel {
    float open_probability;     /* 0=closed, 1=open */
    float channel_conductance;  /* Siemens */
    float reversal_potential;   /* Nernst potential, Volts */
    float time_constant;        /* ms — gating kinetics speed */
    float state_n;              /* HH gating variable */
} UpeIonChannel;

/* Step gating variable toward steady-state for a given membrane voltage (Volts), dt in ms */
void UpeIonChannel_Step(UpeIonChannel *channel, float membrane_voltage_mv, float dt_ms);

/* Ionic current through channel: I = g * (V - E_rev) */
float UpeIonChannel_Current(const UpeIonChannel *channel, float membrane_voltage_mv);


/* -----------------------------------------------------------------------
   Nuclear Fission / Fusion — Q-value energy release stub
   E = (m_reactants - m_products) * c^2
   ----------------------------------------------------------------------- */

/* Q-value in Joules given mass deficit in kg */
float UpeNuclear_QValue(float mass_deficit_kg);

/* Fission: typical Q ~200 MeV per U-235 split.
   Returns energy in Joules. mass_number 235 = typical uranium. */
float UpeNuclear_FissionEnergy(float mass_number);

/* Fusion: D-T reaction releases ~17.6 MeV.
   Returns energy in Joules for a single D-T event. */
float UpeNuclear_FusionDTEnergy(void);

/* Chain reaction: n_events fissions, branching_factor ~2 per generation.
   Returns total energy and final generation count. */
float UpeNuclear_ChainReactionEnergy(float single_fission_joules,
                                      float branching_factor,
                                      uint32_t generations,
                                      uint32_t *out_event_count);


/* -----------------------------------------------------------------------
   Quantum Tunneling — WKB Approximation
   Transmission probability: T = exp(-2 * kappa * d)
   kappa = sqrt(2m(V0-E)) / hbar
   ----------------------------------------------------------------------- */

/* Transmission probability for a particle of mass_kg and kinetic energy_joules
   encountering a rectangular barrier of height barrier_joules and width width_m.
   Returns value in [0, 1]. */
float UpeQM_TunnelingProbability(float mass_kg, float energy_joules,
                                  float barrier_height_joules, float barrier_width_m);

/* Roll a tunneling event using rng_state. Returns 1 (tunnel) or 0 (reflect). */
int UpeQM_TunnelingEvent(float transmission_probability, uint32_t *rng_state);


/* -----------------------------------------------------------------------
   Pauli Exclusion — Short-range hard repulsion approximation
   F(r) = A * exp(-r / rho)
   Prevents overlap below atomic-scale separations.
   ----------------------------------------------------------------------- */

/* Scalar Born repulsion force magnitude at distance r */
float UpePauli_BornRepulsionForce(float amplitude, float repulsion_length, float r);

/* Force vector pushing particle B away from particle A */
UpeVec3 UpePauli_ForceVector(float amplitude, float repulsion_length,
                               UpeVec3 pos_a, UpeVec3 pos_b);


/* -----------------------------------------------------------------------
   Covalent Bond Tension — Morse Potential
   V(r) = D_e * (1 - exp(-a*(r-r_e)))^2
   F(r) = -dV/dr = 2*a*D_e * (1-exp(-a*(r-r_e))) * exp(-a*(r-r_e))
   ----------------------------------------------------------------------- */

typedef struct UpeMorseBond {
    float dissociation_energy;  /* D_e in Joules */
    float equilibrium_distance; /* r_e in metres */
    float width_parameter;      /* a in 1/metres (steepness) */
    int broken;                 /* Set to 1 when bond has dissociated */
} UpeMorseBond;

/* Morse potential energy at distance r */
float UpeMorse_Potential(const UpeMorseBond *bond, float r);

/* Bond force as a scalar (positive = repulsion, negative = attraction) */
float UpeMorse_ForceMagnitude(const UpeMorseBond *bond, float r);

/* Full force vector on particle A from particle B through this bond.
   Sets bond->broken if r is large enough that force has passed the inflection. */
UpeVec3 UpeMorse_ForceVector(UpeMorseBond *bond, UpeVec3 pos_a, UpeVec3 pos_b);

/* Common bond presets (approximate values) */
void UpeMorse_PresetHydrogenH2(UpeMorseBond *bond);
void UpeMorse_PresetCarbonCarbonSingle(UpeMorseBond *bond);
void UpeMorse_PresetOxygenHydrogen(UpeMorseBond *bond);

#ifdef __cplusplus
}
#endif
