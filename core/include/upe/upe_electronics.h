#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Electronics / Circuit Physics (EM/Electronics — Phase 8)

  Discrete circuit simulation and electromagnetic interference effects:
    - Resistor-Capacitor (RC) & Resistor-Inductor (RL) transient state
    - Kirchhoff's current & voltage laws (node-voltage solver, up to 16 nodes)
    - LC oscillator natural frequency & resonance quality factor Q
    - Diode I-V (Shockley equation)
    - Transistor switch model (MOSFET saturation / triode region)
    - Battery internal resistance & state of charge discharge curve
    - Ohmic heating power transferred to thermal system (J/s → UpeThermalGrid)
    - EMP pulse momentary disable (energy density threshold)
    - Electromagnetic induction EMF (Faraday: ε = -dΦ/dt)
    - Eddy current braking torque in conductive disc
    - PCB trace current capacity (Onderdonk's equation for fusing)
    - Signal propagation delay (transmission line t = l/v, v = c/sqrt(εr))
*/

#define UPE_ELECTRONICS_MAX_NODES 16

/* ---- Simple lumped-element state ---- */
typedef struct UpeRCState {
    float V_capacitor;       /* current voltage across capacitor */
    float R_ohm;
    float C_farad;
    float V_source;          /* applied voltage */
} UpeRCState;

typedef struct UpeRLState {
    float I_inductor;        /* current through inductor */
    float R_ohm;
    float L_henry;
    float V_source;
} UpeRLState;

typedef struct UpeRLCState {
    float V_capacitor;
    float I_inductor;
    float R_ohm;
    float L_henry;
    float C_farad;
    float V_source;
} UpeRLCState;

/* ---- Simple circuit node ---- */
typedef struct UpeCircuitNode {
    float voltage;           /* solution voltage */
    float current_source;    /* external injected current (A) */
    int   is_ground;         /* 1 = reference node (V=0) */
} UpeCircuitNode;

/* ---- Battery model ---- */
typedef struct UpeBattery {
    float capacity_Ah;       /* total capacity */
    float charge_Ah;         /* current state of charge */
    float V_open_circuit;    /* open circuit voltage */
    float R_internal_ohm;    /* internal series resistance */
    float V_terminal;        /* computed terminal voltage (V_oc - I*R_int) */
} UpeBattery;

/* ---- RC transient --- */
void UpeRC_Step(UpeRCState *rc, float dt);
float UpeRC_TimeConstant(float R, float C);
float UpeRC_VoltageAtTime(float V0, float Vs, float tau, float t);

/* ---- RL transient ---- */
void UpeRL_Step(UpeRLState *rl, float dt);
float UpeRL_TimeConstant(float R, float L);
float UpeRL_CurrentAtTime(float I0, float I_inf, float tau, float t);

/* ---- RLC oscillator ---- */
void UpeRLC_Step(UpeRLCState *rlc, float dt);
float UpeRLC_NaturalFrequency(float L, float C);     /* rad/s */
float UpeRLC_QualityFactor(float L, float C, float R);

/* ---- Diode I-V (Shockley) ----
   I = I_s * (exp(V / (n*Vt)) - 1)  where Vt = kT/q ≈ 0.02585 V at 300K */
float UpeDiode_Current(float V_diode, float I_sat, float ideality_factor, float temp_K);

/* ---- MOSFET switch (simplified) ----
   Returns drain-source current. In saturation: I_D = 0.5*k*(Vgs-Vth)^2 */
float UpeMOSFET_DrainCurrent(float Vgs, float Vds, float Vth, float k_param);

/* ---- Battery discharge ---- */
float UpeBattery_TerminalVoltage(const UpeBattery *bat, float current_A);
float UpeBattery_StateOfCharge(const UpeBattery *bat);
void  UpeBattery_Discharge(UpeBattery *bat, float current_A, float dt_s);

/* ---- Ohmic heating power ---- */
float UpeElec_OhmicHeat(float current_A, float resistance_ohm);  /* Watts = I²R */

/* ---- EMP disable check ----
   Returns 1 if device is disrupted by EMP energy density (J/m²). */
int UpeElec_EMPDisruptCheck(float energy_density_J_m2, float device_threshold_J_m2);

/* ---- Faraday induction ----
   EMF = -d(B·A)/dt = -dFlux/dt  (V).  Provide flux at t and t+dt. */
float UpeElec_InducedEMF(float flux_before_Wb, float flux_after_Wb, float dt);

/* ---- Eddy current braking torque in circular disc ----
   τ = π * σ * B² * t_disc * r⁴ * ω / 4  (simplified)
   σ = conductivity (S/m), t = thickness (m), r = radius (m), ω = angular vel (rad/s) */
float UpeElec_EddyCurrentTorque(float sigma_S_m, float B_tesla,
                                 float disc_thickness_m, float disc_radius_m,
                                 float omega_rad_s);

/* ---- Onderdonk trace fusing: minimum time to fuse a PCB trace (s) ----
   I² * t = k * A² (A in circular-mils, I in Amps)
   Returns fusing time in seconds. 0 if current is below steady-state limit. */
float UpeElec_PcbFuseTime(float current_A, float trace_width_mm,
                           float trace_thickness_um, float ambient_temp_C);

/* ---- Transmission line propagation delay (s) ---- */
float UpeElec_PropDelay(float length_m, float dielectric_const);

#ifdef __cplusplus
}
#endif
