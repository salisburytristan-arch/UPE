#include "upe/upe_determinism.h"
#include "upe/upe_hash.h"
#include "upe/upe_body.h"
#include "upe/upe_shape.h"
#include "upe/upe_math.h"
#include "upe/upe_api.h"
#include "../world/upe_world.h"
#include <stdlib.h>
#include <string.h>

static void hash_u32(UpeHash64 *hash, uint32_t value) {
    UpeHash64_UpdateU32(hash, value);
}

static void hash_f32(UpeHash64 *hash, float value) {
    UpeHash64_UpdateF32(hash, value);
}

static void hash_vec3(UpeHash64 *hash, UpeVec3 v) {
    hash_f32(hash, v.x);
    hash_f32(hash, v.y);
    hash_f32(hash, v.z);
}

static void hash_quat(UpeHash64 *hash, UpeQuat q) {
    hash_f32(hash, q.x);
    hash_f32(hash, q.y);
    hash_f32(hash, q.z);
    hash_f32(hash, q.w);
}

static void hash_shape(UpeHash64 *hash, const UpeShape *shape) {
    hash_u32(hash, shape->id);
    hash_u32(hash, (uint32_t)shape->type);
    hash_u32(hash, shape->body_id);
    hash_u32(hash, shape->collision_layer);
    hash_u32(hash, shape->collision_mask);
    hash_f32(hash, shape->friction);
    hash_f32(hash, shape->restitution);
    hash_u32(hash, shape->next_shape_index);

    switch (shape->type) {
        case UPE_SHAPE_SPHERE:
            hash_vec3(hash, shape->sphere.center);
            hash_f32(hash, shape->sphere.radius);
            break;
        case UPE_SHAPE_CAPSULE:
            hash_vec3(hash, shape->capsule.a);
            hash_vec3(hash, shape->capsule.b);
            hash_f32(hash, shape->capsule.radius);
            break;
        case UPE_SHAPE_BOX:
            hash_vec3(hash, shape->box.min);
            hash_vec3(hash, shape->box.max);
            break;
        case UPE_SHAPE_OBB:
            hash_vec3(hash, shape->obb.center);
            hash_vec3(hash, shape->obb.half_extents);
            hash_quat(hash, shape->obb.rotation);
            for (int i = 0; i < 3; ++i) {
                hash_vec3(hash, shape->obb.axes[i]);
            }
            break;
        case UPE_SHAPE_MESH:
        default:
            break;
    }
}

static UpeHash64 hash_world_state(const UpeWorld *world, uint64_t seed) {
    UpeHash64 hash;
    UpeHash64_Init(&hash, seed);

    if (!world) {
        return hash;
    }

    hash_f32(&hash, world->time);
    hash_f32(&hash, world->fixed_dt);
    hash_vec3(&hash, world->gravity);

    hash_u32(&hash, world->body_count);
    hash_u32(&hash, world->shape_count);

    if (world->body_alive && world->bodies) {
        for (uint32_t i = 0; i < world->body_capacity; ++i) {
            if (!world->body_alive[i]) {
                continue;
            }
            const UpeBody *body = &world->bodies[i];
            hash_u32(&hash, body->id);
            hash_u32(&hash, (uint32_t)body->type);
            hash_vec3(&hash, body->position);
            hash_vec3(&hash, body->velocity);
            hash_quat(&hash, body->rotation);
            hash_vec3(&hash, body->angular_velocity);
            hash_f32(&hash, body->mass);
            hash_f32(&hash, body->inv_mass);
            hash_vec3(&hash, body->inv_inertia);
            hash_vec3(&hash, body->force);
            hash_vec3(&hash, body->torque);
            hash_u32(&hash, (uint32_t)body->awake);
            hash_u32(&hash, body->collision_layer);
            hash_u32(&hash, body->collision_mask);
            hash_u32(&hash, body->first_shape_index);
        }
    }

    if (world->shape_alive && world->shapes) {
        for (uint32_t i = 0; i < world->shape_capacity; ++i) {
            if (!world->shape_alive[i]) {
                continue;
            }
            hash_shape(&hash, &world->shapes[i]);
        }
    }

    return hash;
}

void UpeDeterminism_SetFpMode(UpeFpMode mode) {
    (void)mode;
}

void UpeDeterminism_FlushDenormals(int enable) {
    (void)enable;
}

