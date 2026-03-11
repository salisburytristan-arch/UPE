#include "upe/upe_2d_physics.h"
#include "upe/upe_determinism.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// World Management
// ============================================================================

void Upe2DWorld_Init(Upe2DWorld *world, UpeVec2 gravity) {
    world->gravity = gravity;
    world->dt = 0.016f;
    world->iteration_count = 10;
    world->frame_index = 0;
    world->replay = NULL;
    
    world->body_capacity = 256;
    world->bodies = (Upe2DBody *)malloc(world->body_capacity * sizeof(Upe2DBody));
    world->body_count = 0;
    
    world->shape_capacity = 256;
    world->shapes = (Upe2DShape *)malloc(world->shape_capacity * sizeof(Upe2DShape));
    world->shape_count = 0;
    
    world->contact_capacity = 512;
    world->contacts = (Upe2DContact *)malloc(world->contact_capacity * sizeof(Upe2DContact));
    world->contact_count = 0;
}

void Upe2DWorld_Destroy(Upe2DWorld *world) {
    free(world->bodies);
    free(world->shapes);
    free(world->contacts);
    world->bodies = NULL;
    world->shapes = NULL;
    world->contacts = NULL;
}

// ============================================================================
// Body Management
// ============================================================================

uint32_t Upe2DWorld_CreateBody(Upe2DWorld *world, Upe2DBodyType type, UpeVec2 position, float rotation, float mass) {
    if (world->body_count >= world->body_capacity) {
        world->body_capacity *= 2;
        world->bodies = (Upe2DBody *)realloc(world->bodies, world->body_capacity * sizeof(Upe2DBody));
    }
    
    uint32_t id = world->body_count++;
    Upe2DBody *body = &world->bodies[id];
    
    body->id = id;
    body->type = type;
    body->position = position;
    body->rotation = rotation;
    body->velocity.x = 0.0f;
    body->velocity.y = 0.0f;
    body->angular_velocity = 0.0f;
    body->mass = mass;
    body->inv_mass = (mass > 0.0f && type == UPE_2D_BODY_DYNAMIC) ? (1.0f / mass) : 0.0f;
    body->inertia = mass * 0.5f;  // Simplified
    body->inv_inertia = (mass > 0.0f && type == UPE_2D_BODY_DYNAMIC) ? (1.0f / body->inertia) : 0.0f;
    body->restitution = 0.5f;
    body->friction = 0.3f;
    body->linear_damping = 0.01f;
    body->angular_damping = 0.05f;
    body->force.x = 0.0f;
    body->force.y = 0.0f;
    body->torque = 0.0f;
    body->shape_id = 0;
    body->is_asleep = 0;

    if (world->replay) {
        float data[5] = {position.x, position.y, rotation, (float)type, mass};
        UpeReplay_RecordSpawn(world->replay, world->frame_index, id, data, 5);
    }
    
    return id;
}

void Upe2DWorld_DestroyBody(Upe2DWorld *world, uint32_t body_id) {
    if (!world || body_id >= world->body_count) {
        return;
    }

    if (world->replay) {
        UpeReplay_RecordDestroy(world->replay, world->frame_index, body_id);
    }

    uint32_t last = world->body_count - 1;
    if (body_id != last) {
        world->bodies[body_id] = world->bodies[last];

        uint32_t shape_id = world->bodies[body_id].shape_id;
        if (shape_id < world->shape_count) {
            world->shapes[shape_id].body_id = body_id;
        }
    }
    world->body_count--;
}

Upe2DBody *Upe2DWorld_GetBody(Upe2DWorld *world, uint32_t body_id) {
    if (body_id < world->body_count) {
        return &world->bodies[body_id];
    }
    return NULL;
}

// ============================================================================
// Shape Management
// ============================================================================

uint32_t Upe2DWorld_CreateCircleShape(Upe2DWorld *world, uint32_t body_id, UpeVec2 center, float radius) {
    if (world->shape_count >= world->shape_capacity) {
        world->shape_capacity *= 2;
        world->shapes = (Upe2DShape *)realloc(world->shapes, world->shape_capacity * sizeof(Upe2DShape));
    }
    
    uint32_t id = world->shape_count++;
    Upe2DShape *shape = &world->shapes[id];
    
    shape->id = id;
    shape->type = UPE_2D_SHAPE_CIRCLE;
    shape->body_id = body_id;
    shape->circle.center = center;
    shape->circle.radius = radius;
    
    // Link shape to body
    if (body_id < world->body_count) {
        world->bodies[body_id].shape_id = id;
    }
    
    return id;
}

uint32_t Upe2DWorld_CreateBoxShape(Upe2DWorld *world, uint32_t body_id, UpeVec2 center, float half_width, float half_height) {
    if (world->shape_count >= world->shape_capacity) {
        world->shape_capacity *= 2;
        world->shapes = (Upe2DShape *)realloc(world->shapes, world->shape_capacity * sizeof(Upe2DShape));
    }
    
    uint32_t id = world->shape_count++;
    Upe2DShape *shape = &world->shapes[id];
    
    shape->id = id;
    shape->type = UPE_2D_SHAPE_BOX;
    shape->body_id = body_id;
    shape->box.center = center;
    shape->box.half_width = half_width;
    shape->box.half_height = half_height;
    
    // Link shape to body
    if (body_id < world->body_count) {
        world->bodies[body_id].shape_id = id;
    }
    
    return id;
}

