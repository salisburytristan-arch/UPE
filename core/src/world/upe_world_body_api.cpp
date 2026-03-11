#include "upe/upe_world_body_api.h"
#include "upe/upe_shape.h"
#include "upe/upe_islands.h"
#include "upe_world.h"
#include "upe/upe_determinism.h"
#include <stdlib.h>
#include <string.h>

static const uint32_t UPE_MAX_PACKED_HANDLE_INDEX = 0xFFFFu;
static const uint32_t UPE_INVALID_SHAPE_INDEX = 0xFFFFFFFFu;

static int UpeWorldEnsureDenseListCapacity(uint32_t **ids, uint32_t *capacity, uint32_t required) {
    if (!ids || !capacity) {
        return 0;
    }
    if (required == 0) {
        required = 1;
    }
    if (*capacity >= required && *ids) {
        return 1;
    }
    uint32_t new_capacity = (*capacity == 0) ? 16u : *capacity;
    while (new_capacity < required) {
        if (new_capacity > 0x7FFFFFFFu) {
            return 0;
        }
        new_capacity *= 2u;
    }
    uint32_t *new_ids = (uint32_t*)realloc(*ids, new_capacity * sizeof(uint32_t));
    if (!new_ids) {
        return 0;
    }
    *ids = new_ids;
    *capacity = new_capacity;
    return 1;
}

static int UpeWorldDenseListAdd(uint32_t **ids, uint32_t *count, uint32_t *capacity, uint32_t value) {
    if (!ids || !count || !capacity) {
        return 0;
    }
    if (!UpeWorldEnsureDenseListCapacity(ids, capacity, *count + 1u)) {
        return 0;
    }
    (*ids)[*count] = value;
    *count += 1u;
    return 1;
}

static void UpeWorldDenseListRemove(uint32_t *ids, uint32_t *count, uint32_t value) {
    if (!ids || !count || *count == 0) {
        return;
    }
    for (uint32_t i = 0; i < *count; ++i) {
        if (ids[i] == value) {
            ids[i] = ids[*count - 1u];
            *count -= 1u;
            return;
        }
    }
}

static uint32_t UpePackBodyHandle(uint32_t index, uint16_t generation) {
    return ((uint32_t)generation << 16) | (index & 0xFFFFu);
}

static uint32_t UpeBodyHandleIndex(uint32_t handle) {
    return handle & 0xFFFFu;
}

static uint16_t UpeBodyHandleGeneration(uint32_t handle) {
    return (uint16_t)(handle >> 16);
}

static uint32_t UpePackShapeHandle(uint32_t index, uint16_t generation) {
    return ((uint32_t)generation << 16) | (index & 0xFFFFu);
}

static uint32_t UpeShapeHandleIndex(uint32_t handle) {
    return handle & 0xFFFFu;
}

static uint16_t UpeShapeHandleGeneration(uint32_t handle) {
    return (uint16_t)(handle >> 16);
}

void UpeWorld_DestroyShape(UpeWorld *world, uint32_t shape_id);

