#pragma once

#include <stdint.h>
#include "upe/upe_geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeTriangleMesh UpeTriangleMesh;

typedef struct UpeCcdResult {
    int hit;
    float toi;
    UpeVec3 position;
    UpeVec3 normal;
} UpeCcdResult;

UpeCcdResult UpeCcd_SphereSphere(UpeSphere a, UpeVec3 vel_a, UpeSphere b, UpeVec3 vel_b, float dt);
UpeCcdResult UpeCcd_CapsuleCapsule(UpeCapsule a, UpeVec3 vel_a, UpeCapsule b, UpeVec3 vel_b, float dt);
UpeCcdResult UpeCcd_SphereCapsule(UpeSphere sphere, UpeVec3 vel_s, UpeCapsule capsule, UpeVec3 vel_c, float dt);
UpeCcdResult UpeCcd_SphereBox(UpeSphere sphere, UpeVec3 vel_s, UpeAabb3 box, UpeVec3 vel_b, float dt);
UpeCcdResult UpeCcd_BoxBox(UpeAabb3 a, UpeVec3 vel_a, UpeAabb3 b, UpeVec3 vel_b, float dt);
UpeCcdResult UpeCcd_CapsuleBox(UpeCapsule capsule, UpeVec3 vel_c, UpeAabb3 box, UpeVec3 vel_b, float dt);

// CCD vs static mesh
UpeCcdResult UpeCcd_SphereTriMesh(UpeSphere sphere, UpeVec3 vel_s, const UpeTriangleMesh *mesh, float dt);
UpeCcdResult UpeCcd_CapsuleTriMesh(UpeCapsule capsule, UpeVec3 vel_c, const UpeTriangleMesh *mesh, float dt);

#ifdef __cplusplus
}
#endif
