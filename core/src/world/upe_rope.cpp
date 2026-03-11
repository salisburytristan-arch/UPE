#include "upe/upe_rope.h"
#include "upe/upe_api.h"
#include "upe/upe_body.h"
#include <stdlib.h>
#include <string.h>

UpeRope *UpeRope_Create(uint32_t max_constraints) {
    UpeRope *rope = (UpeRope*)malloc(sizeof(UpeRope));
    if (!rope) {
        return nullptr;
    }
    
    rope->id = 0;
    rope->constraint_count = 0;
    rope->max_constraints = max_constraints;
    rope->constraints = (UpeRopeConstraint*)malloc(max_constraints * sizeof(UpeRopeConstraint));
    
    if (!rope->constraints) {
        free(rope);
        return nullptr;
    }
    
    memset(rope->constraints, 0, max_constraints * sizeof(UpeRopeConstraint));
    return rope;
}

void UpeRope_Destroy(UpeRope *rope) {
    if (!rope) {
        return;
    }
    free(rope->constraints);
    free(rope);
}

uint32_t UpeRope_AddConstraint(UpeRope *rope, const UpeRopeDesc *desc) {
    if (!rope || !desc || rope->constraint_count >= rope->max_constraints) {
        return 0xFFFFFFFF;
    }
    
    uint32_t idx = rope->constraint_count++;
    UpeRopeConstraint *constraint = &rope->constraints[idx];
    
    constraint->body_a_id = desc->body_a_id;
    constraint->body_b_id = desc->body_b_id;
    constraint->anchor_a = desc->anchor_a;
    constraint->anchor_b = desc->anchor_b;
    constraint->max_length = desc->max_length;
    constraint->elasticity = desc->elasticity > 0.0f ? desc->elasticity : 1.0f;
    constraint->is_taut = 0;
    constraint->current_length = 0.0f;
    constraint->lambda_rope = 0.0f;
    
    return idx;
}

void UpeRope_RemoveConstraint(UpeRope *rope, uint32_t idx) {
    if (!rope || idx >= rope->constraint_count) {
        return;
    }
    
    // Swap with last and reduce count
    if (idx < rope->constraint_count - 1) {
        rope->constraints[idx] = rope->constraints[rope->constraint_count - 1];
    }
    rope->constraint_count--;
}

UpeRopeConstraint *UpeRope_GetConstraint(UpeRope *rope, uint32_t idx) {
    if (!rope || idx >= rope->constraint_count) {
        return nullptr;
    }
    return &rope->constraints[idx];
}