static int UpeWorldGrowBodyStorage(UpeWorld *world, uint32_t target_capacity) {
    if (target_capacity == 0 || target_capacity > (UPE_MAX_PACKED_HANDLE_INDEX + 1u)) {
        return 0;
    }
    uint32_t new_capacity = world->body_capacity == 0 ? 16u : world->body_capacity;
    while (new_capacity < target_capacity) {
        if (new_capacity > UPE_MAX_PACKED_HANDLE_INDEX / 2u) {
            new_capacity = UPE_MAX_PACKED_HANDLE_INDEX + 1u;
            break;
        }
        new_capacity *= 2u;
    }

    UpeBody *new_bodies = (UpeBody *)realloc(world->bodies, new_capacity * sizeof(UpeBody));
    if (!new_bodies) {
        return 0;
    }
    world->bodies = new_bodies;

    uint16_t *new_generations = (uint16_t *)realloc(world->body_generations, new_capacity * sizeof(uint16_t));
    if (!new_generations) {
        return 0;
    }
    world->body_generations = new_generations;

    uint8_t *new_alive = (uint8_t *)realloc(world->body_alive, new_capacity * sizeof(uint8_t));
    if (!new_alive) {
        return 0;
    }
    world->body_alive = new_alive;

    if (new_capacity > world->body_capacity) {
        memset(world->body_alive + world->body_capacity, 0, (new_capacity - world->body_capacity) * sizeof(uint8_t));
        memset(world->body_generations + world->body_capacity, 0, (new_capacity - world->body_capacity) * sizeof(uint16_t));
    }
    world->body_capacity = new_capacity;

    if (world->body_island_ids) {
        uint32_t *new_island_ids = (uint32_t*)realloc(world->body_island_ids,
            new_capacity * sizeof(uint32_t));
        if (!new_island_ids) {
            return 0;
        }
        world->body_island_ids = new_island_ids;
    } else {
        world->body_island_ids = (uint32_t*)malloc(new_capacity * sizeof(uint32_t));
        if (!world->body_island_ids) {
            return 0;
        }
    }

    if (world->islands) {
        UpeIsland *new_islands = (UpeIsland*)realloc(world->islands,
            new_capacity * sizeof(UpeIsland));
        if (!new_islands) {
            return 0;
        }
        if (new_capacity > world->island_capacity) {
            for (uint32_t i = world->island_capacity; i < new_capacity; ++i) {
                UpeIsland_Init(&new_islands[i]);
            }
        }
        world->islands = new_islands;
    } else {
        world->islands = (UpeIsland*)malloc(new_capacity * sizeof(UpeIsland));
        if (!world->islands) {
            return 0;
        }
        for (uint32_t i = 0; i < new_capacity; ++i) {
            UpeIsland_Init(&world->islands[i]);
        }
    }
    world->island_capacity = new_capacity;

    if (world->island_constraint_offsets) {
        uint32_t *new_offsets = (uint32_t*)realloc(world->island_constraint_offsets,
            new_capacity * sizeof(uint32_t));
        if (!new_offsets) {
            return 0;
        }
        world->island_constraint_offsets = new_offsets;
    } else {
        world->island_constraint_offsets = (uint32_t*)malloc(new_capacity * sizeof(uint32_t));
        if (!world->island_constraint_offsets) {
            return 0;
        }
    }

    if (world->island_constraint_counts) {
        uint32_t *new_counts = (uint32_t*)realloc(world->island_constraint_counts,
            new_capacity * sizeof(uint32_t));
        if (!new_counts) {
            return 0;
        }
        world->island_constraint_counts = new_counts;
    } else {
        world->island_constraint_counts = (uint32_t*)malloc(new_capacity * sizeof(uint32_t));
        if (!world->island_constraint_counts) {
            return 0;
        }
    }

    memset(world->island_constraint_offsets, 0, new_capacity * sizeof(uint32_t));
    memset(world->island_constraint_counts, 0, new_capacity * sizeof(uint32_t));

    if (!UpeWorldEnsureDenseListCapacity(&world->alive_body_ids,
                                         &world->alive_body_capacity,
                                         new_capacity) ||
        !UpeWorldEnsureDenseListCapacity(&world->dynamic_body_ids,
                                         &world->dynamic_body_capacity,
                                         new_capacity)) {
        return 0;
    }
    return 1;
}

static int UpeWorldGrowShapeStorage(UpeWorld *world, uint32_t target_capacity) {
    if (target_capacity == 0 || target_capacity > (UPE_MAX_PACKED_HANDLE_INDEX + 1u)) {
        return 0;
    }
    uint32_t new_capacity = world->shape_capacity == 0 ? 16u : world->shape_capacity;
    while (new_capacity < target_capacity) {
        if (new_capacity > UPE_MAX_PACKED_HANDLE_INDEX / 2u) {
            new_capacity = UPE_MAX_PACKED_HANDLE_INDEX + 1u;
            break;
        }
        new_capacity *= 2u;
    }

    UpeShape *new_shapes = (UpeShape *)realloc(world->shapes, new_capacity * sizeof(UpeShape));
    if (!new_shapes) {
        return 0;
    }
    world->shapes = new_shapes;

    uint16_t *new_generations = (uint16_t *)realloc(world->shape_generations, new_capacity * sizeof(uint16_t));
    if (!new_generations) {
        return 0;
    }
    world->shape_generations = new_generations;

    uint8_t *new_alive = (uint8_t *)realloc(world->shape_alive, new_capacity * sizeof(uint8_t));
    if (!new_alive) {
        return 0;
    }
    world->shape_alive = new_alive;

    if (new_capacity > world->shape_capacity) {
        memset(world->shape_alive + world->shape_capacity, 0, (new_capacity - world->shape_capacity) * sizeof(uint8_t));
        memset(world->shape_generations + world->shape_capacity, 0, (new_capacity - world->shape_capacity) * sizeof(uint16_t));
    }
    world->shape_capacity = new_capacity;
    return 1;
}


