#include "upe/upe_molecular_physics.h"
#include <math.h>
#include <stdint.h>

/* Physical constants */
#define UPE_HBAR         1.054571817e-34f  /* J*s */
#define UPE_KB           1.380649e-23f     /* J/K */
#define UPE_C_LIGHT      299792458.0f      /* m/s */
#define UPE_BOHR_RADIUS  5.29177210903e-11f/* m */
#define UPE_AMU          1.66053906660e-27f/* kg per atomic mass unit */
#define UPE_EV_TO_J      1.602176634e-19f  /* Joules per eV */
#define UPE_MEV_TO_J     (1.602176634e-13f)/* Joules per MeV */
#define UPE_PI           3.14159265358979323846f

/* Minimal LCG RNG — deterministic per seed, not cryptographic */
static uint32_t upe_lcg_step(uint32_t *state) {
    *state = (*state) * 1664525u + 1013904223u;
    return *state;
}

/* Box-Muller: returns one N(0,1) sample, advances state by 2 steps */
static float upe_normal_sample(uint32_t *rng_state) {
    uint32_t u1 = upe_lcg_step(rng_state);
    uint32_t u2 = upe_lcg_step(rng_state);
    float fu1 = (float)(u1 >> 1) / 2147483648.0f + 1e-7f; /* (0,1] */
    float fu2 = (float)(u2 >> 1) / 2147483648.0f;
    float r   = sqrtf(-2.0f * logf(fu1));
    float phi = 2.0f * UPE_PI * fu2;
    return r * cosf(phi);
}

/* -----------------------------------------------------------------------
   Brownian Motion
   ----------------------------------------------------------------------- */

float UpeBrownian_DragCoefficient(float viscosity, float particle_radius) {
    if (viscosity <= 0.0f || particle_radius <= 0.0f) return 0.0f;
    return 6.0f * UPE_PI * viscosity * particle_radius;
}

UpeVec3 UpeBrownian_RandomForce(UpeBrownianParams *params, float dt,
                                 uint32_t *rng_state) {
    UpeVec3 zero = {0.0f, 0.0f, 0.0f};
    if (!params || !rng_state || dt <= 0.0f) return zero;

    float gamma = UpeBrownian_DragCoefficient(params->viscosity, params->particle_radius);
    if (gamma <= 0.0f) return zero;

    /* sigma_F = sqrt(2 * k_B * T * gamma / dt) */
    float sigma = sqrtf(2.0f * UPE_KB * params->temperature * gamma / dt);

    UpeVec3 f;
    f.x = sigma * upe_normal_sample(rng_state);
    f.y = sigma * upe_normal_sample(rng_state);
    f.z = sigma * upe_normal_sample(rng_state);
    return f;
}

void UpeBrownian_ApplyToParticles(UpeVec3 *velocities, const float *inv_masses,
                                   uint32_t n_particles, UpeBrownianParams *params,
                                   float dt, uint32_t *rng_state) {
    if (!velocities || !inv_masses || !params || !rng_state || n_particles == 0) return;

    for (uint32_t i = 0; i < n_particles; ++i) {
        if (inv_masses[i] <= 0.0f) continue;
        UpeVec3 f = UpeBrownian_RandomForce(params, dt, rng_state);
        /* dv = (F / m) * dt = F * inv_mass * dt */
        float scale = inv_masses[i] * dt;
        velocities[i].x += f.x * scale;
        velocities[i].y += f.y * scale;
        velocities[i].z += f.z * scale;
    }
}

/* -----------------------------------------------------------------------
   Lennard-Jones Potential
   ----------------------------------------------------------------------- */

float UpeLJ_Potential(const UpeLJParams *p, float r) {
    if (!p || r <= 0.0f) return 0.0f;
    float sr  = p->sigma / r;
    float sr6 = sr * sr * sr * sr * sr * sr;
    float sr12 = sr6 * sr6;
    return 4.0f * p->epsilon * (sr12 - sr6);
}

float UpeLJ_ForceMagnitude(const UpeLJParams *p, float r) {
    if (!p || r <= 0.0f) return 0.0f;
    if (r > p->cutoff_distance && p->cutoff_distance > 0.0f) return 0.0f;
    float sr  = p->sigma / r;
    float sr6 = sr * sr * sr * sr * sr * sr;
    float sr12 = sr6 * sr6;
    /* F = -dV/dr = (24*eps/r)*(2*sr12 - sr6) */
    return (24.0f * p->epsilon / r) * (2.0f * sr12 - sr6);
}

