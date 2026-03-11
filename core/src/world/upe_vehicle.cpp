#include "upe/upe_vehicle.h"
#include "upe/upe_api.h"
#include "upe/upe_body.h"
#include <math.h>
#include "upe/upe_queries.h"

static void UpeVehicle_ApplyForceAtPoint(UpeBody *body, UpeVec3 force, UpeVec3 point)
{
    if (!body) {
        return;
    }
    UpeBody_ApplyForce(body, force);
    UpeVec3 r = UpeVec3_Sub(point, body->position);
    UpeBody_ApplyTorque(body, UpeVec3_Cross(r, force));
}

void UpeVehicle_Init(UpeVehicle *vehicle, const UpeVehicleDesc *desc) {
    if (!vehicle || !desc) {
        return;
    }
    vehicle->chassis_body = desc->chassis_body;
    vehicle->wheel_count = desc->wheel_count < 4 ? desc->wheel_count : 4;
    vehicle->suspension_stiffness = desc->suspension_stiffness;
    vehicle->suspension_damping = desc->suspension_damping;
    vehicle->suspension_cfm = 0.01f;
    vehicle->wheel_friction = 1.0f;
    vehicle->drive_train_torque = 0.0f;
    vehicle->use_cfm_suspension = 0;

    for (uint32_t i = 0; i < vehicle->wheel_count; ++i) {
        UpeWheel *wheel = &vehicle->wheels[i];
        wheel->body_id = (uint32_t)-1;
        wheel->radius = desc->wheel_radius;
        wheel->suspension_length = 0.5f;
        wheel->suspension_stiffness = vehicle->suspension_stiffness;
        wheel->suspension_damping = vehicle->suspension_damping;
        wheel->suspension_force = 0.0f;
        wheel->suspension_velocity = 0.0f;
        wheel->compression_previous = 0.0f;
        wheel->contact_normal = {0.0f, 1.0f, 0.0f};
        wheel->contact_point = {0.0f, 0.0f, 0.0f};
        wheel->is_touching = 0;
        wheel->grip_limit = 5000.0f;
        
        /* Initialize tire model state */
        wheel->slip_angle = 0.0f;
        wheel->slip_ratio = 0.0f;
        wheel->lateral_force = 0.0f;
        wheel->longitudinal_force = 0.0f;
        wheel->angular_velocity = 0.0f;
        wheel->steering_angle = 0.0f;

        // Setup wheel positions (default 4-wheel vehicle)
        if (i == 0) {
            wheel->connection_point = {-0.8f, -0.3f, 1.0f};
            wheel->wheel_direction = {0.0f, -1.0f, 0.0f};
            wheel->steering_axis = {0.0f, 1.0f, 0.0f};
        } else if (i == 1) {
            wheel->connection_point = {0.8f, -0.3f, 1.0f};
            wheel->wheel_direction = {0.0f, -1.0f, 0.0f};
            wheel->steering_axis = {0.0f, 1.0f, 0.0f};
        } else if (i == 2) {
            wheel->connection_point = {-0.8f, -0.3f, -1.0f};
            wheel->wheel_direction = {0.0f, -1.0f, 0.0f};
            wheel->steering_axis = {0.0f, 0.0f, 1.0f};
        } else {
            wheel->connection_point = {0.8f, -0.3f, -1.0f};
            wheel->wheel_direction = {0.0f, -1.0f, 0.0f};
            wheel->steering_axis = {0.0f, 0.0f, 1.0f};
        }
    }
}

