#pragma once

#include <stdint.h>
#include "upe/upe_geometry.h"
#include "upe/upe_obb.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declare UpeConvexHull from upe_convex.h
struct UpeConvexHull;
// Forward declare UpeTriangleMesh from upe_trimesh.h
struct UpeTriangleMesh;

typedef enum UpeShapeType {
    UPE_SHAPE_SPHERE,
    UPE_SHAPE_CAPSULE,
    UPE_SHAPE_BOX,
    UPE_SHAPE_OBB,               // Oriented bounding box
    UPE_SHAPE_MESH,
    UPE_SHAPE_CONVEX             // Convex hull (GJK/EPA) - v0.12.0
} UpeShapeType;

typedef struct UpeShapeDesc {
    UpeShapeType type;
    uint32_t body_id;
    uint32_t collision_layer;   // Override body layer (0 = use body layer)
    uint32_t collision_mask;    // Override body mask (0 = use body mask)
    float friction;             // Static/dynamic averaged friction (0 = default)
    float restitution;          // Bounciness (0 = default)
    union {
        UpeSphere sphere;
        UpeCapsule capsule;
        UpeAabb3 box;
        UpeOBB obb;              // OBB with rotation
        struct UpeConvexHull *convex;  // Convex hull pointer
        struct UpeTriangleMesh *mesh;   // Triangle mesh pointer
    };
} UpeShapeDesc;

typedef struct UpeShape {
    uint32_t id;
    UpeShapeType type;
    uint32_t body_id;
    uint32_t collision_layer;   // Override layer
    uint32_t collision_mask;    // Override mask
    float friction;
    float restitution;
    uint32_t next_shape_index;   // Next shape in body's linked list (0xFFFFFFFF if none)
    union {
        UpeSphere sphere;
        UpeCapsule capsule;
        UpeOBB obb;              // OBB with rotation
        UpeAabb3 box;
        struct UpeConvexHull *convex;  // Convex hull pointer
        struct UpeTriangleMesh *mesh;   // Triangle mesh pointer
    };
} UpeShape;

void UpeShape_Init(UpeShape *shape, uint32_t id, const UpeShapeDesc *desc);

#ifdef __cplusplus
}
#endif
