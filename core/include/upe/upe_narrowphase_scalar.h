#pragma once

#include "upe/upe_contact_scalar.h"
#include "upe/upe_geometry_scalar.h"

#ifdef __cplusplus
extern "C" {
#endif

int UpeNarrowphaseScalar_AabbAabb(UpeScalarAabb3 a, UpeScalarAabb3 b, UpeScalarContactManifold *manifold);
int UpeNarrowphaseScalar_SphereSphereOverlap(UpeScalarSphere a, UpeScalarSphere b);
int UpeNarrowphaseScalar_SphereAabbOverlap(UpeScalarSphere sphere, UpeScalarAabb3 box);
int UpeNarrowphaseScalar_SphereSphere(UpeScalarSphere a, UpeScalarSphere b, UpeScalarContactManifold *manifold);
int UpeNarrowphaseScalar_SphereCapsuleOverlap(UpeScalarSphere sphere, UpeScalarCapsule capsule);
int UpeNarrowphaseScalar_SphereCapsule(UpeScalarSphere sphere, UpeScalarCapsule capsule, UpeScalarContactManifold *manifold);
int UpeNarrowphaseScalar_SphereAabb(UpeScalarSphere sphere, UpeScalarAabb3 box, UpeScalarContactManifold *manifold);
int UpeNarrowphaseScalar_CapsuleAabbOverlap(UpeScalarCapsule capsule, UpeScalarAabb3 box);
int UpeNarrowphaseScalar_CapsuleCapsuleOverlap(UpeScalarCapsule a, UpeScalarCapsule b);
int UpeNarrowphaseScalar_CapsuleCapsule(UpeScalarCapsule a, UpeScalarCapsule b, UpeScalarContactManifold *manifold);

#ifdef __cplusplus
}
#endif
