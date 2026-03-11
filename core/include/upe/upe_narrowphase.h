#pragma once

#include "upe/upe_contact.h"
#include "upe/upe_geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Narrowphase shape wrappers for GJK-based collisions */
typedef struct UpeNarrowphaseConvexShape {
    const struct UpeVec3 *vertices;
    uint32_t vertex_count;
} UpeNarrowphaseConvexShape;

typedef struct UpeNarrowphaseSphereShape {
    struct UpeVec3 center;
    float radius;
} UpeNarrowphaseSphereShape;

typedef struct UpeNarrowphaseCapsuleShape {
    struct UpeVec3 a;
    struct UpeVec3 b;
    float radius;
} UpeNarrowphaseCapsuleShape;

typedef struct UpeNarrowphaseObbShape {
    struct UpeVec3 center;
    struct UpeVec3 half_extents;
    struct UpeVec3 axes[3];       /* Local X, Y, Z axes in world space */
} UpeNarrowphaseObbShape;

typedef struct UpeNarrowphaseMeshShape {
    const struct UpeVec3 *vertices;     /* Vertex array (const, may point to mesh data) */
    const uint32_t *indices;            /* Triangle index array */
    uint32_t vertex_count;
    uint32_t triangle_count;
    struct UpeVec3 center;              /* Mesh center for GJK initial direction */
} UpeNarrowphaseMeshShape;

int UpeNarrowphase_AabbAabb(UpeAabb3 a, UpeAabb3 b, UpeContactManifold *manifold);
int UpeNarrowphase_BoxBox(UpeAabb3 a, UpeAabb3 b, UpeContactManifold *manifold);
int UpeNarrowphase_SphereSphere(UpeSphere a, UpeSphere b, UpeContactManifold *manifold);
int UpeNarrowphase_SphereCapsule(UpeSphere sphere, UpeCapsule capsule, UpeContactManifold *manifold);
int UpeNarrowphase_CapsuleCapsule(UpeCapsule a, UpeCapsule b, UpeContactManifold *manifold);
int UpeNarrowphase_SphereAabb(UpeSphere sphere, UpeAabb3 box, UpeContactManifold *manifold);
int UpeNarrowphase_CapsuleAabb(UpeCapsule capsule, UpeAabb3 box, UpeContactManifold *manifold);
int UpeNarrowphase_SphereHeightfield(UpeSphere sphere, const struct UpeHeightfield *hf, UpeContactManifold *manifold);
int UpeNarrowphase_CapsuleHeightfield(UpeCapsule capsule, const struct UpeHeightfield *hf, UpeContactManifold *manifold);

// Convex hull collision using GJK+EPA
int UpeNarrowphase_ConvexConvex(const void *shape_a, const void *shape_b, UpeContactManifold *manifold);
int UpeNarrowphase_SphereConvex(const void *sphere_ptr, const void *convex_ptr, UpeContactManifold *manifold);
int UpeNarrowphase_CapsuleConvex(const void *capsule_ptr, const void *convex_ptr, UpeContactManifold *manifold);
int UpeNarrowphase_BoxConvex(const void *box_ptr, const void *convex_ptr, UpeContactManifold *manifold);
int UpeNarrowphase_ObbConvex(const void *obb_ptr, const void *convex_ptr, UpeContactManifold *manifold);
int UpeNarrowphase_MeshConvex(const void *mesh_ptr, const void *convex_ptr, UpeContactManifold *manifold);

#ifdef __cplusplus
}
#endif
