#include "upe/upe_trigger_system.h"
#include "upe_world.h"
#include "upe/upe_body.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

void UpeTriggerSystem_Init(UpeTriggerSystem *sys, void *field_system) {
    if (!sys) {
        return;
    }
    
    sys->trigger_capacity = 256;
    sys->trigger_count = 0;
    sys->triggers = (UpeTrigger*)malloc(sys->trigger_capacity * sizeof(UpeTrigger));
    
    sys->event_capacity = 1024;
    sys->event_count = 0;
    sys->event_queue = (UpeTriggerEventData*)malloc(sys->event_capacity * sizeof(UpeTriggerEventData));
    
    sys->callback_count = 0;
    sys->callbacks = (UpeTriggerCallback*)malloc(16 * sizeof(UpeTriggerCallback));
    
    sys->field_system = field_system;
    sys->trigger_broadphase = NULL;
}

void UpeTriggerSystem_Destroy(UpeTriggerSystem *sys) {
    if (!sys) {
        return;
    }
    
    // Free all trigger body lists
    for (uint32_t i = 0; i < sys->trigger_count; ++i) {
        free(sys->triggers[i].bodies_inside);
    }
    
    free(sys->triggers);
    free(sys->event_queue);
    free(sys->callbacks);
    
    sys->triggers = NULL;
    sys->event_queue = NULL;
    sys->callbacks = NULL;
}

uint32_t UpeTrigger_CreateBox(UpeTriggerSystem *sys, UpeVec3 center, UpeVec3 half_extents,
                              UpeQuat rotation, const char *name) {
    if (!sys || sys->trigger_count >= sys->trigger_capacity) {
        return 0xFFFFFFFF;
    }
    
    uint32_t id = sys->trigger_count++;
    UpeTrigger *trigger = &sys->triggers[id];
    
    trigger->id = id;
    strncpy(trigger->name, name ? name : "Box", sizeof(trigger->name) - 1);
    trigger->type = UPE_TRIGGER_BOX;
    trigger->position = center;
    trigger->rotation = rotation;
    trigger->params[0] = half_extents.x;
    trigger->params[1] = half_extents.y;
    trigger->params[2] = half_extents.z;
    trigger->layer_mask = 0xFFFFFFFF;
    trigger->active = 1;
    
    trigger->body_capacity = 64;
    trigger->body_count = 0;
    trigger->bodies_inside = (uint32_t*)malloc(trigger->body_capacity * sizeof(uint32_t));
    trigger->user_data = NULL;
    
    return id;
}

uint32_t UpeTrigger_CreateSphere(UpeTriggerSystem *sys, UpeVec3 center, float radius,
                                 const char *name) {
    if (!sys || sys->trigger_count >= sys->trigger_capacity) {
        return 0xFFFFFFFF;
    }
    
    uint32_t id = sys->trigger_count++;
    UpeTrigger *trigger = &sys->triggers[id];
    
    trigger->id = id;
    strncpy(trigger->name, name ? name : "Sphere", sizeof(trigger->name) - 1);
    trigger->type = UPE_TRIGGER_SPHERE;
    trigger->position = center;
    trigger->rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    trigger->params[0] = radius;
    trigger->layer_mask = 0xFFFFFFFF;
    trigger->active = 1;
    
    trigger->body_capacity = 64;
    trigger->body_count = 0;
    trigger->bodies_inside = (uint32_t*)malloc(trigger->body_capacity * sizeof(uint32_t));
    trigger->user_data = NULL;
    
    return id;
}

uint32_t UpeTrigger_CreateCapsule(UpeTriggerSystem *sys, UpeVec3 point_a, UpeVec3 point_b,
                                  float radius, const char *name) {
    if (!sys || sys->trigger_count >= sys->trigger_capacity) {
        return 0xFFFFFFFF;
    }
    
    uint32_t id = sys->trigger_count++;
    UpeTrigger *trigger = &sys->triggers[id];
    
    trigger->id = id;
    strncpy(trigger->name, name ? name : "Capsule", sizeof(trigger->name) - 1);
    trigger->type = UPE_TRIGGER_CAPSULE;
    trigger->position = UpeVec3_Scale(UpeVec3_Add(point_a, point_b), 0.5f);
    trigger->rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    trigger->params[0] = radius;
    trigger->params[1] = point_a.x;
    trigger->params[2] = point_a.y;
    trigger->params[3] = point_a.z;
    trigger->layer_mask = 0xFFFFFFFF;
    trigger->active = 1;
    
    trigger->body_capacity = 64;
    trigger->body_count = 0;
    trigger->bodies_inside = (uint32_t*)malloc(trigger->body_capacity * sizeof(uint32_t));
    trigger->user_data = NULL;
    
    return id;
}

