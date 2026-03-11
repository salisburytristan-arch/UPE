#include "upe/upe_advanced_vehicle.h"
#include "upe/upe_body.h"
#include "upe/upe_math.h"
#include <math.h>
#include <string.h>

// ============================================================================
// Transmission System
// ============================================================================

void UpeTransmission_Init(UpeTransmission *trans, UpeTransmissionType type) {
    trans->type = type;
    trans->engine_power = 200000.0f;     // 200 kW typical car
    trans->engine_torque = 400.0f;       // 400 N⋅m peak torque
    trans->engine_rpm = 0.0f;
    trans->engine_max_rpm = 8000.0f;
    trans->wheel_rpm = 0.0f;
    trans->clutch_engagement = 1.0f;
    trans->torque_output = 0.0f;

    // Standard gear ratios for front-wheel drive car
    float default_ratios[6] = {3.5f, 2.2f, 1.5f, 1.1f, 0.85f, 0.7f};
    memcpy(trans->manual.gear_ratios, default_ratios, sizeof(default_ratios));
    trans->manual.reverse_ratio = -3.5f;
    trans->manual.final_drive_ratio = 4.0f;
    trans->manual.current_gear = 0;

    // Auto transmission
    memcpy(trans->automatic.gear_ratios, default_ratios, sizeof(default_ratios));
    trans->automatic.reverse_ratio = -3.5f;
    trans->automatic.final_drive_ratio = 4.0f;
    trans->automatic.current_gear = 0;
    trans->automatic.shift_up_rpm = 7000.0f;
    trans->automatic.shift_down_rpm = 2500.0f;
    trans->automatic.shift_time = 0.2f;
    trans->automatic.shift_timer = 0.0f;
    trans->automatic.is_shifting = 0;
}

void UpeTransmission_SetGear(UpeTransmission *trans, int gear) {
    if (trans->type == UPE_TRANSMISSION_MANUAL) {
        if (gear >= -1 && gear <= 6) {
            trans->manual.current_gear = gear;
        }
    }
}

void UpeTransmission_SetThrottle(UpeTransmission *trans, float throttle) {
    // Clamp throttle
    if (throttle < -1.0f) throttle = -1.0f;
    if (throttle > 1.0f) throttle = 1.0f;
    
    // RPM increases with throttle (simplified engine model)
    float rpm_change = throttle * 4000.0f;  // Max 4000 RPM acceleration
    trans->engine_rpm += rpm_change * 0.016f;  // 16ms timestep typical
    
    if (trans->engine_rpm < 0.0f) trans->engine_rpm = 0.0f;
    if (trans->engine_rpm > trans->engine_max_rpm) trans->engine_rpm = trans->engine_max_rpm;
}

float UpeTransmission_GetTorqueOutput(UpeTransmission *trans) {
    int gear = trans->type == UPE_TRANSMISSION_AUTOMATIC ? 
               trans->automatic.current_gear : trans->manual.current_gear;
    
    if (gear == 0) return 0.0f;  // Neutral
    
    // Gear ratio multiplier
    float gear_ratio = (gear > 0) ? 
                       (trans->type == UPE_TRANSMISSION_AUTOMATIC ? 
                        trans->automatic.gear_ratios[gear-1] :
                        trans->manual.gear_ratios[gear-1]) :
                       (trans->type == UPE_TRANSMISSION_AUTOMATIC ?
                        trans->automatic.reverse_ratio :
                        trans->manual.reverse_ratio);
    
    float final_ratio = trans->type == UPE_TRANSMISSION_AUTOMATIC ?
                        trans->automatic.final_drive_ratio :
                        trans->manual.final_drive_ratio;
    
    // Torque from engine RPM (normalized bell curve)
    float rpm_ratio = trans->engine_rpm / trans->engine_max_rpm;
    float torque_multiplier = 4.0f * rpm_ratio * (1.0f - rpm_ratio);  // Bell curve
    torque_multiplier = torque_multiplier < 0.0f ? 0.0f : torque_multiplier;
    
    float engine_torque = trans->engine_torque * torque_multiplier;
    
    // Apply transmission multiplication
    float output_torque = engine_torque * gear_ratio * final_ratio * trans->clutch_engagement;
    
    trans->torque_output = output_torque;
    return output_torque;
}

// ============================================================================
// Tire Model - Pacejka Magic Formula (Simplified)
// ============================================================================