void Upe2DWorld_DestroyShape(Upe2DWorld *world, uint32_t shape_id) {
    if (!world || shape_id >= world->shape_count) {
        return;
    }

    uint32_t last = world->shape_count - 1;
    if (shape_id != last) {
        world->shapes[shape_id] = world->shapes[last];
        uint32_t body_id = world->shapes[shape_id].body_id;
        if (body_id < world->body_count) {
            world->bodies[body_id].shape_id = shape_id;
        }
    }
    world->shape_count--;
}

// ============================================================================
// Force Application
// ============================================================================

void Upe2DBody_ApplyForce(Upe2DBody *body, UpeVec2 force) {
    if (body->type == UPE_2D_BODY_DYNAMIC) {
        body->force = UpeVec2_Add(body->force, force);
    }
}

void Upe2DWorld_ApplyForce(Upe2DWorld *world, uint32_t body_id, UpeVec2 force) {
    if (!world) {
        return;
    }
    Upe2DBody *body = Upe2DWorld_GetBody(world, body_id);
    if (!body) {
        return;
    }
    Upe2DBody_ApplyForce(body, force);
    if (world->replay) {
        float data[4] = {force.x, force.y, 0.0f, 0.0f};
        UpeReplay_RecordInput(world->replay, world->frame_index, body_id, data, 4);
    }
}

void Upe2DBody_ApplyImpulse(Upe2DBody *body, UpeVec2 impulse) {
    if (body->type == UPE_2D_BODY_DYNAMIC) {
        body->velocity = UpeVec2_Add(body->velocity, UpeVec2_Scale(impulse, body->inv_mass));
    }
}

void Upe2DWorld_ApplyImpulse(Upe2DWorld *world, uint32_t body_id, UpeVec2 impulse) {
    if (!world) {
        return;
    }
    Upe2DBody *body = Upe2DWorld_GetBody(world, body_id);
    if (!body) {
        return;
    }
    Upe2DBody_ApplyImpulse(body, impulse);
    if (world->replay) {
        float data[4] = {impulse.x, impulse.y, 0.0f, 2.0f};
        UpeReplay_RecordInput(world->replay, world->frame_index, body_id, data, 4);
    }
}

void Upe2DBody_ApplyTorque(Upe2DBody *body, float torque) {
    if (body->type == UPE_2D_BODY_DYNAMIC) {
        body->torque += torque;
    }
}

void Upe2DWorld_ApplyTorque(Upe2DWorld *world, uint32_t body_id, float torque) {
    if (!world) {
        return;
    }
    Upe2DBody *body = Upe2DWorld_GetBody(world, body_id);
    if (!body) {
        return;
    }
    Upe2DBody_ApplyTorque(body, torque);
    if (world->replay) {
        float data[4] = {torque, 0.0f, 0.0f, 1.0f};
        UpeReplay_RecordInput(world->replay, world->frame_index, body_id, data, 4);
    }
}

// ============================================================================
// Collision Detection
// ============================================================================

int Upe2D_TestCircleCircle(const Upe2DCircle *a, const Upe2DCircle *b, UpeVec2 *normal, float *penetration) {
    UpeVec2 diff = UpeVec2_Sub(b->center, a->center);
    float distance = UpeVec2_Length(diff);
    float sum_radius = a->radius + b->radius;
    
    if (distance < sum_radius) {
        *penetration = sum_radius - distance;
        if (distance > 0.0001f) {
            *normal = UpeVec2_Normalize(diff);
        } else {
            normal->x = 1.0f;
            normal->y = 0.0f;  // Arbitrary normal
        }
        return 1;
    }
    
    return 0;
}

int Upe2D_TestCircleBox(const Upe2DCircle *circle, const Upe2DBox *box, UpeVec2 *normal, float *penetration) {
    // Find closest point on box to circle center
    UpeVec2 diff = UpeVec2_Sub(circle->center, box->center);
    
    float closest_x = diff.x;
    if (closest_x > box->half_width) closest_x = box->half_width;
    if (closest_x < -box->half_width) closest_x = -box->half_width;
    
    float closest_y = diff.y;
    if (closest_y > box->half_height) closest_y = box->half_height;
    if (closest_y < -box->half_height) closest_y = -box->half_height;
    
    UpeVec2 closest = {box->center.x + closest_x, box->center.y + closest_y};
    UpeVec2 distance_vec = UpeVec2_Sub(circle->center, closest);
    float distance = UpeVec2_Length(distance_vec);
    
    if (distance < circle->radius) {
        *penetration = circle->radius - distance;
        if (distance > 0.0001f) {
            *normal = UpeVec2_Normalize(distance_vec);
        } else {
            normal->x = 0.0f;
            normal->y = 1.0f;
        }
        return 1;
    }
    
    return 0;
}