uint32_t UpeTrigger_CreateField(UpeTriggerSystem *sys, uint32_t field_id,
                                UpeFieldCondition condition, float threshold_min,
                                float threshold_max, const char *name) {
    if (!sys || sys->trigger_count >= sys->trigger_capacity) {
        return 0xFFFFFFFF;
    }
    
    uint32_t id = sys->trigger_count++;
    UpeTrigger *trigger = &sys->triggers[id];
    
    trigger->id = id;
    strncpy(trigger->name, name ? name : "Field", sizeof(trigger->name) - 1);
    trigger->type = UPE_TRIGGER_FIELD;
    trigger->field_id = field_id;
    trigger->condition = condition;
    trigger->threshold_min = threshold_min;
    trigger->threshold_max = threshold_max;
    trigger->layer_mask = 0xFFFFFFFF;
    trigger->active = 1;
    
    trigger->body_capacity = 64;
    trigger->body_count = 0;
    trigger->bodies_inside = (uint32_t*)malloc(trigger->body_capacity * sizeof(uint32_t));
    trigger->user_data = NULL;
    
    return id;
}

uint32_t UpeTriggerSystem_Remove(UpeTriggerSystem *sys, uint32_t trigger_id) {
    if (!sys || trigger_id >= sys->trigger_count) {
        return 0xFFFFFFFF;
    }
    
    free(sys->triggers[trigger_id].bodies_inside);
    
    // Swap with last
    if (trigger_id < sys->trigger_count - 1) {
        sys->triggers[trigger_id] = sys->triggers[sys->trigger_count - 1];
        sys->triggers[trigger_id].id = trigger_id;
    }
    sys->trigger_count--;
    return trigger_id;
}

void UpeTriggerSystem_SetActive(UpeTriggerSystem *sys, uint32_t trigger_id, int active) {
    if (!sys || trigger_id >= sys->trigger_count) {
        return;
    }
    sys->triggers[trigger_id].active = active;
}

void UpeTriggerSystem_SetLayerMask(UpeTriggerSystem *sys, uint32_t trigger_id, uint32_t layer_mask) {
    if (!sys || trigger_id >= sys->trigger_count) {
        return;
    }
    sys->triggers[trigger_id].layer_mask = layer_mask;
}

void UpeTriggerSystem_RegisterCallback(UpeTriggerSystem *sys, UpeTriggerCallback callback) {
    if (!sys || sys->callback_count >= 16) {
        return;
    }
    sys->callbacks[sys->callback_count++] = callback;
}

int UpeTrigger_TestPoint(const UpeTrigger *trigger, UpeVec3 point) {
    if (!trigger || !trigger->active) {
        return 0;
    }
    
    switch (trigger->type) {
        case UPE_TRIGGER_SPHERE: {
            float radius_sq = trigger->params[0] * trigger->params[0];
            UpeVec3 diff = UpeVec3_Sub(point, trigger->position);
            float dist_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
            return dist_sq <= radius_sq;
        }
        
        case UPE_TRIGGER_BOX: {
            // Transform point to trigger local space
            UpeQuat inv_rot = {-trigger->rotation.x, -trigger->rotation.y, -trigger->rotation.z, trigger->rotation.w};
            UpeVec3 local_point = UpeQuat_RotateVec3(inv_rot, UpeVec3_Sub(point, trigger->position));
            
            return (local_point.x >= -trigger->params[0] && local_point.x <= trigger->params[0] &&
                    local_point.y >= -trigger->params[1] && local_point.y <= trigger->params[1] &&
                    local_point.z >= -trigger->params[2] && local_point.z <= trigger->params[2]);
        }
        
        default:
            return 0;
    }
}

int UpeTrigger_TestBody(const UpeTrigger *trigger, uint32_t body_id, void *world) {
    if (!trigger || !world) {
        return 0;
    }
    
    UpeWorld *w = (UpeWorld*)world;
    UpeBody *body = UpeWorld_GetBody(w, body_id);
    if (!body) {
        return 0;
    }
    
    // Simple point test using body center
    return UpeTrigger_TestPoint(trigger, body->position);
}

