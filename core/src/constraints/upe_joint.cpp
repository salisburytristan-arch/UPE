#include "upe/upe_joint.h"
#include "upe/upe_solver.h"
#include <math.h>

static UpeVec3 UpeVec3_ComponentMul(UpeVec3 a, UpeVec3 b) {
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}

static void UpeJoint_AccumulateImpulse(UpeJoint *joint, float impulse) {
    if (!joint || joint->broken) {
        return;
    }
    float mag = impulse < 0.0f ? -impulse : impulse;
    joint->accumulated_impulse += mag;
    if (joint->max_impulse > 0.0f && joint->accumulated_impulse >= joint->max_impulse) {
        joint->broken = 1;
        if (joint->on_break) {
            joint->on_break(joint, joint->break_user_data);
        }
    }
}

void UpeJoint_Init(UpeJoint *joint, const UpeJointDesc *desc) {
    if (!joint || !desc) {
        return;
    }
    joint->type = desc->type;
    joint->body_a = desc->body_a;
    joint->body_b = desc->body_b;
    joint->anchor_a = desc->anchor_a;
    joint->anchor_b = desc->anchor_b;
    joint->axis = desc->axis;
    joint->lambda = 0.0f;
    joint->motor_target_velocity = desc->motor_target_velocity;
    joint->motor_max_force = desc->motor_max_force;
    joint->enable_motor = desc->enable_motor;
    joint->lower_limit = desc->lower_limit;
    joint->upper_limit = desc->upper_limit;
    joint->enable_limit = desc->enable_limit;
    joint->current_angle = 0.0f;
    joint->distance = desc->distance;
    joint->min_distance = desc->min_distance;
    joint->max_distance = desc->max_distance;
    joint->stiffness = desc->stiffness;
    joint->damping = desc->damping;
    joint->lambda_distance = 0.0f;
    joint->cone_angle = desc->cone_angle;
    joint->twist_min = desc->twist_min;
    joint->twist_max = desc->twist_max;
    joint->gear_ratio = desc->gear_ratio;
    joint->pulley_ratio = desc->pulley_ratio;
    joint->pulley_length = desc->pulley_length;
    joint->max_impulse = desc->max_impulse;
    joint->accumulated_impulse = 0.0f;
    joint->broken = 0;
    joint->on_break = desc->on_break;
    joint->break_user_data = desc->break_user_data;
}

