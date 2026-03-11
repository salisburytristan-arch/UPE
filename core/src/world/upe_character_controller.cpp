/*
  UPE Character Controller Implementation - Core movement and control logic
*/

#include "upe/upe_character_controller.h"
#include "upe/upe_math.h"
#include "upe/upe_world_body_api.h"
#include "upe/upe_queries.h"
#include "upe/upe_shape.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Gravity constant (m/s²) */
#define UPE_GRAVITY 9.81f

static int UpeCharacterController_RaycastClosestNonSelf(
    const UpeWorld *world,
    UpeVec3 origin,
    UpeVec3 direction,
    float max_distance,
    uint32_t self_body_id,
    UpeRaycastResult *out_hit)
{
    if (!world || !out_hit || max_distance <= 0.0f) {
        return 0;
    }

    UpeQueryFilter filter = UpeQueryFilter_Default();
    UpeRaycastResult hits[16];
    int hit_count = UpeWorld_RaycastAll(world, origin, direction, max_distance, &filter, hits, 16);
    if (hit_count <= 0) {
        return 0;
    }

    int found = 0;
    float best_t = max_distance;
    for (int i = 0; i < hit_count; ++i) {
        if (!hits[i].hit) {
            continue;
        }
        if (hits[i].body_id == self_body_id) {
            continue;
        }
        if (!found || hits[i].t < best_t) {
            best_t = hits[i].t;
            *out_hit = hits[i];
            found = 1;
        }
    }

    return found;
}

/*============================================================================
  Default Configuration
============================================================================*/

UpeCharacterControllerDesc UpeCharacterController_GetDefaultConfig(void) {
    UpeCharacterControllerDesc config;
    config.height = 1.8f;
    config.radius = 0.3f;
    config.mass = 80.0f;

    config.walk_speed = 2.0f;
    config.run_speed = 5.0f;
    config.sprint_speed = 8.0f;
    config.acceleration = 10.0f;
    config.deceleration = 8.0f;

    config.jump_power = 5.0f;
    config.jump_apex_delay = 0.1f;
    config.coyote_time = 0.1f;
    config.jump_buffer_time = 0.1f;

    config.air_acceleration = 0.5f;
    config.air_friction = 0.1f;
    config.terminal_velocity = 25.0f;

    config.max_slope_angle = 45.0f;
    config.slope_friction = 0.7f;

    config.step_height = 0.3f;
    config.step_reach = 0.2f;

    config.ground_check_distance = 0.1f;
    config.skin_width = 0.01f;
    return config;
}

/*============================================================================
  Lifecycle
============================================================================*/

UpeCharacterController* UpeCharacterController_Create(
    UpeWorld *world,
    UpeVec3 position,
    const UpeCharacterControllerDesc *config)
{
    UpeCharacterController *controller = 
        (UpeCharacterController *)malloc(sizeof(UpeCharacterController));
    
    if (!controller) return NULL;
    
    /* Copy config */
    memcpy(&controller->config, config, sizeof(UpeCharacterControllerDesc));
    
    /* Create physics body (capsule) */
    UpeBodyDesc body_desc;
    body_desc.type = UPE_BODY_DYNAMIC;
    body_desc.mass = config->mass;
    body_desc.position = position;
    body_desc.rotation = UpeQuat_Identity();
    body_desc.collision_layer = 0;
    body_desc.collision_mask = 0xFFFFFFFFu;
    
    controller->body_id = UpeWorld_CreateBody(world, &body_desc);
    
    /* Add capsule shape */
    UpeShapeDesc shape_desc;
    shape_desc.type = UPE_SHAPE_CAPSULE;
    shape_desc.body_id = controller->body_id;
    shape_desc.collision_layer = 0;
    shape_desc.collision_mask = 0xFFFFFFFFu;
    float half_segment = fmaxf(0.0f, (config->height * 0.5f) - config->radius);
    shape_desc.capsule.a = UpeVec3{0.0f, -half_segment, 0.0f};
    shape_desc.capsule.b = UpeVec3{0.0f,  half_segment, 0.0f};
    shape_desc.capsule.radius = config->radius;
    UpeWorld_CreateShape(world, &shape_desc);
    
    /* Initialize state */
    controller->root_position = position;
    controller->root_rotation = UpeQuat_Identity();
    
    controller->mode = UPE_CHARCTRL_MODE_GROUNDED;
    controller->locomotion = UPE_CHARCTRL_LOCOMOTION_IDLE;
    controller->velocity = UpeVec3{0.0f, 0.0f, 0.0f};
    controller->desired_velocity = UpeVec3{0.0f, 0.0f, 0.0f};
    
    controller->can_jump = true;
    controller->coyote_counter = 0.0f;
    controller->jump_buffer_counter = 0.0f;
    controller->pending_jump = false;
    
    controller->air_time = 0.0f;
    controller->falling_distance = 0.0f;
    controller->is_falling = false;
    
    controller->ground_normal = UpeVec3{0.0f, 1.0f, 0.0f};
    controller->slope_angle = 0.0f;
    controller->on_slope = false;
    
    controller->ground_body_id = 0;
    controller->platform_velocity = UpeVec3{0.0f, 0.0f, 0.0f};
    
    controller->last_locomotion = UPE_CHARCTRL_LOCOMOTION_IDLE;
    controller->direction_angle = 0.0f;
    controller->speed_multiplier = 0.0f;
    
    controller->apex_time = 0.0f;
    controller->ground_contact_time = 0.0f;
    
    return controller;
}