static void UpeTrigger_AddEvent(UpeTriggerSystem *sys, uint32_t trigger_id, uint32_t body_id,
                                UpeTriggerEvent event_type, UpeVec3 contact_point,
                                UpeVec3 body_velocity, float event_time) {
    if (!sys || sys->event_count >= sys->event_capacity) {
        return;
    }
    
    UpeTriggerEventData *event = &sys->event_queue[sys->event_count++];
    event->trigger_id = trigger_id;
    event->body_id = body_id;
    event->event_type = event_type;
    event->frame_number = 0; // Will be set externally if needed
    event->event_time = event_time;
    event->contact_point = contact_point;
    event->body_velocity = body_velocity;
    event->field_value = 0.0f;
}

static int UpeTrigger_SegmentSphere(UpeVec3 p0, UpeVec3 p1, UpeVec3 center, float radius,
                                    float *t_enter, float *t_exit) {
    UpeVec3 d = UpeVec3_Sub(p1, p0);
    UpeVec3 m = UpeVec3_Sub(p0, center);
    float a = UpeVec3_Dot(d, d);
    float b = 2.0f * UpeVec3_Dot(m, d);
    float c = UpeVec3_Dot(m, m) - radius * radius;

    if (a < 1.0e-8f) {
        return 0;
    }

    float disc = b * b - 4.0f * a * c;
    if (disc < 0.0f) {
        return 0;
    }

    float sqrt_disc = sqrtf(disc);
    float inv = 1.0f / (2.0f * a);
    float t0 = (-b - sqrt_disc) * inv;
    float t1 = (-b + sqrt_disc) * inv;
    if (t0 > t1) {
        float temp = t0;
        t0 = t1;
        t1 = temp;
    }

    if (t1 < 0.0f || t0 > 1.0f) {
        return 0;
    }

    if (t_enter) {
        *t_enter = t0 < 0.0f ? 0.0f : t0;
    }
    if (t_exit) {
        *t_exit = t1 > 1.0f ? 1.0f : t1;
    }
    return 1;
}

static int UpeTrigger_SegmentBoxLocal(UpeVec3 p0, UpeVec3 p1, UpeVec3 half_extents,
                                      float *t_enter, float *t_exit) {
    UpeVec3 d = UpeVec3_Sub(p1, p0);
    float tmin = 0.0f;
    float tmax = 1.0f;
    const float eps = 1.0e-8f;

    float min_vals[3] = {-half_extents.x, -half_extents.y, -half_extents.z};
    float max_vals[3] = {half_extents.x, half_extents.y, half_extents.z};
    float p_vals[3] = {p0.x, p0.y, p0.z};
    float d_vals[3] = {d.x, d.y, d.z};

    for (int i = 0; i < 3; ++i) {
        float pi = p_vals[i];
        float di = d_vals[i];
        float min_v = min_vals[i];
        float max_v = max_vals[i];

        if (fabsf(di) < eps) {
            if (pi < min_v || pi > max_v) {
                return 0;
            }
            continue;
        }

        float inv_d = 1.0f / di;
        float t1 = (min_v - pi) * inv_d;
        float t2 = (max_v - pi) * inv_d;
        if (t1 > t2) {
            float temp = t1;
            t1 = t2;
            t2 = temp;
        }
        if (t1 > tmin) {
            tmin = t1;
        }
        if (t2 < tmax) {
            tmax = t2;
        }
        if (tmin > tmax) {
            return 0;
        }
    }

    if (t_enter) {
        *t_enter = tmin;
    }
    if (t_exit) {
        *t_exit = tmax;
    }
    return 1;
}

