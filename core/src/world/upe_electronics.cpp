#include "upe/upe_electronics.h"
#include <math.h>
#include <string.h>

#define UPE_C_LIGHT  2.998e8f     /* m/s */
#define UPE_KB       1.3806e-23f  /* Boltzmann constant */
#define UPE_Q_ELEC   1.6022e-19f  /* elementary charge */

/* -----------------------------------------------------------------------
   RC
   ----------------------------------------------------------------------- */

void UpeRC_Step(UpeRCState *rc, float dt) {
    if (!rc || rc->C_farad <= 0.0f || rc->R_ohm <= 0.0f) return;
    float tau = rc->R_ohm * rc->C_farad;
    float alpha = dt / tau;
    /* Exact: V(t+dt) = Vs + (V - Vs)*exp(-dt/tau) */
    rc->V_capacitor = rc->V_source + (rc->V_capacitor - rc->V_source) * expf(-alpha);
}

float UpeRC_TimeConstant(float R, float C) { return R * C; }

float UpeRC_VoltageAtTime(float V0, float Vs, float tau, float t) {
    if (tau <= 0.0f) return Vs;
    return Vs + (V0 - Vs) * expf(-t / tau);
}

/* -----------------------------------------------------------------------
   RL
   ----------------------------------------------------------------------- */

void UpeRL_Step(UpeRLState *rl, float dt) {
    if (!rl || rl->L_henry <= 0.0f || rl->R_ohm <= 0.0f) return;
    float tau = rl->L_henry / rl->R_ohm;
    float I_inf = rl->V_source / rl->R_ohm;
    rl->I_inductor = I_inf + (rl->I_inductor - I_inf) * expf(-dt / tau);
}

float UpeRL_TimeConstant(float R, float L) { return (R > 0.0f) ? L / R : 0.0f; }

float UpeRL_CurrentAtTime(float I0, float I_inf, float tau, float t) {
    if (tau <= 0.0f) return I_inf;
    return I_inf + (I0 - I_inf) * expf(-t / tau);
}

/* -----------------------------------------------------------------------
   RLC oscillator (state: V_cap = x, I_ind = y)
   dx/dt = y/C,  dy/dt = (Vs - y*R - x) / L
   ----------------------------------------------------------------------- */

void UpeRLC_Step(UpeRLCState *rlc, float dt) {
    if (!rlc || rlc->L_henry <= 0.0f || rlc->C_farad <= 0.0f) return;
    float V = rlc->V_capacitor;
    float I = rlc->I_inductor;

    float dV = I / rlc->C_farad;
    float dI = (rlc->V_source - I * rlc->R_ohm - V) / rlc->L_henry;

    rlc->V_capacitor += dV * dt;
    rlc->I_inductor  += dI * dt;
}

float UpeRLC_NaturalFrequency(float L, float C) {
    if (L <= 0.0f || C <= 0.0f) return 0.0f;
    return 1.0f / sqrtf(L * C);
}

float UpeRLC_QualityFactor(float L, float C, float R) {
    if (R <= 0.0f || L <= 0.0f || C <= 0.0f) return 0.0f;
    return (1.0f / R) * sqrtf(L / C);
}

/* -----------------------------------------------------------------------
   Diode Shockley
   ----------------------------------------------------------------------- */

float UpeDiode_Current(float V_diode, float I_sat, float ideality_factor, float temp_K) {
    if (temp_K <= 0.0f || ideality_factor <= 0.0f) return 0.0f;
    float Vt = UPE_KB * temp_K / UPE_Q_ELEC;  /* thermal voltage ~0.02585 V at 300K */
    float exp_arg = V_diode / (ideality_factor * Vt);
    if (exp_arg > 80.0f) exp_arg = 80.0f;
    return I_sat * (expf(exp_arg) - 1.0f);
}

/* -----------------------------------------------------------------------
   MOSFET (NMOS, enhancement mode, simplified square-law)
   ----------------------------------------------------------------------- */

float UpeMOSFET_DrainCurrent(float Vgs, float Vds, float Vth, float k_param) {
    float overdrive = Vgs - Vth;
    if (overdrive <= 0.0f) return 0.0f;  /* cutoff */
    if (Vds >= overdrive) {
        /* Saturation: I_D = (k/2)*(Vgs-Vth)² */
        return 0.5f * k_param * overdrive * overdrive;
    } else {
        /* Linear/triode: I_D = k*((Vgs-Vth)*Vds - Vds²/2) */
        return k_param * (overdrive * Vds - 0.5f * Vds * Vds);
    }
}