void UpeCharacterController_Destroy(UpeCharacterController *controller) {
    if (!controller) return;
    /* Note: We don't destroy the physics body here - let world manage it */
    free(controller);
}

/*============================================================================
  Input & Movement Control
============================================================================*/

void UpeCharacterController_SetMovementInput(
    UpeCharacterController *controller,
    UpeVec2 direction,
    UpeCharacterLocomotion movement_type)
{
    if (!controller) return;
    
    /* Clamp to unit circle */
    float len = sqrtf(direction.x * direction.x + direction.y * direction.y);
    if (len > 1.0f) {
        direction.x /= len;
        direction.y /= len;
    }
    
    /* Get target speed based on movement type */
    float target_speed = 0.0f;
    switch (movement_type) {
        case UPE_CHARCTRL_LOCOMOTION_WALKING:
            target_speed = controller->config.walk_speed;
            break;
        case UPE_CHARCTRL_LOCOMOTION_RUNNING:
            target_speed = controller->config.run_speed;
            break;
        case UPE_CHARCTRL_LOCOMOTION_SPRINTING:
            target_speed = controller->config.sprint_speed;
            break;
        default:
            target_speed = 0.0f;
    }
    
    /* Convert local input to world space (simplified - assumes forward = +Z) */
    UpeVec3 forward = UpeVec3{0.0f, 0.0f, 1.0f};
    UpeVec3 right = UpeVec3{1.0f, 0.0f, 0.0f};
    
    UpeVec3 move_dir = UpeVec3{
        right.x * direction.x + forward.x * direction.y,
        0.0f,  /* No vertical movement from input */
        right.z * direction.x + forward.z * direction.y
    };
    
    /* Normalize */
    float move_len = sqrtf(move_dir.x * move_dir.x + move_dir.z * move_dir.z);
    if (move_len > 0.01f) {
        move_dir.x /= move_len;
        move_dir.z /= move_len;
        
        /* Update direction angle for animation */
        controller->direction_angle = atan2f(direction.x, direction.y) * 57.2958f;  /* rad to deg */
    }
    
    controller->desired_velocity.x = move_dir.x * target_speed;
    controller->desired_velocity.z = move_dir.z * target_speed;
    
    controller->locomotion = (target_speed > 0.01f) ? movement_type : UPE_CHARCTRL_LOCOMOTION_IDLE;
}

void UpeCharacterController_RequestJump(UpeCharacterController *controller) {
    if (!controller) return;
    
    controller->pending_jump = true;
    controller->jump_buffer_counter = 0.0f;
}