// joint storage growth (mirrors body/shape mechanics)
static int UpeWorldGrowJointStorage(UpeWorld *world, uint32_t target_capacity) {
    if (!world) return 0;
    if (target_capacity == 0) {
        target_capacity = 1;
    }
    if (world->joint_capacity >= target_capacity && world->joints) {
        return 1;
    }
    uint32_t new_capacity = (world->joint_capacity == 0) ? 16u : world->joint_capacity;
    while (new_capacity < target_capacity) {
        if (new_capacity > 0x7FFFFFFFu) {
            return 0;
        }
        new_capacity *= 2u;
    }
    UpeJoint *new_joints = (UpeJoint*)realloc(world->joints, new_capacity * sizeof(UpeJoint));
    if (!new_joints) {
        return 0;
    }
    world->joints = new_joints;
    if (new_capacity > world->joint_capacity) {
        /* zero new slots */
        memset(world->joints + world->joint_capacity, 0, (new_capacity - world->joint_capacity) * sizeof(UpeJoint));
    }
    world->joint_capacity = new_capacity;
    return 1;
}

uint32_t UpeWorld_AddJoint(UpeWorld *world, const UpeJointDesc *desc) {
    if (!world || !desc) {
        return 0xFFFFFFFFu;
    }
    uint32_t slot = world->joint_count;
    if (slot >= world->joint_capacity) {
        if (!UpeWorldGrowJointStorage(world, slot + 1u)) {
            return 0xFFFFFFFFu;
        }
    }
    if (slot >= world->joint_capacity) {
        return 0xFFFFFFFFu;
    }
    UpeJoint_Init(&world->joints[slot], desc);
    world->joint_count++;
    return slot;
}

uint32_t UpeWorld_GetJointCount(UpeWorld *world) {
    return world ? world->joint_count : 0u;
}

uint32_t UpeWorld_CreateBody(UpeWorld *world, const UpeBodyDesc *desc) {
    if (!world || !desc) {
        return 0xFFFFFFFF;
    }

    uint32_t slot = world->body_capacity;
    for (uint32_t i = 0; i < world->body_capacity; ++i) {
        if (world->body_alive[i] == 0) {
            slot = i;
            break;
        }
    }

    if (slot == world->body_capacity) {
        if (!UpeWorldGrowBodyStorage(world, world->body_capacity + 1u)) {
            return 0xFFFFFFFF;
        }
    }

    if (slot == world->body_capacity) {
        return 0xFFFFFFFF;
    }

    if (slot > UPE_MAX_PACKED_HANDLE_INDEX) {
        return 0xFFFFFFFF;
    }

    uint16_t generation = world->body_generations[slot];
    uint32_t new_id = UpePackBodyHandle(slot, generation);
    UpeBody_Init(&world->bodies[slot], new_id, desc);
    world->body_alive[slot] = 1;
    world->body_count++;
    if (!UpeWorldDenseListAdd(&world->alive_body_ids,
                              &world->alive_body_count,
                              &world->alive_body_capacity,
                              slot)) {
        world->body_alive[slot] = 0;
        world->body_count--;
        memset(&world->bodies[slot], 0, sizeof(UpeBody));
        return 0xFFFFFFFF;
    }
    if (world->bodies[slot].type == UPE_BODY_DYNAMIC) {
        if (!UpeWorldDenseListAdd(&world->dynamic_body_ids,
                                  &world->dynamic_body_count,
                                  &world->dynamic_body_capacity,
                                  slot)) {
            UpeWorldDenseListRemove(world->alive_body_ids, &world->alive_body_count, slot);
            world->body_alive[slot] = 0;
            world->body_count--;
            memset(&world->bodies[slot], 0, sizeof(UpeBody));
            return 0xFFFFFFFF;
        }
    }
    world->query_bvh_dirty = 1;

    if (world->replay) {
        float data[8] = {
            desc->position.x,
            desc->position.y,
            desc->position.z,
            desc->rotation.x,
            desc->rotation.y,
            desc->rotation.z,
            desc->rotation.w,
            desc->mass
        };
        UpeReplay_RecordSpawn(world->replay, world->frame_index, new_id, data, 8);
    }
    return new_id;
}