void UpeVehicle_UpdateSuspension(UpeVehicle *vehicle, UpeBody *chassis, float dt) {
    if (!vehicle || !chassis) return;

    // World-unaware wrapper: keep old behavior if no world is provided.
    // New world-aware function can be called externally if desired.
    for (uint32_t i = 0; i < vehicle->wheel_count; ++i) {
        UpeWheel *wheel = &vehicle->wheels[i];

        // Transform wheel connection point to world space
        UpeVec3 world_connection = UpeVec3_Add(
            chassis->position,
            UpeQuat_RotateVec3(chassis->rotation, wheel->connection_point)
        );

        // Fallback simple plane ground at y=0 to preserve existing behavior
        float ground_y = 0.0f;
        float wheel_y = world_connection.y;
        float compression = fmaxf(0.0f, (wheel_y - wheel->radius) - ground_y);

        wheel->is_touching = compression < wheel->suspension_length ? 1 : 0;

        if (wheel->is_touching && compression > 0.001f) {
            compression = fminf(compression, wheel->suspension_length);

            // Calculate suspension velocity (for damping)
            wheel->suspension_velocity = (compression - wheel->compression_previous) / fmaxf(dt, 0.001f);
            wheel->compression_previous = compression;

            // Spring force: F = -k * x (Hooke's law)
            float spring_force = -wheel->suspension_stiffness * compression;

            // Damping force: F = -c * v (viscous damping)
            float damping_force = -wheel->suspension_damping * wheel->suspension_velocity;

            // Total suspension force
            wheel->suspension_force = spring_force + damping_force;

            // If using CFM soft constraints, reduce stiffness slightly
            if (vehicle->use_cfm_suspension) {
                wheel->suspension_force *= (1.0f - vehicle->suspension_cfm);
            }

            wheel->contact_point = {world_connection.x, ground_y + wheel->radius, world_connection.z};
            wheel->contact_normal = {0.0f, 1.0f, 0.0f};
        } else {
            wheel->suspension_force = 0.0f;
            wheel->suspension_velocity = 0.0f;
            wheel->compression_previous = 0.0f;
            wheel->is_touching = 0;
        }
    }
}

/* World-aware suspension update: raycast per wheel to find contact with environment. */
void UpeVehicle_UpdateSuspensionWithWorld(UpeVehicle *vehicle, UpeBody *chassis, UpeWorld *world, float dt)
{
    if (!vehicle || !chassis) return;

    UpeQueryFilter filter = UpeQueryFilter_Default();

    for (uint32_t i = 0; i < vehicle->wheel_count; ++i) {
        UpeWheel *wheel = &vehicle->wheels[i];
        UpeVec3 world_connection = UpeVec3_Add(chassis->position, UpeQuat_RotateVec3(chassis->rotation, wheel->connection_point));

        // Raycast origin slightly above connection point
        UpeVec3 origin = world_connection;
        origin.y += 0.05f;
        UpeVec3 dir = {0.0f, -1.0f, 0.0f};
        float max_dist = wheel->suspension_length + wheel->radius + 0.1f;

        UpeRaycastResult hit = {};
        int hit_ok = 0;
        if (world) {
            hit_ok = UpeWorld_Raycast(world, origin, dir, max_dist, &filter, &hit);
        }

        float compression = 0.0f;
        if (hit_ok && hit.hit) {
            float hit_height = hit.position.y + wheel->radius;
            compression = fmaxf(0.0f, (origin.y - hit_height));
            compression = fminf(compression, wheel->suspension_length);

            wheel->is_touching = compression < wheel->suspension_length ? 1 : 0;
            wheel->contact_point = UpeVec3_Add(hit.position, UpeVec3_Scale(hit.normal, wheel->radius));
            wheel->contact_normal = hit.normal;
        } else {
            // Fallback plane at y=0
            float ground_y = 0.0f;
            compression = fmaxf(0.0f, (world_connection.y - wheel->radius) - ground_y);
            wheel->contact_point = {world_connection.x, ground_y + wheel->radius, world_connection.z};
            wheel->contact_normal = {0.0f, 1.0f, 0.0f};
            wheel->is_touching = compression < wheel->suspension_length ? 1 : 0;
        }

        if (wheel->is_touching && compression > 1e-4f) {
            // Suspension velocity/damping
            wheel->suspension_velocity = (compression - wheel->compression_previous) / fmaxf(dt, 0.001f);
            wheel->compression_previous = compression;

            float spring_force = -wheel->suspension_stiffness * compression;
            float damping_force = -wheel->suspension_damping * wheel->suspension_velocity;
            wheel->suspension_force = spring_force + damping_force;
            if (vehicle->use_cfm_suspension) wheel->suspension_force *= (1.0f - vehicle->suspension_cfm);
        } else {
            wheel->suspension_force = 0.0f;
            wheel->suspension_velocity = 0.0f;
            wheel->compression_previous = 0.0f;
            wheel->is_touching = 0;
        }
    }
}

