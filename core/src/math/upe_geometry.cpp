#include "upe/upe_geometry.h"

UpeAabb2 UpeAabb2_FromMinMax(UpeVec2 min, UpeVec2 max) {
    UpeAabb2 box;
    box.min = min;
    box.max = max;
    return box;
}

UpeAabb3 UpeAabb3_FromMinMax(UpeVec3 min, UpeVec3 max) {
    UpeAabb3 box;
    box.min = min;
    box.max = max;
    return box;
}

UpeAabb2 UpeAabb2_Expand(UpeAabb2 box, UpeVec2 point) {
    box.min = UpeVec2_Min(box.min, point);
    box.max = UpeVec2_Max(box.max, point);
    return box;
}

UpeAabb3 UpeAabb3_Expand(UpeAabb3 box, UpeVec3 point) {
    box.min = UpeVec3_Min(box.min, point);
    box.max = UpeVec3_Max(box.max, point);
    return box;
}

int UpeAabb2_Contains(UpeAabb2 box, UpeVec2 point) {
    return point.x >= box.min.x && point.x <= box.max.x && point.y >= box.min.y && point.y <= box.max.y;
}

int UpeAabb3_Contains(UpeAabb3 box, UpeVec3 point) {
    return point.x >= box.min.x && point.x <= box.max.x &&
           point.y >= box.min.y && point.y <= box.max.y &&
           point.z >= box.min.z && point.z <= box.max.z;
}

int UpeAabb2_IsValid(UpeAabb2 box) {
    return box.min.x <= box.max.x && box.min.y <= box.max.y;
}

int UpeAabb3_IsValid(UpeAabb3 box) {
    return box.min.x <= box.max.x && box.min.y <= box.max.y && box.min.z <= box.max.z;
}

int UpeSphere_IsValid(UpeSphere sphere) {
    return sphere.radius >= 0.0f;
}

int UpeCapsule_IsValid(UpeCapsule capsule) {
    return capsule.radius >= 0.0f;
}

int UpeObb_IsValid(UpeObb obb) {
    return obb.half_extents.x >= 0.0f && obb.half_extents.y >= 0.0f && obb.half_extents.z >= 0.0f;
}

int UpeAabb2_Overlap(UpeAabb2 a, UpeAabb2 b) {
    return !(a.max.x < b.min.x || a.min.x > b.max.x || a.max.y < b.min.y || a.min.y > b.max.y);
}

int UpeAabb3_Overlap(UpeAabb3 a, UpeAabb3 b) {
    return !(a.max.x < b.min.x || a.min.x > b.max.x ||
             a.max.y < b.min.y || a.min.y > b.max.y ||
             a.max.z < b.min.z || a.min.z > b.max.z);
}

UpeAabb3 UpeAabb3_Union(UpeAabb3 a, UpeAabb3 b) {
    UpeAabb3 out;
    out.min = UpeVec3_Min(a.min, b.min);
    out.max = UpeVec3_Max(a.max, b.max);
    return out;
}

UpeVec3 UpeAabb3_Center(UpeAabb3 box) {
    return UpeVec3_Scale(UpeVec3_Add(box.min, box.max), 0.5f);
}