void UpeWorld_DestroyBody(UpeWorld *world, uint32_t body_id) {
    if (!world) {
        return;
    }

    uint32_t index = UpeBodyHandleIndex(body_id);
    uint16_t generation = UpeBodyHandleGeneration(body_id);
    if (index >= world->body_capacity || !world->body_alive || world->body_alive[index] == 0) {
        return;
    }
    if (world->body_generations[index] != generation) {
        return;
    }

    uint32_t shape_index = world->bodies[index].first_shape_index;
    uint32_t guard = 0;
    while (shape_index != UPE_INVALID_SHAPE_INDEX && guard < world->shape_capacity) {
        if (shape_index >= world->shape_capacity || !world->shape_alive) {
            break;
        }
        uint32_t next_shape_index = world->shapes[shape_index].next_shape_index;
        if (world->shape_alive[shape_index]) {
            uint32_t attached_shape_id = world->shapes[shape_index].id;
            UpeWorld_DestroyShape(world, attached_shape_id);
        }
        shape_index = next_shape_index;
        guard++;
    }

    world->bodies[index].first_shape_index = UPE_INVALID_SHAPE_INDEX;

    UpeWorldDenseListRemove(world->alive_body_ids, &world->alive_body_count, index);
    UpeWorldDenseListRemove(world->dynamic_body_ids, &world->dynamic_body_count, index);

    world->body_alive[index] = 0;
    world->body_generations[index] = (uint16_t)(world->body_generations[index] + 1u);
    memset(&world->bodies[index], 0, sizeof(UpeBody));
    if (world->body_count > 0) {
        world->body_count--;
    }
    world->query_bvh_dirty = 1;

    if (world->replay) {
        UpeReplay_RecordDestroy(world->replay, world->frame_index, body_id);
    }
}

UpeBody *UpeWorld_GetBody(UpeWorld *world, uint32_t body_id) {
    if (!world) {
        return nullptr;
    }

    uint32_t index = UpeBodyHandleIndex(body_id);
    uint16_t generation = UpeBodyHandleGeneration(body_id);
    if (index >= world->body_capacity || !world->body_alive || world->body_alive[index] == 0) {
        return nullptr;
    }
    if (world->body_generations[index] != generation) {
        return nullptr;
    }
    return &world->bodies[index];
}

uint32_t UpeWorld_CreateShape(UpeWorld *world, const UpeShapeDesc *desc) {
    if (!world || !desc) {
        return 0xFFFFFFFF;
    }

    uint32_t body_index = UpeBodyHandleIndex(desc->body_id);
    uint16_t body_generation = UpeBodyHandleGeneration(desc->body_id);
    if (body_index >= world->body_capacity || !world->body_alive || world->body_alive[body_index] == 0) {
        return 0xFFFFFFFF;
    }
    if (world->body_generations[body_index] != body_generation) {
        return 0xFFFFFFFF;
    }

    uint32_t slot = world->shape_capacity;
    for (uint32_t i = 0; i < world->shape_capacity; ++i) {
        if (world->shape_alive[i] == 0) {
            slot = i;
            break;
        }
    }

    if (slot == world->shape_capacity) {
        if (!UpeWorldGrowShapeStorage(world, world->shape_capacity + 1u)) {
            return 0xFFFFFFFF;
        }
    }

    if (slot == world->shape_capacity) {
        return 0xFFFFFFFF;
    }

    if (slot > UPE_MAX_PACKED_HANDLE_INDEX) {
        return 0xFFFFFFFF;
    }

    uint16_t generation = world->shape_generations[slot];
    uint32_t shape_id = UpePackShapeHandle(slot, generation);
    UpeShape_Init(&world->shapes[slot], shape_id, desc);
    world->shape_alive[slot] = 1;
    world->shape_count++;
    world->query_bvh_dirty = 1;
    
    /* Link shape into body's shape list */
    UpeBody *body = &world->bodies[body_index];
    world->shapes[slot].next_shape_index = body->first_shape_index;
    body->first_shape_index = slot;
    
    return shape_id;
}