float UpeTireModel_CalculateGripForce(UpeTireModel *tire, float load, float slip_ratio) {
    // Clamp slip ratio
    if (slip_ratio < 0.0f) slip_ratio = 0.0f;
    if (slip_ratio > 2.0f) slip_ratio = 2.0f;
    
    // Pacejka magic formula: F(slip) = D * sin(C * arctan(B * slip))
    float B = 10.0f;  // Stiffness factor
    float C = 1.9f;   // Shape factor
    float D = load * tire->longitudinal_grip;  // Peak force
    
    float slip_rad = atanf(B * slip_ratio);
    float grip_force = D * sinf(C * slip_rad);
    
    // Temperature effect (grip increases when warm, decreases if overheated)
    float temp_factor = 1.0f;
    if (tire->temperature < 60.0f) {
        temp_factor = 0.5f + (tire->temperature / 60.0f) * 0.5f;
    } else if (tire->temperature > 120.0f) {
        temp_factor = 1.5f - (tire->temperature - 120.0f) / 60.0f * 0.3f;
    }
    
    grip_force *= temp_factor;
    grip_force *= tire->wear;  // Worn tires have less grip
    
    return grip_force;
}

void UpeTireModel_UpdateSlip(UpeTireModel *tire, float wheel_velocity, float chassis_velocity, float steering) {
    // Longitudinal slip ratio
    float vel_diff = fabsf(wheel_velocity - chassis_velocity);
    float ref_vel = fabsf(chassis_velocity) > 0.1f ? fabsf(chassis_velocity) : 0.1f;
    tire->longitudinal_slip = vel_diff / ref_vel;
    
    // Lateral slip (simplified)
    tire->lateral_slip = steering * 15.0f;  // Convert steering to approximate slip angle
    
    // Combined slip affects slide factor
    float combined_slip = sqrtf(tire->longitudinal_slip * tire->longitudinal_slip + 
                                (tire->lateral_slip / 30.0f) * (tire->lateral_slip / 30.0f));
    
    if (combined_slip > tire->grip_threshold) {
        tire->slide_factor = (combined_slip - tire->grip_threshold) / 
                             (2.0f - tire->grip_threshold);
    } else {
        tire->slide_factor = 0.0f;
    }
    
    if (tire->slide_factor > 1.0f) tire->slide_factor = 1.0f;
    
    // Update tire temperature (simplified)
    float heat_generation = tire->slide_factor * combined_slip;
    tire->temperature += heat_generation * 5.0f;  // Heat generation
    tire->temperature *= 0.98f;  // Cooling
}

// ============================================================================
// Advanced Vehicle System
// ============================================================================

void UpeAdvancedVehicle_Init(UpeAdvancedVehicle *vehicle, uint32_t chassis_body, uint32_t wheel_count) {
    vehicle->chassis_body = chassis_body;
    vehicle->wheel_count = wheel_count <= 4 ? wheel_count : 4;
    
    // Initialize wheels
    for (uint32_t i = 0; i < vehicle->wheel_count; ++i) {
        vehicle->wheels[i].body_id = 0;
        vehicle->wheels[i].radius = 0.35f;
        vehicle->wheels[i].load = 0.0f;
        vehicle->wheels[i].drive_torque = 0.0f;
        vehicle->wheels[i].brake_torque = 0.0f;
        vehicle->wheels[i].steering_angle = 0.0f;
        vehicle->wheels[i].rotational_velocity = 0.0f;
        vehicle->wheels[i].suspension_compression = 0.0f;
        vehicle->wheels[i].is_grounded = 0;
        
        // Initialize tire
        vehicle->wheels[i].tire.longitudinal_grip = 1.2f;
        vehicle->wheels[i].tire.lateral_grip = 1.1f;
        vehicle->wheels[i].tire.grip_threshold = 0.1f;
        vehicle->wheels[i].tire.temperature = 20.0f;
        vehicle->wheels[i].tire.wear = 1.0f;
        vehicle->wheels[i].tire.slide_factor = 0.0f;
    }
    
    // Initialize transmission
    UpeTransmission_Init(&vehicle->transmission, UPE_TRANSMISSION_AUTOMATIC);
    
    // Input state
    vehicle->throttle_input = 0.0f;
    vehicle->brake_input = 0.0f;
    vehicle->steering_input = 0.0f;
    vehicle->handbrake_input = 0.0f;
    
    // Physics parameters
    vehicle->max_steering_angle = 0.4f;  // ~23 degrees
    vehicle->suspension_stiffness = 25000.0f;
    vehicle->suspension_damping = 2500.0f;
    vehicle->drag_coefficient = 0.25f;  // Typical car
    vehicle->downforce_coefficient = 0.05f;
    
    // Stability control
    vehicle->traction_control_slip = 0.3f;
    vehicle->traction_control_enabled = 1;
    vehicle->abs_enabled = 1;
    vehicle->abs_threshold = 0.5f;
    
    // Performance
    vehicle->top_speed = 50.0f;  // m/s (~180 km/h)
    vehicle->acceleration = 0.0f;
    vehicle->drift_angle = 0.0f;
    vehicle->drift_score = 0.0f;
}