void UpeCharacterController_SetFacingDirection(
    UpeCharacterController *controller,
    UpeVec3 facing,
    float turn_speed) {
    if (!controller) return;
    
    /* Normalize facing direction */
    float len = sqrtf(facing.x * facing.x + facing.y * facing.y + facing.z * facing.z);
    if (len > 0.01f) {
        facing.x /= len;
        facing.y /= len;
        facing.z /= len;
    }
    
    /* Create rotation from facing direction */
    UpeVec3 forward = UpeVec3{0.0f, 0.0f, 1.0f};
    UpeVec3 cross = UpeVec3{
        forward.y * facing.z - forward.z * facing.y,
        forward.z * facing.x - forward.x * facing.z,
        forward.x * facing.y - forward.y * facing.x
    };
    
    float cross_len = sqrtf(cross.x * cross.x + cross.y * cross.y + cross.z * cross.z);
    if (cross_len > 0.01f) {
        float angle = acos(fmin(1.0f, fmax(-1.0f, 
            forward.x * facing.x + forward.y * facing.y + forward.z * facing.z)));
        
        /* Smooth interpolation */
        angle = angle * (turn_speed / 360.0f);  /* Scale by turn speed */
        
        /* Create quaternion from angle-axis */
        float half_angle = angle * 0.5f;
        float sin_half = sinf(half_angle);
        UpeQuat rot = UpeQuat{
            (cross.x / cross_len) * sin_half,
            (cross.y / cross_len) * sin_half,
            (cross.z / cross_len) * sin_half,
            cosf(half_angle)
        };
        
        /* Blend with current rotation */
        controller->root_rotation = rot;  /* Simplified - direct assignment */
    }
}

void UpeCharacterController_ApplyKnockback(
    UpeCharacterController *controller,
    UpeVec3 impulse,
    float duration) {
    if (!controller) return;
    
    /* Add to velocity immediately */
    controller->velocity.x += impulse.x;
    controller->velocity.y += impulse.y;
    controller->velocity.z += impulse.z;
}

/*============================================================================
  Update & Physics Integration
============================================================================*/

