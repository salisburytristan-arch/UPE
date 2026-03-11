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