/* -----------------------------------------------------------------------
   Battery
   ----------------------------------------------------------------------- */

float UpeBattery_TerminalVoltage(const UpeBattery *bat, float current_A) {
    if (!bat) return 0.0f;
    return bat->V_open_circuit - current_A * bat->R_internal_ohm;
}

float UpeBattery_StateOfCharge(const UpeBattery *bat) {
    if (!bat || bat->capacity_Ah <= 0.0f) return 0.0f;
    return bat->charge_Ah / bat->capacity_Ah;
}

void UpeBattery_Discharge(UpeBattery *bat, float current_A, float dt_s) {
    if (!bat || dt_s <= 0.0f) return;
    float dQ = current_A * dt_s / 3600.0f;  /* convert seconds to Ah */
    bat->charge_Ah -= dQ;
    if (bat->charge_Ah < 0.0f) bat->charge_Ah = 0.0f;
    bat->V_terminal = UpeBattery_TerminalVoltage(bat, current_A);
    /* Simplified OCV curve: V_oc drops linearly 10% from full to empty */
    float soc = UpeBattery_StateOfCharge(bat);
    bat->V_open_circuit = bat->V_open_circuit * (0.9f + 0.1f * soc);
}

/* -----------------------------------------------------------------------
   Ohmic heating
   ----------------------------------------------------------------------- */

float UpeElec_OhmicHeat(float current_A, float resistance_ohm) {
    return current_A * current_A * resistance_ohm;
}

/* -----------------------------------------------------------------------
   EMP disable check
   ----------------------------------------------------------------------- */

int UpeElec_EMPDisruptCheck(float energy_density_J_m2, float device_threshold_J_m2) {
    if (device_threshold_J_m2 <= 0.0f) return 1;
    return (energy_density_J_m2 >= device_threshold_J_m2) ? 1 : 0;
}

/* -----------------------------------------------------------------------
   Faraday induction
   ----------------------------------------------------------------------- */

float UpeElec_InducedEMF(float flux_before_Wb, float flux_after_Wb, float dt) {
    if (dt <= 0.0f) return 0.0f;
    return -(flux_after_Wb - flux_before_Wb) / dt;
}

/* -----------------------------------------------------------------------
   Eddy current braking torque
   τ = (π * σ * B² * t * r⁴ * ω) / 4
   ----------------------------------------------------------------------- */

float UpeElec_EddyCurrentTorque(float sigma_S_m, float B_tesla,
                                 float disc_thickness_m, float disc_radius_m,
                                 float omega_rad_s) {
    const float PI = 3.14159265f;
    return (PI * sigma_S_m * B_tesla * B_tesla * disc_thickness_m *
            disc_radius_m * disc_radius_m * disc_radius_m * disc_radius_m *
            omega_rad_s) / 4.0f;
}

/* -----------------------------------------------------------------------
   Onderdonk fusing time (simplified for PCB copper trace)
   t_fuse = (A * Tm / I)^2  (simplified Onderdonk)
   where A = cross-section area (mm²), Tm = temp rise to melt (~700°C for Cu)
   ----------------------------------------------------------------------- */

float UpeElec_PcbFuseTime(float current_A, float trace_width_mm,
                           float trace_thickness_um, float ambient_temp_C) {
    if (current_A <= 0.0f || trace_width_mm <= 0.0f) return 1e30f;
    /* Cross-section area (mm²) */
    float area_mm2 = trace_width_mm * trace_thickness_um * 1e-3f;
    /* Copper melting 1083°C; rise = 1083 - ambient */
    float dT = 1083.0f - ambient_temp_C;
    if (dT <= 0.0f) return 0.0f;
    /* Onderdonk: I = A * sqrt(   (ln(Tm_rise/234 + 1)) / (0.0842 * t)  ) */
    /* Inverted: t = A^2 * ln(Tm_rise/234 + 1) / (0.0842 * I^2) */
    float log_term = logf(dT / 234.0f + 1.0f);
    float t_fuse = area_mm2 * area_mm2 * log_term / (0.0842f * current_A * current_A);
    return t_fuse;
}

/* -----------------------------------------------------------------------
   Transmission line propagation delay
   ----------------------------------------------------------------------- */

float UpeElec_PropDelay(float length_m, float dielectric_const) {
    if (dielectric_const <= 0.0f) return 0.0f;
    float v = UPE_C_LIGHT / sqrtf(dielectric_const);
    return (v > 0.0f) ? length_m / v : 0.0f;
}
