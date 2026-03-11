#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "upe/upe_compound_shapes.h"
#include "upe/upe_math.h"
#include "upe/upe_shape.h"

// Create compound shape collection
UpeCompound *UpeCompound_Create(const UpeCompoundDesc *desc)
{
    if (!desc || desc->max_shapes == 0) {
        return NULL;
    }

    UpeCompound *compound = (UpeCompound *)malloc(sizeof(UpeCompound));
    if (!compound) return NULL;

    compound->id = (uint32_t)((uintptr_t)compound / sizeof(UpeCompound));
    compound->body_id = desc->body_id;
    compound->shape_count = 0;
    compound->max_shapes = desc->max_shapes;

    // Allocate shape array
    compound->shapes = (UpeCompoundShape *)calloc(desc->max_shapes, sizeof(UpeCompoundShape));
    if (!compound->shapes) {
        free(compound);
        return NULL;
    }

    // Initialize aggregate properties
    compound->aggregate_com.x = 0.0f;
    compound->aggregate_com.y = 0.0f;
    compound->aggregate_com.z = 0.0f;
    compound->aggregate_mass = 0.0f;
    compound->aggregate_inertia.x = 0.0f;
    compound->aggregate_inertia.y = 0.0f;
    compound->aggregate_inertia.z = 0.0f;

    return compound;
}

// Destroy compound
void UpeCompound_Destroy(UpeCompound *compound)
{
    if (!compound) return;

    if (compound->shapes) {
        free(compound->shapes);
        compound->shapes = NULL;
    }

    free(compound);
}

// Add shape to compound
uint32_t UpeCompound_AddShape(UpeCompound *compound, const UpeShape *shape,
                              UpeVec3 local_offset, UpeQuat local_rotation)
{
    if (!compound || !shape || compound->shape_count >= compound->max_shapes) {
        return (uint32_t)-1;
    }

    UpeCompoundShape *cs = &compound->shapes[compound->shape_count];
    cs->shape = *shape;
    cs->local_offset = local_offset;
    cs->local_rotation = local_rotation;
    cs->shape_id = compound->shape_count;

    uint32_t idx = compound->shape_count;
    compound->shape_count++;

    // Update aggregate properties after adding
    UpeCompound_UpdateAggregateProperties(compound);

    return idx;
}

// Remove shape from compound
void UpeCompound_RemoveShape(UpeCompound *compound, uint32_t shape_idx)
{
    if (!compound || shape_idx >= compound->shape_count) {
        return;
    }

    // Swap with last and shrink
    if (shape_idx < compound->shape_count - 1) {
        compound->shapes[shape_idx] = compound->shapes[compound->shape_count - 1];
        compound->shapes[shape_idx].shape_id = shape_idx;
    }
    compound->shape_count--;

    // Update aggregate properties after removing
    UpeCompound_UpdateAggregateProperties(compound);
}

// Get shape from compound
UpeCompoundShape *UpeCompound_GetShape(UpeCompound *compound, uint32_t shape_idx)
{
    if (!compound || shape_idx >= compound->shape_count) {
        return NULL;
    }

    return &compound->shapes[shape_idx];
}

// Recalculate aggregate properties (COM, inertia)
void UpeCompound_UpdateAggregateProperties(UpeCompound *compound)
{
    if (!compound || compound->shape_count == 0) {
        compound->aggregate_mass = 0.0f;
        compound->aggregate_com.x = 0.0f;
        compound->aggregate_com.y = 0.0f;
        compound->aggregate_com.z = 0.0f;
        compound->aggregate_inertia.x = 0.0f;
        compound->aggregate_inertia.y = 0.0f;
        compound->aggregate_inertia.z = 0.0f;
        return;
    }

    // Compute weighted center of mass and total mass
    float total_mass = 0.0f;
    UpeVec3 weighted_com = {0.0f, 0.0f, 0.0f};

    for (uint32_t i = 0; i < compound->shape_count; i++) {
        UpeCompoundShape *cs = &compound->shapes[i];
        // Default mass of 1.0 per shape if not set elsewhere
        float mass = 1.0f;
        
        weighted_com.x += cs->local_offset.x * mass;
        weighted_com.y += cs->local_offset.y * mass;
        weighted_com.z += cs->local_offset.z * mass;
        total_mass += mass;
    }

    if (total_mass > 0.0f) {
        weighted_com.x /= total_mass;
        weighted_com.y /= total_mass;
        weighted_com.z /= total_mass;
    }

    compound->aggregate_mass = total_mass;
    compound->aggregate_com = weighted_com;

    // Compute aggregate inertia using parallel axis theorem
    // I_aggregate = sum(I_individual + m * d²)
    // where d = distance from shape COM to aggregate COM
    UpeVec3 aggregate_inertia = {0.0f, 0.0f, 0.0f};

    for (uint32_t i = 0; i < compound->shape_count; i++) {
        UpeCompoundShape *cs = &compound->shapes[i];
        float mass = 1.0f;

        // Distance from shape position to aggregate COM
        UpeVec3 d = UpeVec3_Sub(cs->local_offset, weighted_com);
        float dist_sq = UpeVec3_Dot(d, d);

        // Parallel axis: add m * d²
        float offset_inertia = mass * dist_sq;

        // Approximate shape inertia (simplified: treat as sphere/box)
        // For now, use mass * radius² as rough approximation
        float shape_inertia = mass * 0.25f;  // Assumes unit-scale shapes

        aggregate_inertia.x += shape_inertia + offset_inertia;
        aggregate_inertia.y += shape_inertia + offset_inertia;
        aggregate_inertia.z += shape_inertia + offset_inertia;
    }

    compound->aggregate_inertia = aggregate_inertia;
}

// Get world-space position of a shape within compound
UpeVec3 UpeCompound_GetShapeWorldPosition(UpeCompound *compound, uint32_t shape_idx,
                                         UpeVec3 body_position, UpeQuat body_rotation)
{
    if (!compound || shape_idx >= compound->shape_count) {
        return body_position;
    }

    UpeCompoundShape *cs = &compound->shapes[shape_idx];

    // Transform local offset to world space
    // p_world = body_pos + body_rot * local_offset
    UpeVec3 rotated_offset = UpeQuat_RotateVec3(body_rotation, cs->local_offset);
    return UpeVec3_Add(body_position, rotated_offset);
}

// Get world-space rotation of a shape within compound
UpeQuat UpeCompound_GetShapeWorldRotation(UpeCompound *compound, uint32_t shape_idx,
                                        UpeQuat body_rotation)
{
    if (!compound || shape_idx >= compound->shape_count) {
        return body_rotation;
    }

    UpeCompoundShape *cs = &compound->shapes[shape_idx];

    // Combine body rotation with local shape rotation
    // q_world = body_rot * local_rot
    return UpeQuat_Mul(body_rotation, cs->local_rotation);
}

