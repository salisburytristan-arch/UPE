#include "upe/upe_math_scalar.h"

UpeScalarVec2 UpeScalarVec2_Add(UpeScalarVec2 a, UpeScalarVec2 b) {
    UpeScalarVec2 out = {
        UpeScalar_Add(a.x, b.x),
        UpeScalar_Add(a.y, b.y)
    };
    return out;
}

UpeScalarVec2 UpeScalarVec2_Sub(UpeScalarVec2 a, UpeScalarVec2 b) {
    UpeScalarVec2 out = {
        UpeScalar_Sub(a.x, b.x),
        UpeScalar_Sub(a.y, b.y)
    };
    return out;
}

UpeScalarVec2 UpeScalarVec2_Scale(UpeScalarVec2 v, UpeScalar s) {
    UpeScalarVec2 out = {
        UpeScalar_Mul(v.x, s),
        UpeScalar_Mul(v.y, s)
    };
    return out;
}

UpeScalar UpeScalarVec2_Dot(UpeScalarVec2 a, UpeScalarVec2 b) {
    return UpeScalar_Add(UpeScalar_Mul(a.x, b.x), UpeScalar_Mul(a.y, b.y));
}

UpeScalarVec3 UpeScalarVec3_Add(UpeScalarVec3 a, UpeScalarVec3 b) {
    UpeScalarVec3 out = {
        UpeScalar_Add(a.x, b.x),
        UpeScalar_Add(a.y, b.y),
        UpeScalar_Add(a.z, b.z)
    };
    return out;
}

UpeScalarVec3 UpeScalarVec3_Sub(UpeScalarVec3 a, UpeScalarVec3 b) {
    UpeScalarVec3 out = {
        UpeScalar_Sub(a.x, b.x),
        UpeScalar_Sub(a.y, b.y),
        UpeScalar_Sub(a.z, b.z)
    };
    return out;
}

UpeScalarVec3 UpeScalarVec3_Scale(UpeScalarVec3 v, UpeScalar s) {
    UpeScalarVec3 out = {
        UpeScalar_Mul(v.x, s),
        UpeScalar_Mul(v.y, s),
        UpeScalar_Mul(v.z, s)
    };
    return out;
}

UpeScalar UpeScalarVec3_Dot(UpeScalarVec3 a, UpeScalarVec3 b) {
    UpeScalar x = UpeScalar_Mul(a.x, b.x);
    UpeScalar y = UpeScalar_Mul(a.y, b.y);
    UpeScalar z = UpeScalar_Mul(a.z, b.z);
    return UpeScalar_Add(UpeScalar_Add(x, y), z);
}

UpeScalarVec3 UpeScalarVec3_Min(UpeScalarVec3 a, UpeScalarVec3 b) {
    UpeScalarVec3 out = {
        UpeScalar_Min(a.x, b.x),
        UpeScalar_Min(a.y, b.y),
        UpeScalar_Min(a.z, b.z)
    };
    return out;
}

UpeScalarVec3 UpeScalarVec3_Max(UpeScalarVec3 a, UpeScalarVec3 b) {
    UpeScalarVec3 out = {
        UpeScalar_Max(a.x, b.x),
        UpeScalar_Max(a.y, b.y),
        UpeScalar_Max(a.z, b.z)
    };
    return out;
}