void UpeRope_Update(UpeRope *rope, UpeWorld *world, float dt) {
    if (!rope || !world || rope->constraint_count == 0) {
        return;
    }
    
    // For each rope constraint, apply maximum distance constraint
    for (uint32_t i = 0; i < rope->constraint_count; ++i) {
        UpeRopeConstraint *constraint = &rope->constraints[i];
        
        UpeBody *body_a = UpeWorld_GetBody(world, constraint->body_a_id);
        UpeBody *body_b = UpeWorld_GetBody(world, constraint->body_b_id);
        
        if (!body_a || !body_b) {
            continue;
        }
        
        // Transform anchor points to world space
        UpeVec3 r_a = UpeQuat_RotateVec3(body_a->rotation, constraint->anchor_a);
        UpeVec3 r_b = UpeQuat_RotateVec3(body_b->rotation, constraint->anchor_b);
        UpeVec3 world_anchor_a = UpeVec3_Add(body_a->position, r_a);
        UpeVec3 world_anchor_b = UpeVec3_Add(body_b->position, r_b);
        
        // Compute current distance
        UpeVec3 diff = UpeVec3_Sub(world_anchor_b, world_anchor_a);
        float dist = UpeVec3_Length(diff);
        constraint->current_length = dist;
        
        // Check if rope is taut (distance exceeds max length)
        if (dist > constraint->max_length) {
            constraint->is_taut = 1;
            
            // Compute constraint direction
            UpeVec3 dir = UpeVec3_Scale(diff, 1.0f / dist);
            
            // Compute error (how much rope is over-stretched)
            float error = dist - constraint->max_length;
            
            // Rope can stretch slightly based on elasticity
            float effective_error = error * constraint->elasticity;
            
            // Compute relative velocity at rope attachment points
            UpeVec3 vel_a = UpeVec3_Add(body_a->velocity, UpeVec3_Cross(body_a->angular_velocity, r_a));
            UpeVec3 vel_b = UpeVec3_Add(body_b->velocity, UpeVec3_Cross(body_b->angular_velocity, r_b));
            UpeVec3 rel_vel = UpeVec3_Sub(vel_b, vel_a);
            
            float constraint_velocity = UpeVec3_Dot(rel_vel, dir);
            
            // Baumgarte stabilization
            const float baumgarte = 0.2f;
            float bias = baumgarte * effective_error / dt;
            
            // Effective mass
            UpeVec3 r_a_cross = UpeVec3_Cross(r_a, dir);
            UpeVec3 r_b_cross = UpeVec3_Cross(r_b, dir);
            
            float k = body_a->inv_mass + body_b->inv_mass;
            k += UpeVec3_Dot(UpeVec3_Scale(r_a_cross, body_a->inv_inertia.x), r_a_cross);
            k += UpeVec3_Dot(UpeVec3_Scale(r_b_cross, body_b->inv_inertia.x), r_b_cross);
            
            if (k < 1.0e-8f) {
                continue;
            }
            
            // Compute impulse magnitude (one-sided: only prevent extension)
            float jv = bias - constraint_velocity;
            float delta_lambda = jv / k;
            
            // Accumulate and clamp to non-negative (rope can only pull, not push)
            float old_lambda = constraint->lambda_rope;
            constraint->lambda_rope = old_lambda + delta_lambda;
            if (constraint->lambda_rope < 0.0f) {
                constraint->lambda_rope = 0.0f;
            }
            delta_lambda = constraint->lambda_rope - old_lambda;
            
            // Apply impulse
            UpeVec3 impulse = UpeVec3_Scale(dir, delta_lambda);
            
            if (body_a->type == UPE_BODY_DYNAMIC) {
                body_a->velocity = UpeVec3_Sub(body_a->velocity, UpeVec3_Scale(impulse, body_a->inv_mass));
                UpeVec3 ang_impulse_a = UpeVec3_Cross(r_a, UpeVec3_Scale(impulse, -1.0f));
                body_a->angular_velocity = UpeVec3_Add(body_a->angular_velocity,
                    UpeVec3_Scale(ang_impulse_a, body_a->inv_inertia.x));
            }
            
            if (body_b->type == UPE_BODY_DYNAMIC) {
                body_b->velocity = UpeVec3_Add(body_b->velocity, UpeVec3_Scale(impulse, body_b->inv_mass));
                UpeVec3 ang_impulse_b = UpeVec3_Cross(r_b, impulse);
                body_b->angular_velocity = UpeVec3_Add(body_b->angular_velocity,
                    UpeVec3_Scale(ang_impulse_b, body_b->inv_inertia.x));
            }
        } else {
            constraint->is_taut = 0;
            constraint->lambda_rope = 0.0f;  // Reset when slack
        }
        
        // Apply motor dynamics if active
        if (constraint->motor_enabled) {
            UpeVec3 r_a = UpeQuat_RotateVec3(body_a->rotation, constraint->anchor_a);
            UpeVec3 r_b = UpeQuat_RotateVec3(body_b->rotation, constraint->anchor_b);
            UpeVec3 world_anchor_a = UpeVec3_Add(body_a->position, r_a);
            UpeVec3 world_anchor_b = UpeVec3_Add(body_b->position, r_b);
            UpeVec3 diff = UpeVec3_Sub(world_anchor_b, world_anchor_a);
            float dist = UpeVec3_Length(diff);
            
            if (dist > 0.0001f) {
                UpeVec3 dir = UpeVec3_Scale(diff, 1.0f / dist);
                
                // Target length changes at motor speed
                float target_velocity = constraint->motor_speed;
                constraint->max_length -= target_velocity * dt;
                if (constraint->max_length < constraint->min_length) {
                    constraint->max_length = constraint->min_length;
                }
                
                // Get relative velocity at attachment points
                UpeVec3 vel_a = UpeVec3_Add(body_a->velocity, UpeVec3_Cross(body_a->angular_velocity, r_a));
                UpeVec3 vel_b = UpeVec3_Add(body_b->velocity, UpeVec3_Cross(body_b->angular_velocity, r_b));
                (void)vel_a;
                (void)vel_b;
                
                // Scale motor force based on max_motor_force
                float force_scale = constraint->max_motor_force / (constraint->constraint_count > 0 ? (float)constraint->constraint_count : 1.0f);
                
                // Apply motor impulse if not at limits
                if ((constraint->motor_speed < 0 && dist > constraint->min_length) ||
                    (constraint->motor_speed > 0 && dist < constraint->max_length)) {
                    float motor_impulse = -force_scale * constraint->motor_speed * dt;
                    UpeVec3 impulse = UpeVec3_Scale(dir, motor_impulse);
                    
                    if (body_a->type == UPE_BODY_DYNAMIC) {
                        body_a->velocity = UpeVec3_Sub(body_a->velocity, UpeVec3_Scale(impulse, body_a->inv_mass));
                    }
                    if (body_b->type == UPE_BODY_DYNAMIC) {
                        body_b->velocity = UpeVec3_Add(body_b->velocity, UpeVec3_Scale(impulse, body_b->inv_mass));
                    }
                }
            }
        }
    }
}

void UpeRope_EnableMotor(UpeRope *rope, uint32_t idx, float target_speed, float max_force) {
    if (!rope || idx >= rope->constraint_count) {
        return;
    }
    UpeRopeConstraint *constraint = &rope->constraints[idx];
    constraint->motor_enabled = 1;
    constraint->motor_speed = target_speed;
    constraint->max_motor_force = max_force;
    constraint->min_length = 0.1f;
}

void UpeRope_DisableMotor(UpeRope *rope, uint32_t idx) {
    if (!rope || idx >= rope->constraint_count) {
        return;
    }
    rope->constraints[idx].motor_enabled = 0;
}

void UpeRope_SetMotorSpeed(UpeRope *rope, uint32_t idx, float speed) {
    if (!rope || idx >= rope->constraint_count) {
        return;
    }
    rope->constraints[idx].motor_speed = speed;
}

int UpeRope_IsTaut(UpeRope *rope, uint32_t idx) {
    if (!rope || idx >= rope->constraint_count) {
        return 0;
    }
    return rope->constraints[idx].is_taut;
}

float UpeRope_GetTension(UpeRope *rope, uint32_t idx) {
    if (!rope || idx >= rope->constraint_count) {
        return 0.0f;
    }
    return rope->constraints[idx].lambda_rope;
}
