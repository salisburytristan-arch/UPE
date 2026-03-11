#include "upe/upe_aerodynamics.h"
#include <math.h>

void UpeAerodynamics_Init(UpeAerodynamics *aero) {
    if (!aero) {
        return;
    }
    aero->drag_coefficient = 0.47f;
    aero->lift_coefficient = 0.0f;
    aero->reference_area = 1.0f;
    aero->air_density = 1.225f;
}

UpeVec3 UpeAerodynamics_ComputeForce(const UpeAerodynamics *aero, UpeVec3 velocity, UpeVec3 wind) {
    UpeVec3 zero_force = {0.0f, 0.0f, 0.0f};
    if (!aero) {
        return zero_force;
    }
    
    UpeVec3 relative_vel = UpeVec3_Sub(velocity, wind);
    float speed_sq = relative_vel.x * relative_vel.x + 
                     relative_vel.y * relative_vel.y + 
                     relative_vel.z * relative_vel.z;
    
    if (speed_sq < 1.0e-6f) {
        return zero_force;
    }
    
    float speed = sqrtf(speed_sq);
    UpeVec3 direction = UpeVec3_Scale(relative_vel, 1.0f / speed);
    
    float drag_magnitude = 0.5f * aero->air_density * speed_sq * 
                          aero->drag_coefficient * aero->reference_area;
    
    UpeVec3 drag_force = UpeVec3_Scale(direction, -drag_magnitude);

    UpeVec3 lift_force = zero_force;
    if (aero->lift_coefficient > 0.0f) {
        UpeVec3 world_up = {0.0f, 1.0f, 0.0f};
        UpeVec3 side_axis = UpeVec3_Cross(direction, world_up);
        float side_len_sq = UpeVec3_Dot(side_axis, side_axis);

        if (side_len_sq > 1.0e-8f) {
            float side_len = sqrtf(side_len_sq);
            side_axis = UpeVec3_Scale(side_axis, 1.0f / side_len);

            UpeVec3 lift_dir = UpeVec3_Cross(side_axis, direction);
            float lift_len_sq = UpeVec3_Dot(lift_dir, lift_dir);
            if (lift_len_sq > 1.0e-8f) {
                float lift_len = sqrtf(lift_len_sq);
                lift_dir = UpeVec3_Scale(lift_dir, 1.0f / lift_len);

                float lift_magnitude = 0.5f * aero->air_density * speed_sq *
                                       aero->lift_coefficient * aero->reference_area;
                lift_force = UpeVec3_Scale(lift_dir, lift_magnitude);
            }
        }
    }

    return UpeVec3_Add(drag_force, lift_force);
}

/* Bernoulli — returns P2:  P2 = P1 + 0.5*rho*(v1^2 - v2^2) */
float UpeAero_BernoulliPressure(float rho, float v1, float P1, float v2) {
    return P1 + 0.5f * rho * (v1 * v1 - v2 * v2);
}

/* Simplified ISA barometric formula:  P = P0 * exp(-g*M_air*h / (R*T0))
   g=9.80665, M_air=0.028964, R=8.31446  =>  scale height ~8434 m at T0=288.15 K */
float UpeAero_BarometricPressure(float P0, float altitude_m, float T0_K) {
    if (T0_K <= 0.0f) T0_K = 288.15f;
    /* Lapse rate simplified: P = P0 * (1 - L*h/T0)^5.256 */
    float L = 0.0065f;   /* K/m, ISA troposphere */
    float r = 1.0f - L * altitude_m / T0_K;
    if (r <= 0.0f) return 0.0f;
    /* pow approximation via expf(5.256 * logf(r)) */
    return P0 * expf(5.256f * logf(r));
}

/* Isentropic relation (gamma=1.4 air):  P_s = P_t * (1 + 0.2*M^2)^-3.5  */
float UpeAero_StaticPressureAtMach(float P_total, float mach) {
    float base = 1.0f + 0.2f * mach * mach;
    return P_total / expf(3.5f * logf(base));
}
