#include "upe/upe_gravity_fields.h"
#include "upe_world.h"
#include <stdlib.h>
#include <string.h>

static int UpeWorld_EnsureGravityFieldCapacity(UpeWorld *world, uint32_t required) {
    if (!world) {
        return 0;
    }
    if (required == 0) {
        required = 1;
    }
    if (world->gravity_field_capacity >= required && world->gravity_fields) {
        return 1;
    }

    uint32_t new_capacity = (world->gravity_field_capacity == 0) ? 8u : world->gravity_field_capacity;
    while (new_capacity < required) {
        if (new_capacity > 0x7FFFFFFFu) {
            return 0;
        }
        new_capacity *= 2u;
    }

    UpeGravityFieldState *new_fields = (UpeGravityFieldState*)realloc(
        world->gravity_fields, new_capacity * sizeof(UpeGravityFieldState));
    if (!new_fields) {
        return 0;
    }

    world->gravity_fields = new_fields;
    world->gravity_field_capacity = new_capacity;
    return 1;
}

UpeGravityFieldDesc UpeGravityFieldDesc_Default(void) {
    UpeGravityFieldDesc desc = {};
    desc.position = {0.0f, 0.0f, 0.0f};
    desc.strength = 9.81f;
    desc.min_distance = 0.5f;
    desc.max_distance = 0.0f;
    desc.falloff_exponent = 2.0f;
    desc.enabled = 1;
    desc.type = UPE_GRAVITY_FIELD_POINT_ATTRACT;
    return desc;
}

uint32_t UpeWorld_AddGravityField(UpeWorld *world, const UpeGravityFieldDesc *desc) {
    if (!world || !desc) {
        return 0;
    }

    if (!UpeWorld_EnsureGravityFieldCapacity(world, world->gravity_field_count + 1u)) {
        return 0;
    }

    uint32_t id = world->gravity_field_next_id;
    if (id == 0) {
        id = 1;
    }
    world->gravity_field_next_id = id + 1u;

    UpeGravityFieldState *field = &world->gravity_fields[world->gravity_field_count++];
    field->id = id;
    field->position = desc->position;
    field->strength = desc->strength;
    field->min_distance = desc->min_distance;
    field->max_distance = desc->max_distance;
    field->falloff_exponent = desc->falloff_exponent;
    field->enabled = (desc->enabled != 0) ? 1 : 0;
    field->type = (int)desc->type;

    return id;
}

int UpeWorld_RemoveGravityField(UpeWorld *world, uint32_t field_id) {
    if (!world || field_id == 0 || world->gravity_field_count == 0) {
        return 0;
    }

    for (uint32_t i = 0; i < world->gravity_field_count; ++i) {
        if (world->gravity_fields[i].id == field_id) {
            uint32_t move_count = world->gravity_field_count - i - 1u;
            if (move_count > 0) {
                memmove(&world->gravity_fields[i],
                        &world->gravity_fields[i + 1u],
                        move_count * sizeof(UpeGravityFieldState));
            }
            world->gravity_field_count -= 1u;
            return 1;
        }
    }

    return 0;
}

void UpeWorld_ClearGravityFields(UpeWorld *world) {
    if (!world) {
        return;
    }
    world->gravity_field_count = 0;
}

uint32_t UpeWorld_GetGravityFieldCount(const UpeWorld *world) {
    if (!world) {
        return 0;
    }
    return world->gravity_field_count;
}

uint32_t UpeWorld_GetGravityFields(const UpeWorld *world, UpeGravityFieldInfo *out_fields, uint32_t max_fields) {
    if (!world || !out_fields || max_fields == 0) {
        return 0;
    }

    uint32_t count = world->gravity_field_count;
    if (count > max_fields) {
        count = max_fields;
    }

    for (uint32_t i = 0; i < count; ++i) {
        const UpeGravityFieldState *field = &world->gravity_fields[i];
        out_fields[i].id = field->id;
        out_fields[i].desc.position = field->position;
        out_fields[i].desc.strength = field->strength;
        out_fields[i].desc.min_distance = field->min_distance;
        out_fields[i].desc.max_distance = field->max_distance;
        out_fields[i].desc.falloff_exponent = field->falloff_exponent;
        out_fields[i].desc.enabled = field->enabled;
        out_fields[i].desc.type = (UpeGravityFieldType)field->type;
    }

    return count;
}
