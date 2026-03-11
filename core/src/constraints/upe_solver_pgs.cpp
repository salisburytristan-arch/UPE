#include "upe/upe_solver.h"
#include <math.h>

static float UpeClamp(float v, float min_v, float max_v) {
    if (v < min_v) return min_v;
    if (v > max_v) return max_v;
    return v;
}

static UpeVec3 UpeVec3_ComponentMul(UpeVec3 a, UpeVec3 b) {
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}

void UpeSolver_PgsIteration(UpeBodyState *bodies, uint32_t body_count,
                            UpeSolverConstraint *constraints, uint32_t constraint_count) {
    if (!bodies || !constraints) {
        return;
    }

    for (uint32_t i = 0; i < constraint_count; ++i) {
        UpeSolverConstraint &c = constraints[i];
        if (c.body_a >= body_count || c.body_b >= body_count) {
            continue;
        }

        UpeBodyState &a = bodies[c.body_a];
        UpeBodyState &b = bodies[c.body_b];

        // === NORMAL IMPULSE (penetration resolution + restitution) ===
        
        // Compute relative velocity at contact point including angular contributions
        UpeVec3 vel_a = UpeVec3_Add(a.velocity, UpeVec3_Cross(a.angular_velocity, c.r_a));
        UpeVec3 vel_b = UpeVec3_Add(b.velocity, UpeVec3_Cross(b.angular_velocity, c.r_b));
        UpeVec3 rel_vel = UpeVec3_Sub(vel_a, vel_b);
        float vel_along_normal = UpeVec3_Dot(rel_vel, c.normal);

        // Baumgarte stabilization bias (pushes bodies apart if penetrating)
        const float baumgarte_coeff = 0.2f;
        const float slop = 0.01f;
        float bias = baumgarte_coeff * (c.penetration > slop ? (c.penetration - slop) : 0.0f) / 0.016f;
        
        // Add restitution bias (bounce) if approaching
        if (vel_along_normal < -1.0f) {  // Only bounce if closing faster than 1 m/s
            bias += c.restitution * -vel_along_normal;
        }

        // Compute effective mass including angular components
        UpeVec3 r_a_cross_n = UpeVec3_Cross(c.r_a, c.normal);
        UpeVec3 r_b_cross_n = UpeVec3_Cross(c.r_b, c.normal);
        
        float k_normal = a.inv_mass + b.inv_mass;
        k_normal += UpeVec3_Dot(UpeVec3_ComponentMul(r_a_cross_n, a.inv_inertia), r_a_cross_n);
        k_normal += UpeVec3_Dot(UpeVec3_ComponentMul(r_b_cross_n, b.inv_inertia), r_b_cross_n);
        
        // Add CFM (Constraint Force Mixing) for soft constraints
        // Larger CFM makes constraint softer/more compliant
        k_normal += c.cfm;
        
        if (k_normal < 1.0e-8f) {
            continue;
        }

        // Compute normal impulse magnitude
        float jv = bias - vel_along_normal;
        float delta_lambda = jv / k_normal;
        float old_lambda = c.lambda;
        c.lambda = (old_lambda + delta_lambda > 0.0f) ? (old_lambda + delta_lambda) : 0.0f;  // Clamp to non-negative
        delta_lambda = c.lambda - old_lambda;

        // Apply normal impulse
        UpeVec3 impulse_n = UpeVec3_Scale(c.normal, delta_lambda);
        a.velocity = UpeVec3_Add(a.velocity, UpeVec3_Scale(impulse_n, a.inv_mass));
        a.angular_velocity = UpeVec3_Add(a.angular_velocity, UpeVec3_ComponentMul(UpeVec3_Cross(c.r_a, impulse_n), a.inv_inertia));
        b.velocity = UpeVec3_Sub(b.velocity, UpeVec3_Scale(impulse_n, b.inv_mass));
        b.angular_velocity = UpeVec3_Sub(b.angular_velocity, UpeVec3_ComponentMul(UpeVec3_Cross(c.r_b, impulse_n), b.inv_inertia));

        // === FRICTION IMPULSES (tangent directions with static/dynamic split) ===
        
        if (c.friction > 0.0f || (c.friction_static > 0.0f && c.friction_dynamic > 0.0f)) {
            // Recompute relative velocity after normal impulse
            vel_a = UpeVec3_Add(a.velocity, UpeVec3_Cross(a.angular_velocity, c.r_a));
            vel_b = UpeVec3_Add(b.velocity, UpeVec3_Cross(b.angular_velocity, c.r_b));
            rel_vel = UpeVec3_Sub(vel_a, vel_b);
            
            // Compute tangential velocity (remove normal component)
            float vel_normal = UpeVec3_Dot(rel_vel, c.normal);
            UpeVec3 vel_tangent = UpeVec3_Sub(rel_vel, UpeVec3_Scale(c.normal, vel_normal));
            float tangent_speed = UpeVec3_Length(vel_tangent);
            
            // Determine friction coefficient based on static/dynamic split
            float active_friction = c.friction;
            const float stick_threshold = 0.1f;  // m/s velocity threshold for static friction
            
            if (c.friction_static > 0.0f && c.friction_dynamic > 0.0f) {
                // Static/dynamic friction model
                if (tangent_speed < stick_threshold) {
                    active_friction = c.friction_static;  // Sticking
                    c.is_sliding = 0;
                } else {
                    active_friction = c.friction_dynamic;  // Sliding
                    c.is_sliding = 1;
                }
            }
            
            // Tangent 1 friction
            float vel_t1 = UpeVec3_Dot(rel_vel, c.tangent1);
            UpeVec3 r_a_cross_t1 = UpeVec3_Cross(c.r_a, c.tangent1);
            UpeVec3 r_b_cross_t1 = UpeVec3_Cross(c.r_b, c.tangent1);
            float k_t1 = a.inv_mass + b.inv_mass;
            k_t1 += UpeVec3_Dot(UpeVec3_ComponentMul(r_a_cross_t1, a.inv_inertia), r_a_cross_t1);
            k_t1 += UpeVec3_Dot(UpeVec3_ComponentMul(r_b_cross_t1, b.inv_inertia), r_b_cross_t1);
            
            // Add CFM for soft tangential constraints
            k_t1 += c.cfm;
            
            if (k_t1 > 1.0e-8f) {
                float delta_lambda_t1 = -vel_t1 / k_t1;
                // Apply anisotropic friction: reduce max friction for tangent1 by anisotropy factor
                // anisotropy > 1.0 = slippery along tangent1 (reduce friction)
                // anisotropy < 1.0 = stickier along tangent1 (increase friction)
                float max_friction = active_friction * c.lambda * (1.0f / c.friction_anisotropy);
                float old_lambda_t1 = c.lambda_t1;
                c.lambda_t1 = UpeClamp(old_lambda_t1 + delta_lambda_t1, -max_friction, max_friction);
                delta_lambda_t1 = c.lambda_t1 - old_lambda_t1;
                
                UpeVec3 impulse_t1 = UpeVec3_Scale(c.tangent1, delta_lambda_t1);
                a.velocity = UpeVec3_Add(a.velocity, UpeVec3_Scale(impulse_t1, a.inv_mass));
                a.angular_velocity = UpeVec3_Add(a.angular_velocity, UpeVec3_ComponentMul(UpeVec3_Cross(c.r_a, impulse_t1), a.inv_inertia));
                b.velocity = UpeVec3_Sub(b.velocity, UpeVec3_Scale(impulse_t1, b.inv_mass));
                b.angular_velocity = UpeVec3_Sub(b.angular_velocity, UpeVec3_ComponentMul(UpeVec3_Cross(c.r_b, impulse_t1), b.inv_inertia));
            }
            
            // Tangent 2 friction
            float vel_t2 = UpeVec3_Dot(rel_vel, c.tangent2);
            UpeVec3 r_a_cross_t2 = UpeVec3_Cross(c.r_a, c.tangent2);
            UpeVec3 r_b_cross_t2 = UpeVec3_Cross(c.r_b, c.tangent2);
            float k_t2 = a.inv_mass + b.inv_mass;
            k_t2 += UpeVec3_Dot(UpeVec3_ComponentMul(r_a_cross_t2, a.inv_inertia), r_a_cross_t2);
            k_t2 += UpeVec3_Dot(UpeVec3_ComponentMul(r_b_cross_t2, b.inv_inertia), r_b_cross_t2);
            
            // Add CFM for soft tangential constraints
            k_t2 += c.cfm;
            
            if (k_t2 > 1.0e-8f) {
                float delta_lambda_t2 = -vel_t2 / k_t2;
                float max_friction = active_friction * c.lambda;
                float old_lambda_t2 = c.lambda_t2;
                c.lambda_t2 = UpeClamp(old_lambda_t2 + delta_lambda_t2, -max_friction, max_friction);
                delta_lambda_t2 = c.lambda_t2 - old_lambda_t2;
                
                UpeVec3 impulse_t2 = UpeVec3_Scale(c.tangent2, delta_lambda_t2);
                a.velocity = UpeVec3_Add(a.velocity, UpeVec3_Scale(impulse_t2, a.inv_mass));
                a.angular_velocity = UpeVec3_Add(a.angular_velocity, UpeVec3_ComponentMul(UpeVec3_Cross(c.r_a, impulse_t2), a.inv_inertia));
                b.velocity = UpeVec3_Sub(b.velocity, UpeVec3_Scale(impulse_t2, b.inv_mass));
                b.angular_velocity = UpeVec3_Sub(b.angular_velocity, UpeVec3_ComponentMul(UpeVec3_Cross(c.r_b, impulse_t2), b.inv_inertia));
            }
        }
    }
}