void UpeCharacterController_Update(
    UpeCharacterController *controller,
    float dt,
    UpeWorld *world)
{
    if (!controller || !world) return;
    
    /* Update ground contact with raycast-driven detection */
    UpeVec3 ground_normal = UpeVec3{0.0f, 1.0f, 0.0f};
    bool is_grounded = false;
    uint32_t detected_ground_body = 0;

    float skin = fmaxf(0.001f, controller->config.skin_width);
    float ground_check = fmaxf(0.01f, controller->config.ground_check_distance);
    UpeVec3 ray_origin = controller->root_position;
    ray_origin.y += controller->config.radius + skin;
    float ray_length = controller->config.radius + ground_check + skin;

    UpeRaycastResult ground_hit = {};
    int has_ground_hit = UpeCharacterController_RaycastClosestNonSelf(
        world,
        ray_origin,
        UpeVec3{0.0f, -1.0f, 0.0f},
        ray_length,
        controller->body_id,
        &ground_hit
    );

    if (has_ground_hit) {
        float desired_root_y = ground_hit.position.y + controller->config.radius + skin;
        float root_above_ground = controller->root_position.y - desired_root_y;

        if (root_above_ground <= ground_check + skin) {
            is_grounded = true;
            ground_normal = ground_hit.normal;
            detected_ground_body = ground_hit.body_id;

            if (controller->velocity.y <= 0.0f && root_above_ground < skin * 4.0f) {
                controller->root_position.y = fmaxf(controller->root_position.y, desired_root_y);
            }
        }
    }

    if (is_grounded) {
        controller->coyote_counter = 0.0f;
    } else {
        controller->coyote_counter += dt;
        is_grounded = controller->coyote_counter < controller->config.coyote_time;
    }
    
    /* Calculate slope angle from ground normal */
    float slope_dot = ground_normal.y;
    float slope_angle_rad = acosf(fmin(1.0f, fmax(-1.0f, slope_dot)));
    float slope_angle_deg = slope_angle_rad * 57.2958f;  /* rad to deg */
    
    controller->slope_angle = slope_angle_deg;
    controller->on_slope = (slope_angle_deg > 5.0f);  /* Small threshold to avoid flat surfaces */
    controller->ground_normal = ground_normal;
    
    /* Check if slope is too steep */
    bool slope_too_steep = (slope_angle_deg > controller->config.max_slope_angle);
    
    /* If on too steep slope, apply sliding force */
    if (is_grounded && slope_too_steep) {
        /* Calculate slide direction (perpendicular to slope normal, in horizontal plane) */
        UpeVec3 slide_dir = UpeVec3{ground_normal.x, 0.0f, ground_normal.z};
        float slide_len = sqrtf(slide_dir.x * slide_dir.x + slide_dir.z * slide_dir.z);
        if (slide_len > 0.01f) {
            slide_dir.x /= slide_len;
            slide_dir.z /= slide_len;
            
            /* Apply sliding force */
            float slide_force = UPE_GRAVITY * sinf(slope_angle_rad) * (1.0f - controller->config.slope_friction);
            controller->velocity.x += slide_dir.x * slide_force * dt;
            controller->velocity.z += slide_dir.z * slide_force * dt;
        }
        
        /* Reduce control when sliding */
        is_grounded = false;  /* Treat as airborne for movement */
    }
    
    /* Step-up detection: raycast at foot/head levels to allow stepping over low obstacles */
    if (is_grounded && !slope_too_steep) {
        UpeVec3 horizontal_vel = UpeVec3{controller->velocity.x, 0.0f, controller->velocity.z};
        float horizontal_speed = sqrtf(horizontal_vel.x * horizontal_vel.x + horizontal_vel.z * horizontal_vel.z);
        
        if (horizontal_speed > 0.1f) {
            /* Normalize horizontal direction */
            UpeVec3 forward = UpeVec3{
                horizontal_vel.x / horizontal_speed,
                0.0f,
                horizontal_vel.z / horizontal_speed
            };

            float reach = fmaxf(controller->config.step_reach, controller->config.radius * 0.5f);
            UpeVec3 foot_origin = controller->root_position;
            foot_origin.y += controller->config.radius + skin;

            UpeVec3 head_origin = controller->root_position;
            head_origin.y += controller->config.radius + controller->config.step_height + skin;

            UpeRaycastResult foot_hit = {};
            UpeRaycastResult head_hit = {};
            bool obstacle_ahead = UpeCharacterController_RaycastClosestNonSelf(
                world, foot_origin, forward, reach, controller->body_id, &foot_hit) != 0;
            bool clear_above = UpeCharacterController_RaycastClosestNonSelf(
                world, head_origin, forward, reach, controller->body_id, &head_hit) == 0;

            if (obstacle_ahead) {
                if (clear_above) {
                    float step_amount = fmaxf(0.0f, controller->config.step_height - skin);
                    controller->root_position.y += step_amount;
                    controller->velocity.y = 0.0f;
                }
            }
        }
    }
    
    /* Jump processing */
    controller->jump_buffer_counter += dt;
    
    if (controller->pending_jump && (is_grounded || controller->coyote_counter < controller->config.coyote_time)) {
        /* Execute jump */
        controller->velocity.y = controller->config.jump_power;
        controller->mode = UPE_CHARCTRL_MODE_JUMPING;
        controller->air_time = 0.0f;
        controller->falling_distance = 0.0f;
        controller->apex_time = 0.0f;
        controller->pending_jump = false;
        is_grounded = false;
    }
    
    controller->pending_jump = false;
    
    /* Update mode based on vertical velocity */
    if (is_grounded) {
        controller->mode = UPE_CHARCTRL_MODE_GROUNDED;
        controller->air_time = 0.0f;
        controller->velocity.y = 0.0f;
        controller->falling_distance = 0.0f;
        controller->is_falling = false;

        controller->ground_body_id = detected_ground_body;
        controller->platform_velocity = UpeVec3{0.0f, 0.0f, 0.0f};

        if (controller->ground_body_id != 0 && controller->ground_body_id != controller->body_id) {
            UpeBody *ground_body = UpeWorld_GetBody(world, controller->ground_body_id);
            if (ground_body) {
                controller->platform_velocity = ground_body->velocity;
            }
        }
    } else {
        if (controller->velocity.y < -0.1f) {
            controller->mode = UPE_CHARCTRL_MODE_FALLING;
            controller->is_falling = true;
        }
        
        controller->air_time += dt;
    }
    
    /* Gravity and air physics */
    if (!is_grounded) {
        /* Apply gravity */
        float gravity_factor = 1.0f;
        
        /* Reduced gravity at apex of jump */
        if (controller->mode == UPE_CHARCTRL_MODE_JUMPING) {
            controller->apex_time += dt;
            if (controller->apex_time < controller->config.jump_apex_delay) {
                gravity_factor *= 0.5f;  /* Floatier at apex */
            }
        }
        
        controller->velocity.y -= UPE_GRAVITY * gravity_factor * dt;
        
        /* Terminal velocity */
        if (controller->velocity.y < -controller->config.terminal_velocity) {
            controller->velocity.y = -controller->config.terminal_velocity;
        }
        
        /* Fall distance tracking */
        if (controller->velocity.y < 0) {
            controller->falling_distance += fabs(controller->velocity.y) * dt;
        }
        
        /* Air control */
        float air_accel = controller->config.acceleration * controller->config.air_acceleration;
        
        UpeVec3 horizontal_desired = controller->desired_velocity;
        horizontal_desired.y = 0;
        
        float goal_speed = sqrtf(horizontal_desired.x * horizontal_desired.x + 
                                  horizontal_desired.z * horizontal_desired.z);
        
        /* Accelerate toward desired direction */
        if (goal_speed > 0.01f) {
            horizontal_desired.x = (horizontal_desired.x / goal_speed) * goal_speed;
            horizontal_desired.z = (horizontal_desired.z / goal_speed) * goal_speed;
            
            float accel_amount = air_accel * dt;
            controller->velocity.x += horizontal_desired.x * accel_amount / goal_speed;
            controller->velocity.z += horizontal_desired.z * accel_amount / goal_speed;
        }
    } else {
        /* Ground movement */
        float accel = (fabs(controller->desired_velocity.x) > 0.01f || 
                       fabs(controller->desired_velocity.z) > 0.01f) ?
                      controller->config.acceleration : 
                      controller->config.deceleration;
        
        /* Accelerate/decelerate horizontally */
        float accel_factor = accel * dt;
        controller->velocity.x += (controller->desired_velocity.x - controller->velocity.x) * accel_factor;
        controller->velocity.z += (controller->desired_velocity.z - controller->velocity.z) * accel_factor;
        
        /* Friction */
        controller->velocity.x *= 0.95f;
        controller->velocity.z *= 0.95f;
    }
    
    /* Update position from velocity */
    controller->root_position.x += controller->velocity.x * dt;
    controller->root_position.y += controller->velocity.y * dt;
    controller->root_position.z += controller->velocity.z * dt;
    
    /* Add platform velocity if on moving platform */
    if (controller->ground_body_id != 0) {
        controller->root_position.x += controller->platform_velocity.x * dt;
        controller->root_position.y += controller->platform_velocity.y * dt;
        controller->root_position.z += controller->platform_velocity.z * dt;
    }
    
    /* Update animation parameters */
    float horizontal_speed = sqrtf(controller->velocity.x * controller->velocity.x + 
                                    controller->velocity.z * controller->velocity.z);
    float max_speed = controller->config.sprint_speed;
    controller->speed_multiplier = fmin(1.0f, horizontal_speed / max_speed);
}