UpeVec3 UpeLJ_ForceVector(const UpeLJParams *p, UpeVec3 pos_a, UpeVec3 pos_b) {
    UpeVec3 zero = {0.0f, 0.0f, 0.0f};
    if (!p) return zero;
    UpeVec3 d = {pos_b.x - pos_a.x, pos_b.y - pos_a.y, pos_b.z - pos_a.z};
    float r2  = d.x*d.x + d.y*d.y + d.z*d.z;
    if (r2 < 1e-30f) return zero;
    float r   = sqrtf(r2);
    float fmag = UpeLJ_ForceMagnitude(p, r);
    /* Force on A is in direction of B (positive fmag = repulsion -> points away) */
    float inv_r = 1.0f / r;
    UpeVec3 result = {-fmag * d.x * inv_r, -fmag * d.y * inv_r, -fmag * d.z * inv_r};
    return result;
}

void UpeLJ_PresetArgon(UpeLJParams *p) {
    if (!p) return;
    p->epsilon        = 1.654e-21f;  /* 120 * k_B */
    p->sigma          = 3.4e-10f;    /* 3.4 Angstrom */
    p->cutoff_distance = 10.2e-10f;  /* 3 * sigma */
}

void UpeLJ_PresetHelium(UpeLJParams *p) {
    if (!p) return;
    p->epsilon        = 1.38e-23f;   /* ~1 * k_B (very weakly interacting) */
    p->sigma          = 2.56e-10f;
    p->cutoff_distance = 7.68e-10f;
}

void UpeLJ_PresetWater(UpeLJParams *p) {
    if (!p) return;
    p->epsilon        = 6.74e-22f;   /* SPC/E oxygen-oxygen */
    p->sigma          = 3.166e-10f;
    p->cutoff_distance = 9.5e-10f;
}

/* -----------------------------------------------------------------------
   Electron Cloud Probability Density
   ----------------------------------------------------------------------- */

float UpeElectron_RadialProbability(float r, float bohr_radius) {
    if (r < 0.0f || bohr_radius <= 0.0f) return 0.0f;
    float r_norm = r / bohr_radius;
    return r_norm * r_norm * expf(-2.0f * r_norm);
}

float UpeElectron_GridDensity(UpeVec3 sample_pos, UpeVec3 nucleus_pos,
                               float bohr_radius) {
    float dx = sample_pos.x - nucleus_pos.x;
    float dy = sample_pos.y - nucleus_pos.y;
    float dz = sample_pos.z - nucleus_pos.z;
    float r  = sqrtf(dx*dx + dy*dy + dz*dz);
    return UpeElectron_RadialProbability(r, bohr_radius);
}

/* -----------------------------------------------------------------------
   Lattice Vibrations (Phonons)
   ----------------------------------------------------------------------- */

float UpeLattice_PhononFrequency(const UpeLatticeParams *p, float k) {
    if (!p || p->atom_mass <= 0.0f || p->spring_constant <= 0.0f) return 0.0f;
    float omega_max = 2.0f * sqrtf(p->spring_constant / p->atom_mass);
    float arg = fabsf(sinf(k * p->lattice_spacing * 0.5f));
    return omega_max * arg;
}

float UpeLattice_PhaseVelocity(const UpeLatticeParams *p, float k) {
    if (!p || fabsf(k) < 1e-30f) return 0.0f;
    return UpeLattice_PhononFrequency(p, k) / fabsf(k);
}

float UpeLattice_GroupVelocity(const UpeLatticeParams *p, float k) {
    if (!p || p->atom_mass <= 0.0f || p->spring_constant <= 0.0f) return 0.0f;
    /* d(omega)/dk = sqrt(K/m) * a * |cos(k*a/2)| */
    float v_sound = sqrtf(p->spring_constant / p->atom_mass) * p->lattice_spacing;
    return v_sound * fabsf(cosf(k * p->lattice_spacing * 0.5f));
}

/* -----------------------------------------------------------------------
   Capillary Action
   ----------------------------------------------------------------------- */

float UpeCapillary_PressureDifference(float surface_tension, float contact_angle,
                                       float tube_radius) {
    if (tube_radius <= 0.0f) return 0.0f;
    return (2.0f * surface_tension * cosf(contact_angle)) / tube_radius;
}

float UpeCapillary_RiseHeight(float surface_tension, float contact_angle,
                               float tube_radius, float fluid_density, float gravity) {
    if (tube_radius <= 0.0f || fluid_density <= 0.0f || gravity <= 0.0f) return 0.0f;
    return (2.0f * surface_tension * cosf(contact_angle)) /
           (fluid_density * gravity * tube_radius);
}