void UpeAdvancedVehicle_SetInput(UpeAdvancedVehicle *vehicle, float throttle, float steering, float brake, float handbrake) {
    vehicle->throttle_input = (throttle < -1.0f) ? -1.0f : (throttle > 1.0f ? 1.0f : throttle);
    vehicle->steering_input = (steering < -1.0f) ? -1.0f : (steering > 1.0f ? 1.0f : steering);
    vehicle->brake_input = (brake < 0.0f) ? 0.0f : (brake > 1.0f ? 1.0f : brake);
    vehicle->handbrake_input = (handbrake < 0.0f) ? 0.0f : (handbrake > 1.0f ? 1.0f : handbrake);
}

void UpeAdvancedVehicle_Update(UpeAdvancedVehicle *vehicle, UpeBody *chassis, float dt) {
    if (!chassis) return;
    
    // Update transmission based on throttle
    UpeTransmission_SetThrottle(&vehicle->transmission, vehicle->throttle_input);
    
    // Auto transmission shifting
    if (vehicle->transmission.type == UPE_TRANSMISSION_AUTOMATIC) {
        UpeAutomaticTransmission *auto_trans = &vehicle->transmission.automatic;
        
        if (!auto_trans->is_shifting) {
            if (auto_trans->current_gear > 0 && 
                vehicle->transmission.engine_rpm > auto_trans->shift_up_rpm) {
                auto_trans->current_gear++;
                if (auto_trans->current_gear > 6) auto_trans->current_gear = 6;
                auto_trans->is_shifting = 1;
                auto_trans->shift_timer = 0.0f;
            } else if (auto_trans->current_gear > 1 && 
                       vehicle->transmission.engine_rpm < auto_trans->shift_down_rpm) {
                auto_trans->current_gear--;
                auto_trans->is_shifting = 1;
                auto_trans->shift_timer = 0.0f;
            } else if (auto_trans->current_gear == 0 && vehicle->throttle_input > 0.1f) {
                auto_trans->current_gear = 1;  // Start in gear 1
            }
        }
        
        // Update shift timer
        if (auto_trans->is_shifting) {
            auto_trans->shift_timer += dt;
            if (auto_trans->shift_timer >= auto_trans->shift_time) {
                auto_trans->is_shifting = 0;
                auto_trans->shift_timer = 0.0f;
            }
            vehicle->transmission.clutch_engagement = 1.0f - (auto_trans->shift_timer / auto_trans->shift_time) * 0.5f;
        } else {
            vehicle->transmission.clutch_engagement = 1.0f;
        }
    }
    
    // Get torque output from transmission
    float torque_output = UpeTransmission_GetTorqueOutput(&vehicle->transmission);
    
    // Calculate wheel forces
    float steering_angle = vehicle->steering_input * vehicle->max_steering_angle;
    
    for (uint32_t i = 0; i < vehicle->wheel_count; ++i) {
        UpeAdvancedWheel *wheel = &vehicle->wheels[i];
        
        // Estimate wheel load from chassis mass and gravity
        float chassis_mass = chassis->mass > 0.0f ? chassis->mass : 1000.0f;
        wheel->load = (chassis_mass * 9.81f) / vehicle->wheel_count;
        
        // Apply downforce based on speed
        float speed = UpeVec3_Length(chassis->velocity);
        float downforce = speed * speed * vehicle->downforce_coefficient * vehicle->wheel_count;
        wheel->load += downforce;
        
        // Drive torque (only front wheels on typical FWD)
        if (i < 2) {
            wheel->drive_torque = torque_output / vehicle->wheels[i].radius;
        } else {
            wheel->drive_torque = 0.0f;
        }
        
        // Braking torque
        float brake_force = vehicle->brake_input * 5000.0f + vehicle->handbrake_input * 3000.0f;
        wheel->brake_torque = brake_force * wheel->radius;
        
        // Steering angle (front wheels only)
        wheel->steering_angle = (i < 2) ? steering_angle : 0.0f;
        
        // Update tire slip
        UpeTireModel_UpdateSlip(&wheel->tire, wheel->rotational_velocity, speed, steering_angle);
        
        // Apply traction control
        if (vehicle->traction_control_enabled && wheel->tire.slide_factor > vehicle->traction_control_slip) {
            wheel->drive_torque *= (1.0f - wheel->tire.slide_factor);
        }
    }
    
    // Apply ABS
    if (vehicle->abs_enabled) {
        UpeAdvancedVehicle_ApplyABS(vehicle);
    }
    
    // Apply aerodynamic drag
    float speed = UpeVec3_Length(chassis->velocity);
    if (speed > 0.1f) {
        UpeVec3 drag_force = UpeVec3_Scale(chassis->velocity, -0.5f * vehicle->drag_coefficient * speed);
        UpeBody_ApplyForce(chassis, drag_force);
    }
    
    // Calculate drift angle
    vehicle->drift_angle = UpeAdvancedVehicle_GetDriftAngle(vehicle);
    
    // Update drift score
    if (fabsf(vehicle->drift_angle) > 10.0f) {  // Only count significant drifts
        vehicle->drift_score += fabsf(vehicle->drift_angle) * 0.01f * dt;
    } else {
        vehicle->drift_score *= 0.95f;  // Decay when not drifting
    }
}

