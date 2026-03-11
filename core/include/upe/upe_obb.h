#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Oriented Bounding Box (OBB) Collision (5.0.0 + 5.0.0.0)
  
  True oriented boxes (not axis-aligned).
  Includes OBB vs OBB narrowphase using SAT (Separating Axis Theorem).
*/

// Oriented bounding box
typedef struct UpeOBB {
    UpeVec3 center;
    UpeVec3 half_extents;        // Half-widths along local axes
    UpeQuat rotation;            // Orientation
    
    // Cached axes (updated when rotation changes)
    UpeVec3 axes[3];             // Local X, Y, Z axes in world space
    
} UpeOBB;

// OBB-OBB contact result
typedef struct UpeOBBContact {
    int intersecting;
    
    UpeVec3 normal;              // Contact normal (from A to B)
    float penetration_depth;
    
    // Contact points (up to 8 for edge-edge, 4 for face-face)
    UpeVec3 contact_points[8];
    uint32_t contact_count;
    
    // Feature IDs (for persistent manifolds)
    uint32_t feature_a;          // Edge or face index on A
    uint32_t feature_b;          // Edge or face index on B
    
} UpeOBBContact;

// OBB construction
void UpeOBB_FromCenterExtents(UpeOBB *obb, UpeVec3 center, UpeVec3 half_extents, UpeQuat rotation);
void UpeOBB_FromAABB(UpeOBB *obb, UpeVec3 min_bounds, UpeVec3 max_bounds, UpeQuat rotation);
void UpeOBB_FromPoints(UpeOBB *obb, const UpeVec3 *points, uint32_t point_count);  // Fit to point cloud

// Update cached axes (call after rotation changes)
void UpeOBB_UpdateAxes(UpeOBB *obb);

// Queries
int UpeOBB_ContainsPoint(const UpeOBB *obb, UpeVec3 point);
UpeVec3 UpeOBB_ClosestPoint(const UpeOBB *obb, UpeVec3 point);
float UpeOBB_DistanceToPoint(const UpeOBB *obb, UpeVec3 point);

// AABB conversion (for broadphase)
void UpeOBB_ToAABB(const UpeOBB *obb, UpeVec3 *min_bounds, UpeVec3 *max_bounds);

// === OBB vs OBB Collision (SAT) ===

// Main collision test
int UpeOBB_TestOBB(const UpeOBB *a, const UpeOBB *b, UpeOBBContact *contact);

// SAT helper: test separation along axis
int UpeOBB_TestSeparation(const UpeOBB *a, const UpeOBB *b, UpeVec3 axis, float *separation);

// Contact generation (after intersection confirmed)
void UpeOBB_GenerateContacts(const UpeOBB *a, const UpeOBB *b, UpeOBBContact *contact);

// === OBB vs Other Shapes ===

int UpeOBB_TestSphere(const UpeOBB *obb, UpeVec3 sphere_center, float sphere_radius,
                      UpeOBBContact *contact);
int UpeOBB_TestCapsule(const UpeOBB *obb, UpeVec3 capsule_a, UpeVec3 capsule_b,
                       float capsule_radius, UpeOBBContact *contact);
int UpeOBB_TestPlane(const UpeOBB *obb, UpeVec3 plane_normal, float plane_distance,
                     UpeOBBContact *contact);

// === Ray vs OBB ===

int UpeOBB_Raycast(const UpeOBB *obb, UpeVec3 ray_origin, UpeVec3 ray_direction,
                   float max_distance, float *hit_distance, UpeVec3 *hit_normal);

// === Support Functions (for GJK/EPA) ===

UpeVec3 UpeOBB_Support(const UpeOBB *obb, UpeVec3 direction);

// === Utilities ===

// Volume and surface area
float UpeOBB_Volume(const UpeOBB *obb);
float UpeOBB_SurfaceArea(const UpeOBB *obb);

// Mass properties (assuming uniform density)
float UpeOBB_Mass(const UpeOBB *obb, float density);
UpeMat3 UpeOBB_InertiaTensor(const UpeOBB *obb, float mass);

// Transform OBB
void UpeOBB_Transform(UpeOBB *obb, UpeVec3 translation, UpeQuat rotation);

// Merge two OBBs (compute bounding OBB)
void UpeOBB_Merge(const UpeOBB *a, const UpeOBB *b, UpeOBB *result);

// === Debug ===

void UpeOBB_DebugDraw(const UpeOBB *obb, void *debug_draw, float r, float g, float b);

#ifdef __cplusplus
}
#endif
