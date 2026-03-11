#include "upe/upe_geometry_scalar.h"

UpeScalarAabb3 UpeScalarAabb3_FromMinMax(UpeScalarVec3 min, UpeScalarVec3 max) {
    UpeScalarAabb3 box;
    box.min = min;
    box.max = max;
    return box;
}

UpeScalarAabb3 UpeScalarAabb3_Expand(UpeScalarAabb3 box, UpeScalarVec3 point) {
    box.min = UpeScalarVec3_Min(box.min, point);
    box.max = UpeScalarVec3_Max(box.max, point);
    return box;
}

int UpeScalarAabb3_Contains(UpeScalarAabb3 box, UpeScalarVec3 point) {
    return UpeScalar_Cmp(point.x, box.min.x) >= 0 && UpeScalar_Cmp(point.x, box.max.x) <= 0 &&
           UpeScalar_Cmp(point.y, box.min.y) >= 0 && UpeScalar_Cmp(point.y, box.max.y) <= 0 &&
           UpeScalar_Cmp(point.z, box.min.z) >= 0 && UpeScalar_Cmp(point.z, box.max.z) <= 0;
}

int UpeScalarAabb3_IsValid(UpeScalarAabb3 box) {
    return UpeScalar_Cmp(box.min.x, box.max.x) <= 0 &&
           UpeScalar_Cmp(box.min.y, box.max.y) <= 0 &&
           UpeScalar_Cmp(box.min.z, box.max.z) <= 0;
}

int UpeScalarAabb3_Overlap(UpeScalarAabb3 a, UpeScalarAabb3 b) {
    return !(UpeScalar_Cmp(a.max.x, b.min.x) < 0 || UpeScalar_Cmp(a.min.x, b.max.x) > 0 ||
             UpeScalar_Cmp(a.max.y, b.min.y) < 0 || UpeScalar_Cmp(a.min.y, b.max.y) > 0 ||
             UpeScalar_Cmp(a.max.z, b.min.z) < 0 || UpeScalar_Cmp(a.min.z, b.max.z) > 0);
}

UpeScalarAabb3 UpeScalarAabb3_Union(UpeScalarAabb3 a, UpeScalarAabb3 b) {
    UpeScalarAabb3 out;
    out.min = UpeScalarVec3_Min(a.min, b.min);
    out.max = UpeScalarVec3_Max(a.max, b.max);
    return out;
}

UpeScalarVec3 UpeScalarAabb3_Center(UpeScalarAabb3 box) {
    UpeScalarVec3 sum = UpeScalarVec3_Add(box.min, box.max);
    return UpeScalarVec3_Scale(sum, UpeScalar_Half());
}