void UpeCharacterController_ApplyVelocity(
    UpeCharacterController *controller,
    UpeWorld *world)
{
    if (!controller || !world) return;
    
    /* Sync with physics body */
    UpeBody *body = UpeWorld_GetBody(world, controller->body_id);
    if (!body) return;
    body->velocity = controller->velocity;
    body->position = controller->root_position;
}

/*============================================================================
  State Query
============================================================================*/

UpeCharacterLocomotion UpeCharacterController_GetLocomotion(
    const UpeCharacterController *controller)
{
    return controller ? controller->locomotion : UPE_CHARCTRL_LOCOMOTION_IDLE;
}

bool UpeCharacterController_IsGrounded(const UpeCharacterController *controller) {
    return controller ? (controller->mode == UPE_CHARCTRL_MODE_GROUNDED) : false;
}

bool UpeCharacterController_IsInAir(const UpeCharacterController *controller) {
    if (!controller) return false;
    return controller->mode == UPE_CHARCTRL_MODE_JUMPING ||
           controller->mode == UPE_CHARCTRL_MODE_FALLING;
}

UpeVec3 UpeCharacterController_GetVelocity(const UpeCharacterController *controller) {
    return controller ? controller->velocity : UpeVec3{0.0f, 0.0f, 0.0f};
}

UpeVec3 UpeCharacterController_GetPosition(const UpeCharacterController *controller) {
    return controller ? controller->root_position : UpeVec3{0.0f, 0.0f, 0.0f};
}

float UpeCharacterController_GetFallDistance(const UpeCharacterController *controller) {
    return controller ? controller->falling_distance : 0.0f;
}

void UpeCharacterController_GetAnimationParameters(
    const UpeCharacterController *controller,
    float *out_speed,
    float *out_direction,
    bool *out_is_in_air)
{
    if (!controller) return;
    
    if (out_speed) *out_speed = controller->speed_multiplier;
    if (out_direction) *out_direction = controller->direction_angle;
    if (out_is_in_air) *out_is_in_air = UpeCharacterController_IsInAir(controller);
}

/*============================================================================
  Configuration Query (Runtime Tweak)
============================================================================*/

void UpeCharacterController_SetJumpPower(
    UpeCharacterController *controller,
    float jump_power)
{
    if (controller) {
        controller->config.jump_power = fmax(1.0f, jump_power);
    }
}

