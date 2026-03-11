#include "upe/upe_biomechanics.h"
#include <math.h>
#include <string.h>

#define UPE_PI 3.14159265358979f

/* -----------------------------------------------------------------------
   Hill model
   ----------------------------------------------------------------------- */

float UpeBiomech_HillForcevelocity(float v_ms, float v_max_ms) {
    if (v_max_ms <= 0.0f) return 0.0f;
    const float k = 0.25f;        /* Hill constant, dimensionless */
    if (v_ms <= 0.0f) {
        /* Shortening: F_v = (v_max - v) / (v_max + k*v), v > 0 */
        float v_pos = -v_ms;      /* shortening speed (positive) */
        if (v_pos >= v_max_ms) return 0.0f;
        return (v_max_ms - v_pos) / (v_max_ms + k * v_pos);
    } else {
        /* Lengthening (eccentric): extended Hill
           F_v = (b - a*v) / (b + v),  a ~ 1.8*F_max normalization => factor up to 1.8 */
        float b = v_max_ms * (0.25f + 0.75f);  /* ~v_max */
        float a_coef = 1.8f;
        return (a_coef * b - (a_coef - 1.0f) * v_ms) / (b + v_ms);
    }
}

float UpeBiomech_HillForceLength(float fiber_length_m, float optimal_length_m) {
    if (optimal_length_m <= 0.0f) return 0.0f;
    float rel = fiber_length_m / optimal_length_m;
    /* Gaussian: F_l = exp(-((l/l0 - 1)^2) / (2*0.25^2)) */
    float x = (rel - 1.0f) / 0.25f;
    return expf(-0.5f * x * x);
}

float UpeBiomech_HillForce(const UpeHillMuscle *m) {
    if (!m || m->F_max_N <= 0.0f) return 0.0f;
    float Fl = UpeBiomech_HillForceLength(m->fiber_length_m, m->optimal_length_m);
    float Fv = UpeBiomech_HillForcevelocity(m->contraction_vel, m->v_max_ms);
    float F_active = m->activation * m->F_max_N * Fl * Fv;
    /* Pennation angle projection */
    float cos_pen = cosf(m->pennation_angle);
    return F_active * cos_pen;
}

/* -----------------------------------------------------------------------
   Tendon SEE
   ----------------------------------------------------------------------- */

float UpeBiomech_TendonForce(const UpeTendon *t, float elongation_m, float elongation_rate) {
    if (!t || elongation_m < 0.0f) return 0.0f;
    float F = t->stiffness_N_m * elongation_m + t->damping_Ns_m * elongation_rate;
    if (F < 0.0f) F = 0.0f;
    if (t->max_force_N > 0.0f && F > t->max_force_N) F = t->max_force_N;
    return F;
}

/* -----------------------------------------------------------------------
   Passive joint torque
   ----------------------------------------------------------------------- */

float UpeBiomech_PassiveJointTorque(const UpeBiomechJoint *joint,
                                     float angle_rad, float ang_vel) {
    if (!joint) return 0.0f;
    float T = 0.0f;
    if (angle_rad < joint->min_angle_rad) {
        T = joint->passive_stiffness_Nm_rad * (joint->min_angle_rad - angle_rad);
    } else if (angle_rad > joint->max_angle_rad) {
        T = joint->passive_stiffness_Nm_rad * (joint->max_angle_rad - angle_rad);
    }
    T -= joint->passive_damping_Nms_rad * ang_vel;
    return T;
}

/* -----------------------------------------------------------------------
   Fatigue (Xia & Frey-Law 2008)
   ----------------------------------------------------------------------- */

void UpeBiomech_FatigueStep(UpeFatigueState *state, float activation, float dt) {
    if (!state || dt <= 0.0f) return;
    float Ma = state->active_fraction;
    float Mf = state->fatigued_fraction;
    float F_ = state->fatigue_rate;
    float R_ = state->recovery_rate;

    float dMa = -F_ * Ma * activation + R_ * Mf;
    float dMf =  F_ * Ma * activation - R_ * Mf;

    state->active_fraction   = Ma + dMa * dt;
    state->fatigued_fraction = Mf + dMf * dt;

    /* Clamp */
    if (state->active_fraction   < 0.0f) state->active_fraction   = 0.0f;
    if (state->fatigued_fraction < 0.0f) state->fatigued_fraction = 0.0f;
    float sum = state->active_fraction + state->fatigued_fraction;
    if (sum > 1.0f) {
        state->active_fraction   /= sum;
        state->fatigued_fraction /= sum;
    }
    state->resting_fraction = 1.0f - state->active_fraction - state->fatigued_fraction;
    if (state->resting_fraction < 0.0f) state->resting_fraction = 0.0f;
}

