#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Biomechanics Module (Phase 6)

  Muscle-tendon-skeletal mechanics for characters, creatures, and athletic sims:
    - Hill model: muscle force-velocity + force-length curves
    - Metabolic cost (oxygen uptake per unit work)
    - Joint torque limits (flexion / extension per joint type)
    - Tendon spring-damper (series elastic element)
    - Center of Pressure (CoP) balance for standing/walking
    - Fatigue accumulation (Xia & Frey-Law model)
    - Impact injury threshold (Head Injury Criterion, HIC)
    - Breathing cycle (tidal volume, respiratory rate effect on buoyancy)
    - Bone fracture energy threshold (cortical vs. cancellous)
    - Gait cycle: foot-strike timing, stance/swing phase detection
*/

/* ---- Hill muscle model parameters ---- */
typedef struct UpeHillMuscle {
    float F_max_N;          /* maximum isometric force */
    float v_max_ms;         /* maximum shortening velocity (muscle lengths/s) */
    float optimal_length_m; /* optimal fiber length */
    float pennation_angle;  /* rad — angle of fibers to tendon axis */

    /* State */
    float activation;       /* [0,1] neural drive */
    float fiber_length_m;   /* current */
    float contraction_vel;  /* positive = shortening */
    float fatigue_level;    /* [0,1] cumulative fatigue */
} UpeHillMuscle;

/* ---- Tendon SEE (series elastic element) ---- */
typedef struct UpeTendon {
    float rest_length_m;
    float stiffness_N_m;
    float damping_Ns_m;
    float max_force_N;    /* rupture threshold */
} UpeTendon;

/* ---- Simple joint descriptor ---- */
typedef struct UpeBiomechJoint {
    float min_angle_rad;  /* anatomical flexion limit */
    float max_angle_rad;
    float passive_stiffness_Nm_rad;
    float passive_damping_Nms_rad;
} UpeBiomechJoint;

/* ---- Fatigue state (Xia model) ---- */
typedef struct UpeFatigueState {
    float active_fraction;    /* Ma */
    float fatigued_fraction;  /* Mf */
    float resting_fraction;   /* Mr = 1 - Ma - Mf */
    float fatigue_rate;       /* F_fac */
    float recovery_rate;      /* R_fac */
} UpeFatigueState;

/* ---- Gait state ---- */
typedef struct UpeGaitState {
    float stance_start_s;
    float swing_start_s;
    float stride_length_m;
    float cadence_spm;       /* steps per minute */
    int   foot_in_contact;   /* 1 = stance, 0 = swing */
} UpeGaitState;

/* --- Hill force-velocity factor  (F_v): Blix / Hill hyperbola ---
   For shortening: F = F_max * (v_max - v) / (v_max + k*v)  (k ~ 0.25)
   For lengthening: F = F_max * 1.8 * (v_max + v) / (v_max*7.56 + v)
   Returns dimensionless factor in [0, ~1.8]. */
float UpeBiomech_HillForcevelocity(float v_ms, float v_max_ms);

/* --- Hill force-length factor (F_l): Gaussian around optimal length --- */
float UpeBiomech_HillForceLength(float fiber_length_m, float optimal_length_m);

/* --- Total Hill muscle force (N) --- */
float UpeBiomech_HillForce(const UpeHillMuscle *m);

/* --- Tendon force (N) --- */
float UpeBiomech_TendonForce(const UpeTendon *t, float elongation_m, float elongation_rate);

/* --- Passive joint torque (Nm) outside anatomical range --- */
float UpeBiomech_PassiveJointTorque(const UpeBiomechJoint *joint, float angle_rad, float ang_vel);

/* --- Fatigue update (Xia & Frey-Law 2008)
   Ma' = -F_fac * Ma * (F/Fmax) + R_fac * Mf
   Mf' =  F_fac * Ma * (F/Fmax) - R_fac * Mf
   Effective max force = F_max * (1 - Mf) */
void UpeBiomech_FatigueStep(UpeFatigueState *state, float activation, float dt);
float UpeBiomech_FatiguedMaxForce(float F_max_N, const UpeFatigueState *state);

/* --- Metabolic power (W): Margaria model ---
   P_met = a * F_muscle * v_shortening + basal_W */
float UpeBiomech_MetabolicPower(float muscle_force_N, float shortening_vel_ms);

/* --- Center of Pressure from foot contact forces ---
   Given N contact point positions and their normal forces, returns CoP.
   Returns (0,0,0) if total force is zero. */
UpeVec3 UpeBiomech_CenterOfPressure(const UpeVec3 *contact_points,
                                     const float   *normal_forces,
                                     uint32_t       n_contacts);

/* --- Balance quality: distance from CoP to support polygon centroid (m) ---
   Returns positive = good margin, negative = outside polygon. */
float UpeBiomech_BalanceMargin(UpeVec3 cop, UpeVec3 polygon_centroid, float polygon_radius);

/* --- Head Injury Criterion (HIC):
   HIC = max over [t1,t2] of { (t2-t1) * [1/(t2-t1) * integral(a dt)]^2.5 }
   Simplified 2-point version: HIC = dt * ((a1+a2)/2/9.81)^2.5
   Returns HIC value; threshold for severe injury ~ 1000. */
float UpeBiomech_HIC(float avg_accel_ms2, float duration_s);

/* --- Bone fracture threshold (cortical bone): returns 1 if fractured ---
   Stress in MPa; cortical ultimate compressive ~170-200 MPa. */
int UpeBiomech_BoneFractureCheck(float stress_MPa, float material_strength_MPa);

/* --- Respiratory tidal volume effect on buoyancy ---
   Returns extra upward force (N) due to lungs.
   tidal_vol_m3 = breath volume; water_density = 1000 kg/m³. */
float UpeBiomech_LungBuoyancy(float tidal_vol_m3, float breath_fraction,
                               float water_density_kg_m3);

/* --- Gait phase detection ---
   Sets foot_in_contact, updates stride timing. */
void UpeBiomech_UpdateGait(UpeGaitState *gait, int ground_contact, float time_s, float step_length_m);

#ifdef __cplusplus
}
#endif