void UpeTriggerSystem_Update(UpeTriggerSystem *sys, void *world) {
    if (!sys || !world) {
        return;
    }
    
    UpeWorld *w = (UpeWorld*)world;
    
    // Check all active triggers
    for (uint32_t t = 0; t < sys->trigger_count; ++t) {
        UpeTrigger *trigger = &sys->triggers[t];
        if (!trigger->active) {
            continue;
        }
        
        // Get all bodies in world (this could use broadphase for optimization)
        for (uint32_t b = 0; b < w->body_count; ++b) {
            UpeBody *body = &w->bodies[b];
            if (body->type == UPE_BODY_STATIC) {
                continue;
            }
            
            // Layer mask filtering
            if ((trigger->layer_mask & (1u << body->collision_layer)) == 0) {
                continue;
            }
            
            int is_inside = UpeTrigger_TestBody(trigger, body->id, world);
            
            // Check if body was inside last frame
            int was_inside = 0;
            for (uint32_t i = 0; i < trigger->body_count; ++i) {
                if (trigger->bodies_inside[i] == body->id) {
                    was_inside = 1;
                    break;
                }
            }
            
            // Generate events based on state changes
            if (is_inside && !was_inside) {
                float event_time = 0.0f;
                UpeVec3 contact_point = body->position;
                if (body->type == UPE_BODY_DYNAMIC) {
                    float dt = w->fixed_dt;
                    UpeVec3 prev_pos = UpeVec3_Sub(body->position, UpeVec3_Scale(body->velocity, dt));
                    float t_enter = 0.0f;
                    float t_exit = 0.0f;
                    if (trigger->type == UPE_TRIGGER_SPHERE) {
                        if (UpeTrigger_SegmentSphere(prev_pos, body->position, trigger->position,
                                                     trigger->params[0], &t_enter, &t_exit)) {
                            event_time = t_enter * dt;
                            contact_point = UpeVec3_Add(prev_pos, UpeVec3_Scale(UpeVec3_Sub(body->position, prev_pos), t_enter));
                        }
                    } else if (trigger->type == UPE_TRIGGER_BOX) {
                        UpeQuat inv_rot = {-trigger->rotation.x, -trigger->rotation.y, -trigger->rotation.z, trigger->rotation.w};
                        UpeVec3 local_prev = UpeQuat_RotateVec3(inv_rot, UpeVec3_Sub(prev_pos, trigger->position));
                        UpeVec3 local_curr = UpeQuat_RotateVec3(inv_rot, UpeVec3_Sub(body->position, trigger->position));
                        UpeVec3 half_extents = {trigger->params[0], trigger->params[1], trigger->params[2]};
                        if (UpeTrigger_SegmentBoxLocal(local_prev, local_curr, half_extents, &t_enter, &t_exit)) {
                            event_time = t_enter * dt;
                            contact_point = UpeVec3_Add(prev_pos, UpeVec3_Scale(UpeVec3_Sub(body->position, prev_pos), t_enter));
                        }
                    }
                }
                // ENTER event
                UpeTrigger_AddEvent(sys, trigger->id, body->id, UPE_TRIGGER_ENTER,
                                   contact_point, body->velocity, event_time);
                
                // Add to inside list
                if (trigger->body_count < trigger->body_capacity) {
                    trigger->bodies_inside[trigger->body_count++] = body->id;
                }
            } else if (!is_inside && was_inside) {
                float event_time = 0.0f;
                UpeVec3 contact_point = body->position;
                if (body->type == UPE_BODY_DYNAMIC) {
                    float dt = w->fixed_dt;
                    UpeVec3 prev_pos = UpeVec3_Sub(body->position, UpeVec3_Scale(body->velocity, dt));
                    float t_enter = 0.0f;
                    float t_exit = 0.0f;
                    if (trigger->type == UPE_TRIGGER_SPHERE) {
                        if (UpeTrigger_SegmentSphere(prev_pos, body->position, trigger->position,
                                                     trigger->params[0], &t_enter, &t_exit)) {
                            event_time = t_exit * dt;
                            contact_point = UpeVec3_Add(prev_pos, UpeVec3_Scale(UpeVec3_Sub(body->position, prev_pos), t_exit));
                        }
                    } else if (trigger->type == UPE_TRIGGER_BOX) {
                        UpeQuat inv_rot = {-trigger->rotation.x, -trigger->rotation.y, -trigger->rotation.z, trigger->rotation.w};
                        UpeVec3 local_prev = UpeQuat_RotateVec3(inv_rot, UpeVec3_Sub(prev_pos, trigger->position));
                        UpeVec3 local_curr = UpeQuat_RotateVec3(inv_rot, UpeVec3_Sub(body->position, trigger->position));
                        UpeVec3 half_extents = {trigger->params[0], trigger->params[1], trigger->params[2]};
                        if (UpeTrigger_SegmentBoxLocal(local_prev, local_curr, half_extents, &t_enter, &t_exit)) {
                            event_time = t_exit * dt;
                            contact_point = UpeVec3_Add(prev_pos, UpeVec3_Scale(UpeVec3_Sub(body->position, prev_pos), t_exit));
                        }
                    }
                }
                // EXIT event
                UpeTrigger_AddEvent(sys, trigger->id, body->id, UPE_TRIGGER_EXIT,
                                   contact_point, body->velocity, event_time);
                
                // Remove from inside list
                for (uint32_t i = 0; i < trigger->body_count; ++i) {
                    if (trigger->bodies_inside[i] == body->id) {
                        trigger->bodies_inside[i] = trigger->bodies_inside[--trigger->body_count];
                        break;
                    }
                }
            } else if (is_inside && was_inside) {
                // STAY event (optional, could be throttled)
                // UpeTrigger_AddEvent(sys, trigger->id, body->id, UPE_TRIGGER_STAY, ...);
            }
        }
    }
}