float UpeBiomech_FatiguedMaxForce(float F_max_N, const UpeFatigueState *state) {
    if (!state) return F_max_N;
    return F_max_N * (1.0f - state->fatigued_fraction);
}

/* -----------------------------------------------------------------------
   Metabolic power (Margaria approximation)
   ----------------------------------------------------------------------- */

float UpeBiomech_MetabolicPower(float muscle_force_N, float shortening_vel_ms) {
    const float basal_W    = 1.5f;   /* basal metabolic rate equivalent (W) */
    const float efficiency = 0.25f;  /* muscle mechanical to metabolic efficiency */
    float mech_power = muscle_force_N * shortening_vel_ms;
    if (mech_power < 0.0f) mech_power = 0.0f;
    return mech_power / efficiency + basal_W;
}

/* -----------------------------------------------------------------------
   Center of Pressure
   ----------------------------------------------------------------------- */

UpeVec3 UpeBiomech_CenterOfPressure(const UpeVec3 *contact_points,
                                     const float   *normal_forces,
                                     uint32_t       n_contacts) {
    UpeVec3 cop = {0.0f, 0.0f, 0.0f};
    if (!contact_points || !normal_forces || n_contacts == 0) return cop;
    float total = 0.0f;
    for (uint32_t i = 0; i < n_contacts; ++i) {
        float f = normal_forces[i] > 0.0f ? normal_forces[i] : 0.0f;
        cop.x += contact_points[i].x * f;
        cop.y += contact_points[i].y * f;
        cop.z += contact_points[i].z * f;
        total += f;
    }
    if (total > 0.0f) {
        cop.x /= total; cop.y /= total; cop.z /= total;
    }
    return cop;
}

float UpeBiomech_BalanceMargin(UpeVec3 cop, UpeVec3 polygon_centroid, float polygon_radius) {
    float dx = cop.x - polygon_centroid.x;
    float dz = cop.z - polygon_centroid.z;
    float dist = sqrtf(dx*dx + dz*dz);
    return polygon_radius - dist;
}

/* -----------------------------------------------------------------------
   Head Injury Criterion
   ----------------------------------------------------------------------- */

float UpeBiomech_HIC(float avg_accel_ms2, float duration_s) {
    if (duration_s <= 0.0f) return 0.0f;
    /* HIC = dt * (avg_a / g)^2.5 */
    float g_units = avg_accel_ms2 / 9.81f;
    return duration_s * powf(g_units, 2.5f);
}

/* -----------------------------------------------------------------------
   Bone fracture
   ----------------------------------------------------------------------- */

int UpeBiomech_BoneFractureCheck(float stress_MPa, float material_strength_MPa) {
    if (material_strength_MPa <= 0.0f) return 1;
    return (fabsf(stress_MPa) >= material_strength_MPa) ? 1 : 0;
}

/* -----------------------------------------------------------------------
   Lung buoyancy
   ----------------------------------------------------------------------- */

float UpeBiomech_LungBuoyancy(float tidal_vol_m3, float breath_fraction,
                               float water_density_kg_m3) {
    const float g = 9.81f;
    /* Functional residual capacity + tidal fraction */
    float lung_vol = tidal_vol_m3 * breath_fraction;
    return water_density_kg_m3 * g * lung_vol;
}

/* -----------------------------------------------------------------------
   Gait phase detection
   ----------------------------------------------------------------------- */

void UpeBiomech_UpdateGait(UpeGaitState *gait, int ground_contact,
                            float time_s, float step_length_m) {
    if (!gait) return;
    int was_contact = gait->foot_in_contact;
    gait->foot_in_contact = ground_contact;

    if (!was_contact && ground_contact) {
        /* heel strike */
        if (gait->swing_start_s > 0.0f) {
            float stride_t = time_s - gait->stance_start_s;
            if (stride_t > 0.0f)
                gait->cadence_spm = 60.0f / stride_t;
        }
        gait->stance_start_s  = time_s;
        gait->stride_length_m = step_length_m;
    } else if (was_contact && !ground_contact) {
        /* toe-off */
        gait->swing_start_s = time_s;
    }
}