void UpeVehicle_ApplyInput(UpeVehicle *vehicle, 
                          float steering_angle, float drive_power, float brake_power)
{
    if (!vehicle) {
        return;
    }

    for (uint32_t i = 0; i < vehicle->wheel_count; ++i) {
        UpeWheel *wheel = &vehicle->wheels[i];

        // Apply steering (front wheels only)
        if (i < 2) {
            float s = sinf(steering_angle);
            float c = cosf(steering_angle);
            wheel->steering_axis.x = s;
            wheel->steering_axis.z = c;
        }

        vehicle->drive_train_torque = drive_power;

        if (brake_power > 0.0f) {
            float damping = fmaxf(0.0f, 1.0f - brake_power * 0.08f);
            wheel->angular_velocity *= damping;
        }
    }
}

float UpeVehicle_GetWheelCompressionRatio(const UpeVehicle *vehicle, uint32_t wheel_idx)
{
    if (!vehicle || wheel_idx >= vehicle->wheel_count) {
        return 0.0f;
    }
    float max_compression = vehicle->wheels[wheel_idx].suspension_length;
    if (max_compression < 0.001f) return 0.0f;
    return fminf(1.0f, vehicle->wheels[wheel_idx].compression_previous / max_compression);
}

/*===========================================================================
  Tire Model - Simplified Pacejka "Magic Formula"
===========================================================================*/

/**
 * Calculate lateral tire force using simplified Pacejka model
 * 
 * @param slip_angle Slip angle in radians (positive = turning left)
 * @param load Normal load on tire in Newtons
 * @param friction Surface friction coefficient (0.0 - 1.0+)
 * @return Lateral force in Newtons
 */
float UpeVehicle_CalculatePacejkaLateral(float slip_angle, float load, float friction)
{
    if (load < 1.0f) return 0.0f;
    
    /* Simplified Pacejka coefficients for lateral force */
    const float B = 10.0f;   /* Stiffness factor */
    const float C = 1.9f;    /* Shape factor */
    const float D = 1.0f;    /* Peak factor */
    const float E = 0.97f;   /* Curvature factor */
    
    /* Magic formula: Y = D * sin(C * atan(B*x - E*(B*x - atan(B*x)))) */
    float x = slip_angle;
    float Bx = B * x;
    float atan_Bx = atanf(Bx);
    float arg = C * atanf(Bx - E * (Bx - atan_Bx));
    float normalized_force = D * sinf(arg);
    
    /* Scale by load and friction */
    float lateral_force = normalized_force * load * friction;
    
    return lateral_force;
}

/**
 * Calculate longitudinal tire force using simplified Pacejka model
 * 
 * @param slip_ratio Slip ratio (-1.0 = locked wheel, 0.0 = free rolling, 1.0 = spinning)
 * @param load Normal load on tire in Newtons
 * @param friction Surface friction coefficient
 * @return Longitudinal force in Newtons
 */
float UpeVehicle_CalculatePacejkaLongitudinal(float slip_ratio, float load, float friction)
{
    if (load < 1.0f) return 0.0f;
    
    /* Simplified Pacejka coefficients for longitudinal force */
    const float B = 11.0f;   /* Stiffness factor (slightly higher than lateral) */
    const float C = 1.9f;    /* Shape factor */
    const float D = 1.0f;    /* Peak factor */
    const float E = 0.97f;   /* Curvature factor */
    
    /* Magic formula */
    float x = slip_ratio;
    float Bx = B * x;
    float atan_Bx = atanf(Bx);
    float arg = C * atanf(Bx - E * (Bx - atan_Bx));
    float normalized_force = D * sinf(arg);
    
    /* Scale by load and friction */
    float longitudinal_force = normalized_force * load * friction;
    
    return longitudinal_force;
}