// ============================================================================
// Physics Step
// ============================================================================

void Upe2DWorld_Step(Upe2DWorld *world, float dt) {
    world->dt = dt;
    
    // Apply forces and integrate velocities
    for (uint32_t i = 0; i < world->body_count; ++i) {
        Upe2DBody *body = &world->bodies[i];
        
        if (body->type != UPE_2D_BODY_DYNAMIC || body->is_asleep) continue;
        
        // Apply gravity
        UpeVec2 gravity_force = UpeVec2_Scale(world->gravity, body->mass);
        Upe2DBody_ApplyForce(body, gravity_force);
        
        // Integrate velocity
        body->velocity = UpeVec2_Add(body->velocity, UpeVec2_Scale(body->force, body->inv_mass * dt));
        body->angular_velocity += body->torque * body->inv_inertia * dt;
        
        // Apply damping
        body->velocity = UpeVec2_Scale(body->velocity, 1.0f - body->linear_damping);
        body->angular_velocity *= (1.0f - body->angular_damping);
        
        // Clear forces
        body->force.x = 0.0f;
        body->force.y = 0.0f;
        body->torque = 0.0f;
    }
    
    // Broadphase collision detection (N^2 for simplicity)
    world->contact_count = 0;
    
    for (uint32_t i = 0; i < world->body_count; ++i) {
        for (uint32_t j = i + 1; j < world->body_count; ++j) {
            Upe2DBody *body_a = &world->bodies[i];
            Upe2DBody *body_b = &world->bodies[j];
            
            // Skip if both static or both asleep
            if (body_a->type == UPE_2D_BODY_STATIC && body_b->type == UPE_2D_BODY_STATIC) continue;
            if (body_a->is_asleep && body_b->is_asleep) continue;
            
            // Get shapes
            Upe2DShape *shape_a = (body_a->shape_id < world->shape_count) ? &world->shapes[body_a->shape_id] : NULL;
            Upe2DShape *shape_b = (body_b->shape_id < world->shape_count) ? &world->shapes[body_b->shape_id] : NULL;
            
            if (!shape_a || !shape_b) continue;
            
            // Narrowphase
            UpeVec2 normal;
            float penetration;
            int colliding = 0;
            
            if (shape_a->type == UPE_2D_SHAPE_CIRCLE && shape_b->type == UPE_2D_SHAPE_CIRCLE) {
                colliding = Upe2D_TestCircleCircle(&shape_a->circle, &shape_b->circle, &normal, &penetration);
            }
            
            if (colliding && world->contact_count < world->contact_capacity) {
                Upe2DContact *contact = &world->contacts[world->contact_count++];
                contact->body_a = body_a->id;
                contact->body_b = body_b->id;
                contact->normal = normal;
                contact->penetration = penetration;
                contact->point = UpeVec2_Add(body_a->position, UpeVec2_Scale(normal, shape_a->circle.radius));
                contact->normal_impulse = 0.0f;
                contact->tangent_impulse = 0.0f;
            }
        }
    }
    
    // Solve contacts (Sequential Impulse)
    for (uint32_t iter = 0; iter < world->iteration_count; ++iter) {
        for (uint32_t i = 0; i < world->contact_count; ++i) {
            Upe2DContact *contact = &world->contacts[i];
            Upe2DBody *body_a = &world->bodies[contact->body_a];
            Upe2DBody *body_b = &world->bodies[contact->body_b];
            
            // Relative velocity
            UpeVec2 rel_vel = UpeVec2_Sub(body_b->velocity, body_a->velocity);
            float vel_along_normal = UpeVec2_Dot(rel_vel, contact->normal);
            
            // Don't resolve if velocities are separating
            if (vel_along_normal > 0) continue;
            
            // Calculate impulse
            float restitution = (body_a->restitution + body_b->restitution) * 0.5f;
            float j = -(1.0f + restitution) * vel_along_normal;
            j /= (body_a->inv_mass + body_b->inv_mass);
            
            // Apply impulse
            UpeVec2 impulse = UpeVec2_Scale(contact->normal, j);
            
            if (body_a->type == UPE_2D_BODY_DYNAMIC) {
                body_a->velocity = UpeVec2_Sub(body_a->velocity, UpeVec2_Scale(impulse, body_a->inv_mass));
            }
            if (body_b->type == UPE_2D_BODY_DYNAMIC) {
                body_b->velocity = UpeVec2_Add(body_b->velocity, UpeVec2_Scale(impulse, body_b->inv_mass));
            }
        }
    }
    
    // Integrate positions
    for (uint32_t i = 0; i < world->body_count; ++i) {
        Upe2DBody *body = &world->bodies[i];
        
        if (body->type != UPE_2D_BODY_DYNAMIC || body->is_asleep) continue;
        
        body->position = UpeVec2_Add(body->position, UpeVec2_Scale(body->velocity, dt));
        body->rotation += body->angular_velocity * dt;
    }

    world->frame_index += 1u;
}

void Upe2DWorld_SetReplayRecorder(Upe2DWorld *world, UpeReplayRecorder *replay) {
    if (!world) {
        return;
    }
    world->replay = replay;
}