void UpeAdvancedVehicle_ApplyABS(UpeAdvancedVehicle *vehicle) {
    // Anti-lock braking system: prevent wheel lockup
    for (uint32_t i = 0; i < vehicle->wheel_count; ++i) {
        UpeAdvancedWheel *wheel = &vehicle->wheels[i];
        
        if (wheel->is_grounded && wheel->brake_torque > 0.0f) {
            // Check if wheel is locking up (wheel slip too high)
            if (fabsf(wheel->tire.longitudinal_slip) > vehicle->abs_threshold) {
                // Reduce braking force
                wheel->brake_torque *= 0.7f;
            }
        }
    }
}

float UpeAdvancedVehicle_GetDriftAngle(UpeAdvancedVehicle *vehicle) {
    // Drift angle = angle between velocity direction and heading direction
    // 0° = no drift, 90° = full sideways drift
    float slip = vehicle->wheels[0].tire.longitudinal_slip;
    float speed = slip > 0.0f ? 1.0f : 0.0f;
    
    return speed > 0.5f ? 10.0f : 0.0f;  // Simplified
}

float UpeAdvancedVehicle_GetDriftScore(UpeAdvancedVehicle *vehicle) {
    return vehicle->drift_score;
}

void UpeAdvancedVehicle_ApplyTractionControl(UpeAdvancedVehicle *vehicle, float dt) {
    (void)dt;
    // Traction control reduces power to wheels that are slipping
    for (uint32_t i = 0; i < vehicle->wheel_count; ++i) {
        if (vehicle->wheels[i].tire.slide_factor > vehicle->traction_control_slip) {
            vehicle->wheels[i].drive_torque *= (1.0f - vehicle->wheels[i].tire.slide_factor * 0.5f);
        }
    }
}

float UpeVehicle_DifferentialSlipRatio(float wheel_speed_L, float wheel_speed_R, float v_chassis) {
    float w_avg = 0.5f * (wheel_speed_L + wheel_speed_R);
    float denom = fabsf(v_chassis);
    if (denom < 0.1f) denom = 0.1f;
    return (w_avg - v_chassis) / denom;
}

float UpeVehicle_TurbolagBoostPressure(float throttle, float boost_setpoint,
                                        float tau_turbo, float dt) {
    if (tau_turbo <= 1e-4f) return throttle * boost_setpoint;
    /* First-order lag response to commanded boost */
    float alpha = 1.0f - expf(-dt / tau_turbo);
    static float boost_state = 0.0f;
    float target = throttle * boost_setpoint;
    boost_state += (target - boost_state) * alpha;
    return boost_state;
}

float UpeVehicle_ClutchSlipTorque(float engine_torque, float slip_ratio,
                                   float clutch_capacity) {
    float slip = fabsf(slip_ratio);
    float engagement = 1.0f - (slip > 1.0f ? 1.0f : slip);
    float transferable = clutch_capacity * engagement;
    if (transferable < 0.0f) transferable = 0.0f;
    if (transferable > fabsf(engine_torque)) transferable = fabsf(engine_torque);
    return (engine_torque >= 0.0f) ? transferable : -transferable;
}

float UpeVehicle_BrakeFadeCoeff(float base_Crr, float brake_temp_C) {
    if (brake_temp_C <= 200.0f) return base_Crr;
    float fade = 1.0f + 0.0025f * (brake_temp_C - 200.0f);
    if (fade > 3.0f) fade = 3.0f;
    return base_Crr * fade;
}

float UpeVehicle_HydroplaningSpeed(float tire_width_m, float water_depth_m,
                                    float tire_pressure_Pa) {
    (void)tire_width_m;
    /* Baseline NASA relation in m/s from pressure in psi: V(knots)=10.35*sqrt(P_psi) */
    float p_psi = tire_pressure_Pa / 6894.757f;
    if (p_psi < 1.0f) p_psi = 1.0f;
    float v_knots = 10.35f * sqrtf(p_psi);
    float v_ms = v_knots * 0.514444f;
    /* Deeper water lowers onset speed */
    float depth_factor = 1.0f - 0.35f * (water_depth_m > 0.02f ? 1.0f : water_depth_m / 0.02f);
    if (depth_factor < 0.5f) depth_factor = 0.5f;
    return v_ms * depth_factor;
}