/**
 * Update tire slip angles, slip ratios, and calculate tire forces
 * 
 * @param vehicle Vehicle to update
 * @param chassis Chassis rigid body
 * @param dt Timestep in seconds
 */
void UpeVehicle_UpdateTireForces(UpeVehicle *vehicle, UpeBody *chassis, float dt)
{
    if (!vehicle || !chassis || dt < 0.0001f) return;
    
    for (uint32_t i = 0; i < vehicle->wheel_count; ++i) {
        UpeWheel *wheel = &vehicle->wheels[i];
        
        if (!wheel->is_touching) {
            wheel->slip_angle = 0.0f;
            wheel->slip_ratio = 0.0f;
            wheel->lateral_force = 0.0f;
            wheel->longitudinal_force = 0.0f;
            continue;
        }
        
        /* Chassis velocity at wheel contact point */
        UpeVec3 r = UpeVec3_Sub(wheel->contact_point, chassis->position);
        UpeVec3 contact_velocity = UpeVec3_Add(chassis->velocity, UpeVec3_Cross(chassis->angular_velocity, r));
        
        /* Calculate wheel forward and right directions (after steering) */
        UpeVec3 chassis_forward = UpeQuat_RotateVec3(chassis->rotation, UpeVec3{0.0f, 0.0f, 1.0f});
        UpeVec3 chassis_right = UpeQuat_RotateVec3(chassis->rotation, UpeVec3{1.0f, 0.0f, 0.0f});
        
        /* Apply steering angle to front wheels */
        float steer = wheel->steering_angle;
        float cos_steer = cosf(steer);
        float sin_steer = sinf(steer);
        
        UpeVec3 wheel_forward = UpeVec3{
            chassis_forward.x * cos_steer - chassis_right.x * sin_steer,
            0.0f,
            chassis_forward.z * cos_steer - chassis_right.z * sin_steer
        };
        
        UpeVec3 wheel_right = UpeVec3{
            chassis_forward.x * sin_steer + chassis_right.x * cos_steer,
            0.0f,
            chassis_forward.z * sin_steer + chassis_right.z * cos_steer
        };
        
        /* Normalize */
        float forward_len = sqrtf(wheel_forward.x * wheel_forward.x + wheel_forward.z * wheel_forward.z);
        if (forward_len > 0.001f) {
            wheel_forward.x /= forward_len;
            wheel_forward.z /= forward_len;
        }
        
        float right_len = sqrtf(wheel_right.x * wheel_right.x + wheel_right.z * wheel_right.z);
        if (right_len > 0.001f) {
            wheel_right.x /= right_len;
            wheel_right.z /= right_len;
        }
        
        /* Velocity components along wheel axes */
        float v_forward = contact_velocity.x * wheel_forward.x + contact_velocity.z * wheel_forward.z;
        float v_right = contact_velocity.x * wheel_right.x + contact_velocity.z * wheel_right.z;
        
        /* Slip angle: angle between velocity and wheel forward direction */
        if (fabsf(v_forward) > 0.1f) {
            wheel->slip_angle = atanf(v_right / fabsf(v_forward));
        } else {
            wheel->slip_angle = 0.0f;
        }
        
        /* Slip ratio: (wheel_speed - vehicle_speed) / vehicle_speed */
        float wheel_surface_speed = wheel->angular_velocity * wheel->radius;
        if (fabsf(v_forward) > 0.1f) {
            wheel->slip_ratio = (wheel_surface_speed - v_forward) / fabsf(v_forward);
            wheel->slip_ratio = fmaxf(-1.0f, fminf(1.0f, wheel->slip_ratio));
        } else {
            wheel->slip_ratio = 0.0f;
        }
        
        /* Calculate tire forces using Pacejka model */
        float normal_load = fabsf(wheel->suspension_force);
        float friction = vehicle->wheel_friction;
        
        wheel->lateral_force = UpeVehicle_CalculatePacejkaLateral(
            wheel->slip_angle,
            normal_load,
            friction
        );
        
        wheel->longitudinal_force = UpeVehicle_CalculatePacejkaLongitudinal(
            wheel->slip_ratio,
            normal_load,
            friction
        );
        
        /* Combined slip reduction (simplified - no full ellipse) */
        float combined_slip = sqrtf(wheel->slip_angle * wheel->slip_angle + 
                                     wheel->slip_ratio * wheel->slip_ratio);
        if (combined_slip > 0.3f) {
            float reduction = 0.3f / combined_slip;
            wheel->lateral_force *= reduction;
            wheel->longitudinal_force *= reduction;
        }
        
        float normal_force = -wheel->suspension_force;
        if (normal_force < 0.0f) {
            normal_force = 0.0f;
        }

        UpeVec3 force_lateral = UpeVec3_Scale(wheel_right, -wheel->lateral_force);
        UpeVec3 force_longitudinal = UpeVec3_Scale(wheel_forward, wheel->longitudinal_force);
        UpeVec3 force_suspension = UpeVec3_Scale(wheel->contact_normal, normal_force);
        UpeVec3 tire_force = UpeVec3_Add(UpeVec3_Add(force_lateral, force_longitudinal), force_suspension);

        UpeVehicle_ApplyForceAtPoint(chassis, tire_force, wheel->contact_point);

        float wheel_inertia = 0.5f * fmaxf(0.001f, chassis->mass / fmaxf(1.0f, (float)vehicle->wheel_count)) * wheel->radius * wheel->radius;
        float drive_torque = (i >= 2) ? vehicle->drive_train_torque : 0.0f;
        float traction_torque = wheel->longitudinal_force * wheel->radius;
        float angular_accel = (drive_torque - traction_torque) / wheel_inertia;
        wheel->angular_velocity += angular_accel * dt;
    }
}