static int UpeTriggerSystem_CompareEvents(const void *a, const void *b) {
    const UpeTriggerEventData *ea = (const UpeTriggerEventData*)a;
    const UpeTriggerEventData *eb = (const UpeTriggerEventData*)b;
    
    // Sort by frame, then time, then trigger ID (deterministic)
    if (ea->frame_number != eb->frame_number) {
        return (int)ea->frame_number - (int)eb->frame_number;
    }
    if (ea->event_time != eb->event_time) {
        return (ea->event_time < eb->event_time) ? -1 : 1;
    }
    if (ea->trigger_id != eb->trigger_id) {
        return (int)ea->trigger_id - (int)eb->trigger_id;
    }
    if (ea->body_id != eb->body_id) {
        return (int)ea->body_id - (int)eb->body_id;
    }
    // Ensure ENTER fires before EXIT for determinism (exit events should be processed last)
    // Priority: ENTER (0) < STAY (1) < EXIT (2)
    return (int)ea->event_type - (int)eb->event_type;
}

void UpeTriggerSystem_SortEvents(UpeTriggerSystem *sys) {
    if (!sys || sys->event_count == 0) {
        return;
    }
    
    qsort(sys->event_queue, sys->event_count, sizeof(UpeTriggerEventData),
          UpeTriggerSystem_CompareEvents);
}

void UpeTriggerSystem_ProcessEvents(UpeTriggerSystem *sys) {
    if (!sys) {
        return;
    }
    
    // Sort events for determinism
    UpeTriggerSystem_SortEvents(sys);
    
    // Invoke callbacks
    for (uint32_t i = 0; i < sys->event_count; ++i) {
        UpeTriggerEventData *event = &sys->event_queue[i];
        
        for (uint32_t c = 0; c < sys->callback_count; ++c) {
            if (sys->callbacks[c]) {
                sys->callbacks[c](event, sys->triggers[event->trigger_id].user_data);
            }
        }
    }
}

void UpeTriggerSystem_ClearEvents(UpeTriggerSystem *sys) {
    if (sys) {
        sys->event_count = 0;
    }
}

const UpeTrigger* UpeTriggerSystem_Get(const UpeTriggerSystem *sys, uint32_t trigger_id) {
    if (!sys || trigger_id >= sys->trigger_count) {
        return NULL;
    }
    return &sys->triggers[trigger_id];
}

uint32_t UpeTriggerSystem_FindByName(const UpeTriggerSystem *sys, const char *name) {
    if (!sys || !name) {
        return 0xFFFFFFFF;
    }
    
    for (uint32_t i = 0; i < sys->trigger_count; ++i) {
        if (strcmp(sys->triggers[i].name, name) == 0) {
            return i;
        }
    }
    
    return 0xFFFFFFFF;
}

void UpeTriggerSystem_PrintStats(const UpeTriggerSystem *sys) {
    if (!sys) {
        return;
    }
    
    printf("=== Trigger System Stats ===\n");
    printf("Trigger Count: %u / %u\n", sys->trigger_count, sys->trigger_capacity);
    printf("Event Count:   %u / %u\n", sys->event_count, sys->event_capacity);
    printf("Callbacks:     %u\n", sys->callback_count);
    
    for (uint32_t i = 0; i < sys->trigger_count; ++i) {
        const UpeTrigger *t = &sys->triggers[i];
        printf("  [%u] %s: %u bodies inside, %s\n",
               t->id, t->name, t->body_count, t->active ? "active" : "inactive");
    }
    
    printf("============================\n");
}

void UpeTrigger_Update(UpeTriggerSystem *sys, void *world) {
    if (!sys || !world) {
        return;
    }

    UpeTriggerSystem_ClearEvents(sys);
    UpeTriggerSystem_Update(sys, world);
    UpeTriggerSystem_SortEvents(sys);
}
