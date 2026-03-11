#pragma once

#include <stdint.h>
#include "upe/upe_shape.h"
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

// Compound shape: multiple primitive shapes attached to one body
// Useful for complex objects, characters, vehicles
typedef struct UpeCompoundShape {
    UpeShape shape;              // Base shape data
    UpeVec3 local_offset;        // Position offset in body space
    UpeQuat local_rotation;      // Rotation offset in body space
    uint32_t shape_id;           // Unique ID within compound
} UpeCompoundShape;

typedef struct UpeCompound {
    uint32_t id;                 // Compound body ID
    uint32_t body_id;            // Associated rigid body
    
    UpeCompoundShape *shapes;    // Array of shapes
    uint32_t shape_count;        // Number of shapes
    uint32_t max_shapes;         // Allocated capacity
    
    // Cached aggregate properties
    UpeVec3 aggregate_com;       // Center of mass (combined)
    float aggregate_mass;        // Total mass
    UpeVec3 aggregate_inertia;   // Combined inertia diagonal
} UpeCompound;

typedef struct UpeCompoundDesc {
    uint32_t body_id;
    uint32_t max_shapes;
} UpeCompoundDesc;

// Create compound shape collection
UpeCompound *UpeCompound_Create(const UpeCompoundDesc *desc);

// Destroy compound
void UpeCompound_Destroy(UpeCompound *compound);

// Add shape to compound
uint32_t UpeCompound_AddShape(UpeCompound *compound, const UpeShape *shape,
                              UpeVec3 local_offset, UpeQuat local_rotation);

// Remove shape from compound
void UpeCompound_RemoveShape(UpeCompound *compound, uint32_t shape_idx);

// Get shape from compound
UpeCompoundShape *UpeCompound_GetShape(UpeCompound *compound, uint32_t shape_idx);

// Recalculate aggregate properties (COM, inertia)
void UpeCompound_UpdateAggregateProperties(UpeCompound *compound);

// Get world-space position of a shape within compound
UpeVec3 UpeCompound_GetShapeWorldPosition(UpeCompound *compound, uint32_t shape_idx,
                                         UpeVec3 body_position, UpeQuat body_rotation);

// Get world-space rotation of a shape within compound
UpeQuat UpeCompound_GetShapeWorldRotation(UpeCompound *compound, uint32_t shape_idx,
                                        UpeQuat body_rotation);

#ifdef __cplusplus
}
#endif