float UpeCapillary_PoiseuillveVelocity(float tube_radius, float pressure_diff,
                                        float viscosity, float channel_length) {
    if (viscosity <= 0.0f || channel_length <= 0.0f) return 0.0f;
    return (tube_radius * tube_radius * pressure_diff) / (8.0f * viscosity * channel_length);
}

/* -----------------------------------------------------------------------
   Ion Channel Gating
   ----------------------------------------------------------------------- */

/* HH alpha_n rate function for K+ channel gating variable n */
static float hh_alpha_n(float v_mv) {
    float dv = v_mv + 55.0f;
    if (fabsf(dv) < 0.01f) return 0.1f; /* L'Hopital limit */
    return 0.01f * dv / (1.0f - expf(-dv / 10.0f));
}

static float hh_beta_n(float v_mv) {
    return 0.125f * expf(-(v_mv + 65.0f) / 80.0f);
}

void UpeIonChannel_Step(UpeIonChannel *channel, float membrane_voltage_mv, float dt_ms) {
    if (!channel) return;
    float alpha = hh_alpha_n(membrane_voltage_mv);
    float beta  = hh_beta_n(membrane_voltage_mv);
    float n_inf = alpha / (alpha + beta + 1e-12f);
    float tau_n = 1.0f / (alpha + beta + 1e-12f);
    /* Euler integration for gating variable */
    channel->state_n += (dt_ms / tau_n) * (n_inf - channel->state_n);
    channel->state_n = (channel->state_n < 0.0f) ? 0.0f :
                       (channel->state_n > 1.0f) ? 1.0f : channel->state_n;
    /* Open probability is n^4 for K+ channel */
    float n4 = channel->state_n * channel->state_n *
               channel->state_n * channel->state_n;
    channel->open_probability = n4;
}

float UpeIonChannel_Current(const UpeIonChannel *channel, float membrane_voltage_mv) {
    if (!channel) return 0.0f;
    float g_eff = channel->channel_conductance * channel->open_probability;
    return g_eff * (membrane_voltage_mv - channel->reversal_potential);
}

/* -----------------------------------------------------------------------
   Nuclear Fission / Fusion
   ----------------------------------------------------------------------- */

float UpeNuclear_QValue(float mass_deficit_kg) {
    return mass_deficit_kg * UPE_C_LIGHT * UPE_C_LIGHT;
}

float UpeNuclear_FissionEnergy(float mass_number) {
    /* Approximate: ~0.84 MeV per nucleon for U-235 fission */
    (void)mass_number;
    return 200.0f * UPE_MEV_TO_J; /* 200 MeV per fission event */
}

float UpeNuclear_FusionDTEnergy(void) {
    return 17.6f * UPE_MEV_TO_J;
}

float UpeNuclear_ChainReactionEnergy(float single_fission_joules,
                                      float branching_factor,
                                      uint32_t generations,
                                      uint32_t *out_event_count) {
    if (branching_factor <= 0.0f || generations == 0) {
        if (out_event_count) *out_event_count = 0;
        return 0.0f;
    }
    /* Geometric series: sum = (b^n - 1) / (b - 1) for b != 1 */
    float total_events = 0.0f;
    float events_this_gen = 1.0f;
    for (uint32_t g = 0; g < generations; ++g) {
        total_events  += events_this_gen;
        events_this_gen *= branching_factor;
    }
    if (out_event_count) *out_event_count = (uint32_t)total_events;
    return total_events * single_fission_joules;
}

/* -----------------------------------------------------------------------
   Quantum Tunneling (WKB)
   ----------------------------------------------------------------------- */

float UpeQM_TunnelingProbability(float mass_kg, float energy_joules,
                                  float barrier_height_joules, float barrier_width_m) {
    if (mass_kg <= 0.0f || barrier_width_m <= 0.0f) return 0.0f;
    float dE = barrier_height_joules - energy_joules;
    if (dE <= 0.0f) return 1.0f; /* Above barrier — classical pass */
    /* kappa = sqrt(2m * dE) / hbar */
    float kappa = sqrtf(2.0f * mass_kg * dE) / UPE_HBAR;
    float exponent = -2.0f * kappa * barrier_width_m;
    if (exponent < -87.0f) return 0.0f; /* Underflow guard */
    return expf(exponent);
}