UpeStateHash UpeDeterminism_HashState(const void *world) {
    const UpeWorld *w = (const UpeWorld *)world;

    UpeHash64 h = hash_world_state(w, 0xA5A5A5A5u);

    UpeStateHash out;
    out.state = UpeHash64_Final(&h);
    return out;
}

int UpeDeterminism_CompareHash(UpeStateHash a, UpeStateHash b) {
    return (a.state == b.state) ? 1 : 0;
}

void UpeRng_Init(UpeDeterministicRng *rng, uint64_t seed) {
    if (!rng) {
        return;
    }
    rng->state = seed ? seed : 0x853C49E6748FEA9BULL;
    rng->increment = 0x9E3779B97F4A7C15ULL;
}

uint32_t UpeRng_Next(UpeDeterministicRng *rng) {
    if (!rng) {
        return 0;
    }
    rng->state = rng->state * 6364136223846793005ULL + rng->increment;
    return (uint32_t)(rng->state >> 32);
}

float UpeRng_Float(UpeDeterministicRng *rng) {
    uint32_t bits = UpeRng_Next(rng) >> 8;
    return (float)bits * (1.0f / 16777216.0f);
}

float UpeRng_FloatRange(UpeDeterministicRng *rng, float min, float max) {
    float t = UpeRng_Float(rng);
    return min + (max - min) * t;
}

void UpeFrameClock_Init(UpeFrameClock *clock, float fixed_dt) {
    if (!clock) {
        return;
    }
    clock->frame = 0;
    clock->fixed_dt = fixed_dt;
    clock->accumulated_time = 0.0;
}

void UpeFrameClock_Advance(UpeFrameClock *clock) {
    if (!clock) {
        return;
    }
    clock->frame += 1;
    clock->accumulated_time += clock->fixed_dt;
}

uint64_t UpeFrameClock_GetFrame(const UpeFrameClock *clock) {
    return clock ? clock->frame : 0;
}

float UpeFrameClock_GetInterpolation(const UpeFrameClock *clock) {
    if (!clock || clock->fixed_dt <= 0.0f) {
        return 0.0f;
    }
    double fractional = clock->accumulated_time / (double)clock->fixed_dt;
    double frac = fractional - (double)((uint64_t)fractional);
    return (float)frac;
}

void UpeReplay_Init(UpeReplayRecorder *replay, uint32_t max_events) {
    if (!replay) {
        return;
    }
    replay->events = (UpeEvent *)malloc(max_events * sizeof(UpeEvent));
    replay->event_count = 0;
    replay->event_capacity = max_events;
    replay->recording = 0;
}

void UpeReplay_Destroy(UpeReplayRecorder *replay) {
    if (!replay) {
        return;
    }
    free(replay->events);
    replay->events = NULL;
    replay->event_count = 0;
    replay->event_capacity = 0;
    replay->recording = 0;
}

void UpeReplay_StartRecording(UpeReplayRecorder *replay) {
    if (!replay) {
        return;
    }
    replay->recording = 1;
    replay->event_count = 0;
}

void UpeReplay_StopRecording(UpeReplayRecorder *replay) {
    if (!replay) {
        return;
    }
    replay->recording = 0;
}

void UpeReplay_RecordEvent(UpeReplayRecorder *replay, const UpeEvent *event) {
    if (!replay || !event || !replay->recording) {
        return;
    }
    if (replay->event_count >= replay->event_capacity) {
        return;
    }
    replay->events[replay->event_count++] = *event;
}

const UpeEvent* UpeReplay_GetEvent(const UpeReplayRecorder *replay, uint32_t index) {
    if (!replay || index >= replay->event_count) {
        return NULL;
    }
    return &replay->events[index];
}

static void UpeReplay_RecordEventInternal(UpeReplayRecorder *replay, uint64_t frame, UpeEventType type,
                                          uint32_t entity_id, const float *data, uint32_t data_count) {
    if (!replay || !replay->recording) {
        return;
    }
    UpeEvent evt = {};
    evt.frame = frame;
    evt.type = type;
    evt.entity_id = entity_id;
    uint32_t count = data_count > 8 ? 8 : data_count;
    for (uint32_t i = 0; i < count; ++i) {
        evt.data[i] = data[i];
    }
    UpeReplay_RecordEvent(replay, &evt);
}

void UpeReplay_RecordInput(UpeReplayRecorder *replay, uint64_t frame, uint32_t entity_id,
                           const float *data, uint32_t data_count) {
    UpeReplay_RecordEventInternal(replay, frame, UPE_EVENT_INPUT, entity_id, data, data_count);
}