void UpeWorld_DestroyShape(UpeWorld *world, uint32_t shape_id) {
    if (!world) {
        return;
    }

    uint32_t index = UpeShapeHandleIndex(shape_id);
    uint16_t generation = UpeShapeHandleGeneration(shape_id);
    if (index >= world->shape_capacity || !world->shape_alive || world->shape_alive[index] == 0) {
        return;
    }
    if (world->shape_generations[index] != generation) {
        return;
    }

    /* Unlink shape from body's shape list */
    uint32_t body_id = world->shapes[index].body_id;
    uint32_t body_index = UpeBodyHandleIndex(body_id);
    uint16_t body_generation = UpeBodyHandleGeneration(body_id);
    if (body_index < world->body_capacity &&
        world->body_alive && world->body_alive[body_index] &&
        world->body_generations && world->body_generations[body_index] == body_generation) {
        UpeBody *body = &world->bodies[body_index];
        if (body->first_shape_index == index) {
            /* Shape is head of list */
            body->first_shape_index = world->shapes[index].next_shape_index;
        } else {
            /* Find previous shape in list */
            uint32_t prev = body->first_shape_index;
            uint32_t guard = 0;
            while (prev != UPE_INVALID_SHAPE_INDEX && guard < world->shape_capacity) {
                if (prev >= world->shape_capacity || !world->shape_alive || world->shape_alive[prev] == 0) {
                    prev = UPE_INVALID_SHAPE_INDEX;
                    break;
                }
                if (world->shapes[prev].next_shape_index == index) {
                    break;
                }
                prev = world->shapes[prev].next_shape_index;
                guard++;
            }
            if (prev != UPE_INVALID_SHAPE_INDEX && prev < world->shape_capacity) {
                world->shapes[prev].next_shape_index = world->shapes[index].next_shape_index;
            }
        }
    }

    world->shape_alive[index] = 0;
    world->shape_generations[index] = (uint16_t)(world->shape_generations[index] + 1u);
    memset(&world->shapes[index], 0, sizeof(UpeShape));
    if (world->shape_count > 0) {
        world->shape_count--;
    }
    world->query_bvh_dirty = 1;
}

void UpeWorld_SetReplayRecorder(UpeWorld *world, UpeReplayRecorder *replay) {
    if (!world) {
        return;
    }
    world->replay = replay;
}

void UpeWorld_SetProfiler(UpeWorld *world, UpeProfiler *profiler) {
    if (!world) {
        return;
    }
    world->profiler = profiler;
}

void UpeWorld_ApplyForce(UpeWorld *world, uint32_t body_id, UpeVec3 force) {
    if (!world) {
        return;
    }
    UpeBody *body = UpeWorld_GetBody(world, body_id);
    if (!body) {
        return;
    }
    UpeBody_ApplyForce(body, force);

    if (world->replay) {
        float data[4] = {force.x, force.y, force.z, 0.0f};
        UpeReplay_RecordInput(world->replay, world->frame_index, body_id, data, 4);
    }
}

void UpeWorld_ApplyTorque(UpeWorld *world, uint32_t body_id, UpeVec3 torque) {
    if (!world) {
        return;
    }
    UpeBody *body = UpeWorld_GetBody(world, body_id);
    if (!body) {
        return;
    }
    UpeBody_ApplyTorque(body, torque);

    if (world->replay) {
        float data[4] = {torque.x, torque.y, torque.z, 1.0f};
        UpeReplay_RecordInput(world->replay, world->frame_index, body_id, data, 4);
    }
}

void UpeWorld_ApplyImpulse(UpeWorld *world, uint32_t body_id, UpeVec3 impulse) {
    if (!world) {
        return;
    }
    UpeBody *body = UpeWorld_GetBody(world, body_id);
    if (!body) {
        return;
    }
    UpeBody_ApplyImpulse(body, impulse);

    if (world->replay) {
        UpeReplay_RecordImpulse(world->replay, world->frame_index, body_id, impulse);
    }
}