void UpeCharacterController_SetMaxSlope(
    UpeCharacterController *controller,
    float angle_degrees)
{
    if (controller) {
        controller->config.max_slope_angle = fmax(0.0f, fmin(90.0f, angle_degrees));
    }
}

void UpeCharacterController_SetTerminalVelocity(
    UpeCharacterController *controller,
    float terminal_velocity)
{
    if (controller) {
        controller->config.terminal_velocity = fmax(5.0f, terminal_velocity);
    }
}

/*============================================================================
  Preset Controllers
============================================================================*/

UpeCharacterController* UpeCharacterController_CreateHumanoid(
    UpeWorld *world,
    UpeVec3 position)
{
    UpeCharacterControllerDesc config = UpeCharacterController_GetDefaultConfig();
    /* Use defaults - they're already tuned for humanoids */
    return UpeCharacterController_Create(world, position, &config);
}

UpeCharacterController* UpeCharacterController_CreateActionHero(
    UpeWorld *world,
    UpeVec3 position)
{
    UpeCharacterControllerDesc config = UpeCharacterController_GetDefaultConfig();
    config.walk_speed = 3.0f;
    config.run_speed = 7.0f;
    config.sprint_speed = 10.0f;
    config.jump_power = 6.0f;
    config.jump_apex_delay = 0.15f;
    config.air_acceleration = 0.8f;
    config.acceleration = 15.0f;
    config.terminal_velocity = 30.0f;
    return UpeCharacterController_Create(world, position, &config);
}

UpeCharacterController* UpeCharacterController_CreateExplorer(
    UpeWorld *world,
    UpeVec3 position)
{
    UpeCharacterControllerDesc config = UpeCharacterController_GetDefaultConfig();
    config.walk_speed = 1.5f;
    config.run_speed = 3.0f;
    config.sprint_speed = 5.0f;
    config.jump_power = 4.0f;
    config.coyote_time = 0.2f;
    config.max_slope_angle = 60.0f;
    config.acceleration = 8.0f;
    config.terminal_velocity = 20.0f;
    return UpeCharacterController_Create(world, position, &config);
}

/*============================================================================
  Debug & Utility
============================================================================*/

char* UpeCharacterController_GetDebugString(const UpeCharacterController *controller) {
    if (!controller) return NULL;
    
    char *str = (char *)malloc(512);
    if (!str) return NULL;
    
    const char *mode_str = "";
    switch (controller->mode) {
        case UPE_CHARCTRL_MODE_GROUNDED:
            mode_str = "GROUNDED";
            break;
        case UPE_CHARCTRL_MODE_JUMPING:
            mode_str = "JUMPING";
            break;
        case UPE_CHARCTRL_MODE_FALLING:
            mode_str = "FALLING";
            break;
        case UPE_CHARCTRL_MODE_LEDGE_GRAB:
            mode_str = "LEDGE_GRAB";
            break;
    }
    
    const char *loco_str = "";
    switch (controller->locomotion) {
        case UPE_CHARCTRL_LOCOMOTION_IDLE:
            loco_str = "IDLE";
            break;
        case UPE_CHARCTRL_LOCOMOTION_WALKING:
            loco_str = "WALKING";
            break;
        case UPE_CHARCTRL_LOCOMOTION_RUNNING:
            loco_str = "RUNNING";
            break;
        case UPE_CHARCTRL_LOCOMOTION_SPRINTING:
            loco_str = "SPRINTING";
            break;
        case UPE_CHARCTRL_LOCOMOTION_STRAFING:
            loco_str = "STRAFING";
            break;
    }
    
    snprintf(str, 512,
             "CharCtrl: %s/%s\n"
             "Pos: (%.2f, %.2f, %.2f)\n"
             "Vel: (%.2f, %.2f, %.2f) |v|=%.2f\n"
             "AirTime: %.3f, FallDist: %.2f\n"
             "Speed: %.1f%%, Dir: %.1f°",
             mode_str, loco_str,
             controller->root_position.x, controller->root_position.y, controller->root_position.z,
             controller->velocity.x, controller->velocity.y, controller->velocity.z,
             sqrtf(controller->velocity.x * controller->velocity.x + 
                   controller->velocity.z * controller->velocity.z),
             controller->air_time,
             controller->falling_distance,
             controller->speed_multiplier * 100.0f,
             controller->direction_angle);
    
    return str;
}