void UpeReplay_RecordSpawn(UpeReplayRecorder *replay, uint64_t frame, uint32_t entity_id,
                           const float *data, uint32_t data_count) {
    UpeReplay_RecordEventInternal(replay, frame, UPE_EVENT_SPAWN, entity_id, data, data_count);
}

void UpeReplay_RecordDestroy(UpeReplayRecorder *replay, uint64_t frame, uint32_t entity_id) {
    UpeReplay_RecordEventInternal(replay, frame, UPE_EVENT_DESTROY, entity_id, NULL, 0);
}

void UpeReplay_RecordImpulse(UpeReplayRecorder *replay, uint64_t frame, uint32_t entity_id,
                             UpeVec3 impulse) {
    float data[3] = {impulse.x, impulse.y, impulse.z};
    UpeReplay_RecordEventInternal(replay, frame, UPE_EVENT_IMPULSE, entity_id, data, 3);
}

void UpeReplay_ApplyToWorld(UpeWorld *world, const UpeReplayRecorder *replay, uint64_t frame) {
    if (!world || !replay) {
        return;
    }

    for (uint32_t i = 0; i < replay->event_count; ++i) {
        const UpeEvent *evt = &replay->events[i];
        if (evt->frame != frame) {
            continue;
        }

        switch (evt->type) {
            case UPE_EVENT_SPAWN: {
                UpeBodyDesc desc = {};
                desc.type = UPE_BODY_DYNAMIC;
                desc.position = {evt->data[0], evt->data[1], evt->data[2]};
                desc.rotation = {evt->data[3], evt->data[4], evt->data[5], evt->data[6]};
                desc.mass = (evt->data[7] > 0.0f) ? evt->data[7] : 1.0f;
                desc.collision_layer = 1u;
                desc.collision_mask = 0xFFFFFFFFu;
                UpeWorld_CreateBody(world, &desc);
                break;
            }
            case UPE_EVENT_DESTROY:
                UpeWorld_DestroyBody(world, evt->entity_id);
                break;
            case UPE_EVENT_INPUT: {
                float mode = evt->data[3];
                if (mode >= 0.5f) {
                    UpeWorld_ApplyTorque(world, evt->entity_id, {evt->data[0], evt->data[1], evt->data[2]});
                } else {
                    UpeWorld_ApplyForce(world, evt->entity_id, {evt->data[0], evt->data[1], evt->data[2]});
                }
                break;
            }
            case UPE_EVENT_IMPULSE:
                UpeWorld_ApplyImpulse(world, evt->entity_id, {evt->data[0], evt->data[1], evt->data[2]});
                break;
            default:
                break;
        }
    }
}

void UpeReplay_ApplyToWorld2D(Upe2DWorld *world, const UpeReplayRecorder *replay, uint64_t frame) {
    if (!world || !replay) {
        return;
    }

    for (uint32_t i = 0; i < replay->event_count; ++i) {
        const UpeEvent *evt = &replay->events[i];
        if (evt->frame != frame) {
            continue;
        }

        switch (evt->type) {
            case UPE_EVENT_SPAWN: {
                Upe2DBodyType type = (Upe2DBodyType)(int)(evt->data[3] + 0.5f);
                float mass = (evt->data[4] > 0.0f) ? evt->data[4] : 1.0f;
                Upe2DWorld_CreateBody(world, type, {evt->data[0], evt->data[1]}, evt->data[2], mass);
                break;
            }
            case UPE_EVENT_DESTROY:
                Upe2DWorld_DestroyBody(world, evt->entity_id);
                break;
            case UPE_EVENT_INPUT: {
                int mode = (int)(evt->data[3] + 0.5f);
                if (mode == 1) {
                    Upe2DWorld_ApplyTorque(world, evt->entity_id, evt->data[0]);
                } else if (mode == 2) {
                    Upe2DWorld_ApplyImpulse(world, evt->entity_id, {evt->data[0], evt->data[1]});
                } else {
                    Upe2DWorld_ApplyForce(world, evt->entity_id, {evt->data[0], evt->data[1]});
                }
                break;
            }
            default:
                break;
        }
    }
}

int UpeDeterminism_VerifyPlatform(void) {
    uint32_t value = 0x01020304u;
    uint8_t *bytes = (uint8_t *)&value;
    int little_endian = (bytes[0] == 0x04);
    return little_endian ? 1 : 0;
}