/**
 * Apply Ackermann steering geometry for realistic turning
 * 
 * Inner wheel steers more than outer wheel to prevent tire scrub
 * 
 * @param vehicle Vehicle to update
 * @param steering_input Steering input (-1.0 = left, 1.0 = right)
 * @param wheelbase Distance from front to rear axle (meters)
 * @param track_width Distance between left and right wheels (meters)
 */
void UpeVehicle_ApplyAckermannSteering(UpeVehicle *vehicle, float steering_input, 
                                       float wheelbase, float track_width)
{
    if (!vehicle || wheelbase < 0.1f || track_width < 0.1f) return;
    
    /* Maximum steering angle (radians) */
    const float max_steer = 0.52f;  /* ~30 degrees */
    
    /* Base steering angle from input */
    float base_angle = steering_input * max_steer;
    
    if (fabsf(base_angle) < 0.001f) {
        /* Straight ahead */
        for (uint32_t i = 0; i < vehicle->wheel_count && i < 2; ++i) {
            vehicle->wheels[i].steering_angle = 0.0f;
        }
        return;
    }
    
    /* Ackermann geometry:
     *   tan(outer_angle) = L / (R + T/2)
     *   tan(inner_angle) = L / (R - T/2)
     * where L = wheelbase, R = turning radius, T = track width
     */
    
    /* Calculate turning radius from base angle */
    float turn_radius = wheelbase / tanf(fabsf(base_angle));
    
    /* Calculate inner and outer wheel angles */
    float inner_angle = atanf(wheelbase / (turn_radius - track_width * 0.5f));
    float outer_angle = atanf(wheelbase / (turn_radius + track_width * 0.5f));
    
    /* Determine which wheel is inner vs outer based on steering direction */
    if (steering_input < 0.0f) {
        /* Turning left: left wheel is inner */
        vehicle->wheels[0].steering_angle = -inner_angle;  /* Left front */
        vehicle->wheels[1].steering_angle = -outer_angle;  /* Right front */
    } else {
        /* Turning right: right wheel is inner */
        vehicle->wheels[0].steering_angle = outer_angle;   /* Left front */
        vehicle->wheels[1].steering_angle = inner_angle;   /* Right front */
    }
}