int UpeQM_TunnelingEvent(float transmission_probability, uint32_t *rng_state) {
    if (!rng_state) return 0;
    uint32_t rv = upe_lcg_step(rng_state);
    float fv = (float)(rv >> 1) / 2147483648.0f;
    return (fv < transmission_probability) ? 1 : 0;
}

/* -----------------------------------------------------------------------
   Pauli Exclusion (Born repulsion)
   ----------------------------------------------------------------------- */

float UpePauli_BornRepulsionForce(float amplitude, float repulsion_length, float r) {
    if (r <= 0.0f || repulsion_length <= 0.0f) return 0.0f;
    return amplitude * expf(-r / repulsion_length);
}

UpeVec3 UpePauli_ForceVector(float amplitude, float repulsion_length,
                               UpeVec3 pos_a, UpeVec3 pos_b) {
    UpeVec3 zero = {0.0f, 0.0f, 0.0f};
    float dx = pos_b.x - pos_a.x;
    float dy = pos_b.y - pos_a.y;
    float dz = pos_b.z - pos_a.z;
    float r2 = dx*dx + dy*dy + dz*dz;
    if (r2 < 1e-40f) return zero;
    float r    = sqrtf(r2);
    float fmag = UpePauli_BornRepulsionForce(amplitude, repulsion_length, r);
    float inv_r = 1.0f / r;
    /* Repulsion: force on A points away from B */
    UpeVec3 result = {-fmag * dx * inv_r, -fmag * dy * inv_r, -fmag * dz * inv_r};
    return result;
}

/* -----------------------------------------------------------------------
   Morse Potential (Covalent Bond)
   ----------------------------------------------------------------------- */

float UpeMorse_Potential(const UpeMorseBond *bond, float r) {
    if (!bond) return 0.0f;
    float e = expf(-bond->width_parameter * (r - bond->equilibrium_distance));
    float v = bond->dissociation_energy * (1.0f - e) * (1.0f - e);
    return v;
}

float UpeMorse_ForceMagnitude(const UpeMorseBond *bond, float r) {
    if (!bond) return 0.0f;
    float a  = bond->width_parameter;
    float De = bond->dissociation_energy;
    float re = bond->equilibrium_distance;
    float e  = expf(-a * (r - re));
    /* F = -dV/dr = 2*a*De*(1-e)*e  (positive → repulsion at r < re) */
    return 2.0f * a * De * (1.0f - e) * e;
}

UpeVec3 UpeMorse_ForceVector(UpeMorseBond *bond, UpeVec3 pos_a, UpeVec3 pos_b) {
    UpeVec3 zero = {0.0f, 0.0f, 0.0f};
    if (!bond || bond->broken) return zero;
    float dx = pos_b.x - pos_a.x;
    float dy = pos_b.y - pos_a.y;
    float dz = pos_b.z - pos_a.z;
    float r2 = dx*dx + dy*dy + dz*dz;
    if (r2 < 1e-40f) return zero;
    float r    = sqrtf(r2);
    float fmag = UpeMorse_ForceMagnitude(bond, r);
    float inv_r = 1.0f / r;
    /* If r >> equilibrium and force ≈ 0, bond has dissociated */
    if (r > bond->equilibrium_distance * 5.0f && fabsf(fmag) < 1e-20f) {
        bond->broken = 1;
        return zero;
    }
    /* Force on A: attractive (toward B) when r > re, repulsive when r < re */
    UpeVec3 result = {fmag * dx * inv_r, fmag * dy * inv_r, fmag * dz * inv_r};
    return result;
}

void UpeMorse_PresetHydrogenH2(UpeMorseBond *bond) {
    if (!bond) return;
    bond->dissociation_energy  = 7.24e-19f; /* 4.52 eV */
    bond->equilibrium_distance = 7.4e-11f;  /* 0.74 Angstrom */
    bond->width_parameter      = 1.942e10f; /* 1/m */
    bond->broken               = 0;
}

void UpeMorse_PresetCarbonCarbonSingle(UpeMorseBond *bond) {
    if (!bond) return;
    bond->dissociation_energy  = 5.76e-19f; /* 3.6 eV */
    bond->equilibrium_distance = 1.54e-10f; /* 1.54 Angstrom */
    bond->width_parameter      = 1.8e10f;
    bond->broken               = 0;
}

void UpeMorse_PresetOxygenHydrogen(UpeMorseBond *bond) {
    if (!bond) return;
    bond->dissociation_energy  = 7.57e-19f; /* 4.73 eV */
    bond->equilibrium_distance = 9.6e-11f;  /* 0.96 Angstrom */
    bond->width_parameter      = 2.29e10f;
    bond->broken               = 0;
}