void UpeJoint_Solve(UpeJoint *joint, UpeBodyState *bodies, uint32_t body_count) {
    if (!joint || !bodies) {
        return;
    }
    if (joint->broken) {
        return;
    }
    if (joint->body_a >= body_count || joint->body_b >= body_count) {
        return;
    }

    UpeBodyState &a = bodies[joint->body_a];
    UpeBodyState &b = bodies[joint->body_b];

    // Transform anchor points to world space
    UpeVec3 r_a = UpeQuat_RotateVec3(a.rotation, joint->anchor_a);
    UpeVec3 r_b = UpeQuat_RotateVec3(b.rotation, joint->anchor_b);
    UpeVec3 world_anchor_a = UpeVec3_Add(a.position, r_a);
    UpeVec3 world_anchor_b = UpeVec3_Add(b.position, r_b);
    
    // Position error (separation between anchors)
    UpeVec3 pos_error = UpeVec3_Sub(world_anchor_b, world_anchor_a);
    float error_len = UpeVec3_Length(pos_error);
    UpeVec3 error_dir = {0.0f, 0.0f, 0.0f};
    if (error_len > 1.0e-6f) {
        error_dir = UpeVec3_Scale(pos_error, 1.0f / error_len);
    }

    UpeVec3 vel_a = UpeVec3_Add(a.velocity, UpeVec3_Cross(a.angular_velocity, r_a));
    UpeVec3 vel_b = UpeVec3_Add(b.velocity, UpeVec3_Cross(b.angular_velocity, r_b));
    UpeVec3 rel_vel = UpeVec3_Sub(vel_a, vel_b);
    const float baumgarte = 0.1f;

    // Position constraint for joints that keep anchors coincident
    if (joint->type != UPE_JOINT_DISTANCE &&
        joint->type != UPE_JOINT_SPRING &&
        joint->type != UPE_JOINT_GEAR &&
        joint->type != UPE_JOINT_PULLEY) {
        if (error_len < 1.0e-6f) {
            return;
        }

        float constraint_velocity = UpeVec3_Dot(rel_vel, error_dir);

        // Baumgarte stabilization
        float bias = baumgarte * error_len / 0.016f;  // Push to resolve error

        // Compute effective mass with angular terms
        UpeVec3 r_a_cross = UpeVec3_Cross(r_a, error_dir);
        UpeVec3 r_b_cross = UpeVec3_Cross(r_b, error_dir);

        float k = a.inv_mass + b.inv_mass;
        k += UpeVec3_Dot(UpeVec3_ComponentMul(r_a_cross, a.inv_inertia), r_a_cross);
        k += UpeVec3_Dot(UpeVec3_ComponentMul(r_b_cross, b.inv_inertia), r_b_cross);

        if (k < 1.0e-8f) {
            return;
        }

        // Compute impulse magnitude
        float jv = bias - constraint_velocity;
        float lambda = jv / k;
        UpeJoint_AccumulateImpulse(joint, lambda);
        if (joint->broken) {
            return;
        }

        // Apply impulse
        UpeVec3 impulse = UpeVec3_Scale(error_dir, lambda);
        a.velocity = UpeVec3_Add(a.velocity, UpeVec3_Scale(impulse, a.inv_mass));
        a.angular_velocity = UpeVec3_Add(a.angular_velocity,
            UpeVec3_ComponentMul(UpeVec3_Cross(r_a, impulse), a.inv_inertia));
        b.velocity = UpeVec3_Sub(b.velocity, UpeVec3_Scale(impulse, b.inv_mass));
        b.angular_velocity = UpeVec3_Sub(b.angular_velocity,
            UpeVec3_ComponentMul(UpeVec3_Cross(r_b, impulse), b.inv_inertia));
    }
    
    // Distance joint: constrain distance between anchors
    if (joint->type == UPE_JOINT_DISTANCE) {
        if (error_len < 1.0e-6f) {
            return;
        }
        // Check if distance is within limits
        if (error_len < joint->min_distance) {
            // Too close: push apart
            float overlap = joint->min_distance - error_len;
            UpeVec3 separation_impulse = UpeVec3_Scale(error_dir, overlap * 100.0f * a.inv_mass);
            a.velocity = UpeVec3_Add(a.velocity, separation_impulse);
        } else if (error_len > joint->max_distance) {
            // Too far: pull together
            float separation = error_len - joint->max_distance;
            UpeVec3 pulling_direction = UpeVec3_Scale(error_dir, -1.0f);
            UpeVec3 pull_impulse = UpeVec3_Scale(pulling_direction, separation * 100.0f * a.inv_mass);
            a.velocity = UpeVec3_Add(a.velocity, pull_impulse);
        }
    }
    // Spring joint: apply spring force and damping
    else if (joint->type == UPE_JOINT_SPRING) {
        if (error_len < 1.0e-6f) {
            return;
        }
        UpeVec3 rel_vel = UpeVec3_Sub(vel_a, vel_b);
        // Spring force: F = -k * (current_distance - rest_distance)
        float distance_error = error_len - joint->distance;
        
        // Damping force: F = -c * relative_velocity_along_direction
        float spring_velocity = UpeVec3_Dot(rel_vel, error_dir);
        float damping_force = -joint->damping * spring_velocity;
        
        // Total force magnitude
        float force_mag = (-joint->stiffness * distance_error) + damping_force;
        
        UpeVec3 spring_impulse = UpeVec3_Scale(error_dir, force_mag * 0.001f);  // Scale by small dt
        a.velocity = UpeVec3_Add(a.velocity, UpeVec3_Scale(spring_impulse, a.inv_mass));
        b.velocity = UpeVec3_Sub(b.velocity, UpeVec3_Scale(spring_impulse, b.inv_mass));
    }
    else if (joint->type == UPE_JOINT_GEAR) {
        float ratio = joint->gear_ratio;
        if (ratio > -1.0e-6f && ratio < 1.0e-6f) {
            ratio = 1.0f;
        }

        UpeVec3 world_axis = UpeVec3_Normalize(UpeQuat_RotateVec3(a.rotation, joint->axis));
        float w_a = UpeVec3_Dot(a.angular_velocity, world_axis);
        float w_b = UpeVec3_Dot(b.angular_velocity, world_axis);
        float constraint = w_a + ratio * w_b;

        float ang_k_a = UpeVec3_Dot(UpeVec3_ComponentMul(world_axis, a.inv_inertia), world_axis);
        float ang_k_b = UpeVec3_Dot(UpeVec3_ComponentMul(world_axis, b.inv_inertia), world_axis);
        float k = ang_k_a + (ratio * ratio) * ang_k_b;
        if (k > 1.0e-8f) {
            float lambda = -constraint / k;
            UpeJoint_AccumulateImpulse(joint, lambda);
            if (joint->broken) {
                return;
            }
            UpeVec3 impulse_a = UpeVec3_Scale(world_axis, lambda);
            UpeVec3 impulse_b = UpeVec3_Scale(world_axis, ratio * lambda);
            a.angular_velocity = UpeVec3_Add(a.angular_velocity,
                UpeVec3_ComponentMul(impulse_a, a.inv_inertia));
            b.angular_velocity = UpeVec3_Add(b.angular_velocity,
                UpeVec3_ComponentMul(impulse_b, b.inv_inertia));
        }
    }
    else if (joint->type == UPE_JOINT_PULLEY) {
        if (error_len < 1.0e-6f) {
            return;
        }
        float ratio = joint->pulley_ratio;
        if (ratio > -1.0e-6f && ratio < 1.0e-6f) {
            ratio = 1.0f;
        }
        float target_length = joint->pulley_length;
        if (target_length <= 0.0f) {
            target_length = error_len;
        }

        UpeVec3 vel_a = UpeVec3_Add(a.velocity, UpeVec3_Cross(a.angular_velocity, r_a));
        UpeVec3 vel_b = UpeVec3_Add(b.velocity, UpeVec3_Cross(b.angular_velocity, r_b));
        float constraint = (error_len - target_length);
        float constraint_velocity = UpeVec3_Dot(UpeVec3_Sub(vel_a, vel_b), error_dir);

        const float baumgarte = 0.1f;
        float bias = baumgarte * constraint / 0.016f;

        UpeVec3 r_a_cross = UpeVec3_Cross(r_a, error_dir);
        UpeVec3 r_b_cross = UpeVec3_Cross(r_b, error_dir);

        float k = a.inv_mass + (ratio * ratio) * b.inv_mass;
        k += UpeVec3_Dot(UpeVec3_ComponentMul(r_a_cross, a.inv_inertia), r_a_cross);
        k += (ratio * ratio) * UpeVec3_Dot(UpeVec3_ComponentMul(r_b_cross, b.inv_inertia), r_b_cross);

        if (k > 1.0e-8f) {
            float lambda = (bias - constraint_velocity) / k;
            UpeJoint_AccumulateImpulse(joint, lambda);
            if (joint->broken) {
                return;
            }
            UpeVec3 impulse = UpeVec3_Scale(error_dir, lambda);
            UpeVec3 impulse_b = UpeVec3_Scale(impulse, ratio);
            a.velocity = UpeVec3_Add(a.velocity, UpeVec3_Scale(impulse, a.inv_mass));
            a.angular_velocity = UpeVec3_Add(a.angular_velocity,
                UpeVec3_ComponentMul(UpeVec3_Cross(r_a, impulse), a.inv_inertia));
            b.velocity = UpeVec3_Sub(b.velocity, UpeVec3_Scale(impulse_b, b.inv_mass));
            b.angular_velocity = UpeVec3_Sub(b.angular_velocity,
                UpeVec3_ComponentMul(UpeVec3_Cross(r_b, impulse_b), b.inv_inertia));
        }
    }
    
    // For hinge joints, add angular constraint to lock 2 axes
    if (joint->type == UPE_JOINT_HINGE) {
        UpeVec3 world_axis = UpeVec3_Normalize(UpeQuat_RotateVec3(a.rotation, joint->axis));
        
        // Constrain angular velocity perpendicular to hinge axis
        UpeVec3 ang_vel_diff = UpeVec3_Sub(a.angular_velocity, b.angular_velocity);
        
        // Project angular velocity onto plane perpendicular to axis
        UpeVec3 axis_component = UpeVec3_Scale(world_axis, UpeVec3_Dot(ang_vel_diff, world_axis));
        UpeVec3 perp_component = UpeVec3_Sub(ang_vel_diff, axis_component);
        
        float perp_mag = UpeVec3_Length(perp_component);
        if (perp_mag > 1.0e-6f) {
            UpeVec3 perp_dir = UpeVec3_Scale(perp_component, 1.0f / perp_mag);
            
            // Effective angular mass for this axis
            float ang_k = UpeVec3_Dot(UpeVec3_ComponentMul(perp_dir, a.inv_inertia), perp_dir) +
                          UpeVec3_Dot(UpeVec3_ComponentMul(perp_dir, b.inv_inertia), perp_dir);
            
            if (ang_k > 1.0e-8f) {
                float ang_lambda = -perp_mag / ang_k;
                UpeJoint_AccumulateImpulse(joint, ang_lambda);
                if (joint->broken) {
                    return;
                }
                UpeVec3 ang_impulse = UpeVec3_Scale(perp_dir, ang_lambda);
                a.angular_velocity = UpeVec3_Add(a.angular_velocity, 
                    UpeVec3_ComponentMul(ang_impulse, a.inv_inertia));
                b.angular_velocity = UpeVec3_Sub(b.angular_velocity, 
                    UpeVec3_ComponentMul(ang_impulse, b.inv_inertia));
            }
        }
    }
    // Cone-twist: limit swing cone and twist angle around axis
    else if (joint->type == UPE_JOINT_CONE_TWIST) {
        const float dt = 1.0f / 60.0f;
        UpeVec3 world_axis = UpeVec3_Normalize(UpeQuat_RotateVec3(a.rotation, joint->axis));
        UpeVec3 ang_vel_diff = UpeVec3_Sub(a.angular_velocity, b.angular_velocity);

        UpeVec3 twist_component = UpeVec3_Scale(world_axis, UpeVec3_Dot(ang_vel_diff, world_axis));
        UpeVec3 swing_component = UpeVec3_Sub(ang_vel_diff, twist_component);
        float swing_speed = UpeVec3_Length(swing_component);

        if (joint->cone_angle > 0.0f && swing_speed > 1.0e-6f) {
            float swing_angle = swing_speed * dt;
            if (swing_angle > joint->cone_angle) {
                UpeVec3 swing_dir = UpeVec3_Scale(swing_component, 1.0f / swing_speed);
                float ang_k = UpeVec3_Dot(UpeVec3_ComponentMul(swing_dir, a.inv_inertia), swing_dir) +
                              UpeVec3_Dot(UpeVec3_ComponentMul(swing_dir, b.inv_inertia), swing_dir);
                if (ang_k > 1.0e-8f) {
                    float swing_lambda = -(swing_angle - joint->cone_angle) / (ang_k * dt);
                    UpeJoint_AccumulateImpulse(joint, swing_lambda);
                    if (joint->broken) {
                        return;
                    }
                    UpeVec3 swing_impulse = UpeVec3_Scale(swing_dir, swing_lambda);
                    a.angular_velocity = UpeVec3_Add(a.angular_velocity,
                        UpeVec3_ComponentMul(swing_impulse, a.inv_inertia));
                    b.angular_velocity = UpeVec3_Sub(b.angular_velocity,
                        UpeVec3_ComponentMul(swing_impulse, b.inv_inertia));
                }
            }
        }

        float twist_speed = UpeVec3_Dot(ang_vel_diff, world_axis);
        joint->current_angle += twist_speed * dt;
        if (joint->current_angle < joint->twist_min) {
            float limit_error = joint->twist_min - joint->current_angle;
            float ang_k = UpeVec3_Dot(UpeVec3_ComponentMul(world_axis, a.inv_inertia), world_axis) +
                          UpeVec3_Dot(UpeVec3_ComponentMul(world_axis, b.inv_inertia), world_axis);
            if (ang_k > 1.0e-8f) {
                float limit_lambda = (limit_error / dt) / ang_k;
                UpeJoint_AccumulateImpulse(joint, limit_lambda);
                if (joint->broken) {
                    return;
                }
                UpeVec3 limit_impulse = UpeVec3_Scale(world_axis, limit_lambda);
                a.angular_velocity = UpeVec3_Add(a.angular_velocity,
                    UpeVec3_ComponentMul(limit_impulse, a.inv_inertia));
                b.angular_velocity = UpeVec3_Sub(b.angular_velocity,
                    UpeVec3_ComponentMul(limit_impulse, b.inv_inertia));
                joint->current_angle = joint->twist_min;
            }
        } else if (joint->current_angle > joint->twist_max) {
            float limit_error = joint->twist_max - joint->current_angle;
            float ang_k = UpeVec3_Dot(UpeVec3_ComponentMul(world_axis, a.inv_inertia), world_axis) +
                          UpeVec3_Dot(UpeVec3_ComponentMul(world_axis, b.inv_inertia), world_axis);
            if (ang_k > 1.0e-8f) {
                float limit_lambda = (limit_error / dt) / ang_k;
                UpeJoint_AccumulateImpulse(joint, limit_lambda);
                if (joint->broken) {
                    return;
                }
                UpeVec3 limit_impulse = UpeVec3_Scale(world_axis, limit_lambda);
                a.angular_velocity = UpeVec3_Add(a.angular_velocity,
                    UpeVec3_ComponentMul(limit_impulse, a.inv_inertia));
                b.angular_velocity = UpeVec3_Sub(b.angular_velocity,
                    UpeVec3_ComponentMul(limit_impulse, b.inv_inertia));
                joint->current_angle = joint->twist_max;
            }
        }
    }
    // For slider joints, constrain perpendicular motion
    else if (joint->type == UPE_JOINT_SLIDER) {
        UpeVec3 world_axis = UpeVec3_Normalize(UpeQuat_RotateVec3(a.rotation, joint->axis));
        
        // Project position error onto plane perpendicular to slide axis
        UpeVec3 axis_component = UpeVec3_Scale(world_axis, UpeVec3_Dot(pos_error, world_axis));
        UpeVec3 perp_error = UpeVec3_Sub(pos_error, axis_component);
        
        float perp_error_len = UpeVec3_Length(perp_error);
        if (perp_error_len > 1.0e-6f) {
            UpeVec3 perp_dir = UpeVec3_Scale(perp_error, 1.0f / perp_error_len);
            
            // Constrain relative velocity perpendicular to axis
            float perp_vel = UpeVec3_Dot(rel_vel, perp_dir);
            float perp_bias = baumgarte * perp_error_len / 0.016f;
            
            UpeVec3 r_a_cross_perp = UpeVec3_Cross(r_a, perp_dir);
            UpeVec3 r_b_cross_perp = UpeVec3_Cross(r_b, perp_dir);
            
            float perp_k = a.inv_mass + b.inv_mass;
            perp_k += UpeVec3_Dot(UpeVec3_ComponentMul(r_a_cross_perp, a.inv_inertia), r_a_cross_perp);
            perp_k += UpeVec3_Dot(UpeVec3_ComponentMul(r_b_cross_perp, b.inv_inertia), r_b_cross_perp);
            
            if (perp_k > 1.0e-8f) {
                float perp_lambda = (perp_bias - perp_vel) / perp_k;
                UpeJoint_AccumulateImpulse(joint, perp_lambda);
                if (joint->broken) {
                    return;
                }
                UpeVec3 perp_impulse = UpeVec3_Scale(perp_dir, perp_lambda);
                
                a.velocity = UpeVec3_Add(a.velocity, UpeVec3_Scale(perp_impulse, a.inv_mass));
                a.angular_velocity = UpeVec3_Add(a.angular_velocity,
                    UpeVec3_ComponentMul(UpeVec3_Cross(r_a, perp_impulse), a.inv_inertia));
                b.velocity = UpeVec3_Sub(b.velocity, UpeVec3_Scale(perp_impulse, b.inv_mass));
                b.angular_velocity = UpeVec3_Sub(b.angular_velocity,
                    UpeVec3_ComponentMul(UpeVec3_Cross(r_b, perp_impulse), b.inv_inertia));
            }
        }
    }
    
    // Apply motor forces if enabled
    if (joint->enable_motor && joint->motor_max_force > 0.0f) {
        UpeVec3 world_axis = UpeVec3_Normalize(UpeQuat_RotateVec3(a.rotation, joint->axis));
        
        // Compute relative angular velocity along joint axis
        UpeVec3 ang_vel_diff = UpeVec3_Sub(a.angular_velocity, b.angular_velocity);
        float current_ang_vel = UpeVec3_Dot(ang_vel_diff, world_axis);
        
        // Motor error: target velocity - current velocity
        float motor_error = joint->motor_target_velocity - current_ang_vel;
        
        // Compute effective angular mass along axis
        float motor_k = UpeVec3_Dot(UpeVec3_ComponentMul(world_axis, a.inv_inertia), world_axis) +
                       UpeVec3_Dot(UpeVec3_ComponentMul(world_axis, b.inv_inertia), world_axis);
        
        if (motor_k > 1.0e-8f) {
            float motor_impulse = motor_error / motor_k;
            // Clamp to maximum motor force
            float max_impulse = joint->motor_max_force * 0.01667f;  // Assume 60Hz
            motor_impulse = motor_impulse < -max_impulse ? -max_impulse : 
                           (motor_impulse > max_impulse ? max_impulse : motor_impulse);
            UpeJoint_AccumulateImpulse(joint, motor_impulse);
            if (joint->broken) {
                return;
            }
            
            UpeVec3 motor_torque = UpeVec3_Scale(world_axis, motor_impulse);
            a.angular_velocity = UpeVec3_Add(a.angular_velocity, 
                UpeVec3_ComponentMul(motor_torque, a.inv_inertia));
            b.angular_velocity = UpeVec3_Sub(b.angular_velocity, 
                UpeVec3_ComponentMul(motor_torque, b.inv_inertia));
        }
    }
    
    // Enforce angle/position limits if enabled
    if (joint->enable_limit && (joint->type == UPE_JOINT_HINGE || joint->type == UPE_JOINT_SLIDER)) {
        UpeVec3 world_axis = UpeVec3_Normalize(UpeQuat_RotateVec3(a.rotation, joint->axis));
        
        // For hinge: track rotation along axis
        // For slider: track translation along axis
        if (joint->type == UPE_JOINT_HINGE) {
            // Approximate angle from angular velocities integrated over time
            float ang_vel_mag = UpeVec3_Length(UpeVec3_Sub(a.angular_velocity, b.angular_velocity));
            joint->current_angle += ang_vel_mag * 0.016667f;  // 60Hz
            
            // Check limits
            if (joint->current_angle < joint->lower_limit) {
                joint->current_angle = joint->lower_limit;
                // Apply corrective impulse
                float limit_error = joint->lower_limit - joint->current_angle;
                UpeVec3 limit_correction = UpeVec3_Scale(world_axis, limit_error * 10.0f);
                a.angular_velocity = UpeVec3_Add(a.angular_velocity, 
                    UpeVec3_ComponentMul(limit_correction, a.inv_inertia));
            } else if (joint->current_angle > joint->upper_limit) {
                joint->current_angle = joint->upper_limit;
                float limit_error = joint->upper_limit - joint->current_angle;
                UpeVec3 limit_correction = UpeVec3_Scale(world_axis, limit_error * 10.0f);
                a.angular_velocity = UpeVec3_Add(a.angular_velocity, 
                    UpeVec3_ComponentMul(limit_correction, a.inv_inertia));
            }
        }
    }
}
