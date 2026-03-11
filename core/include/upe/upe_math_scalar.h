#pragma once

#include "upe/upe_scalar.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeScalarVec2 {
    UpeScalar x;
    UpeScalar y;
} UpeScalarVec2;

typedef struct UpeScalarVec3 {
    UpeScalar x;
    UpeScalar y;
    UpeScalar z;
} UpeScalarVec3;

UpeScalarVec2 UpeScalarVec2_Add(UpeScalarVec2 a, UpeScalarVec2 b);
UpeScalarVec2 UpeScalarVec2_Sub(UpeScalarVec2 a, UpeScalarVec2 b);
UpeScalarVec2 UpeScalarVec2_Scale(UpeScalarVec2 v, UpeScalar s);
UpeScalar UpeScalarVec2_Dot(UpeScalarVec2 a, UpeScalarVec2 b);

UpeScalarVec3 UpeScalarVec3_Add(UpeScalarVec3 a, UpeScalarVec3 b);
UpeScalarVec3 UpeScalarVec3_Sub(UpeScalarVec3 a, UpeScalarVec3 b);
UpeScalarVec3 UpeScalarVec3_Scale(UpeScalarVec3 v, UpeScalar s);
UpeScalar UpeScalarVec3_Dot(UpeScalarVec3 a, UpeScalarVec3 b);
UpeScalarVec3 UpeScalarVec3_Min(UpeScalarVec3 a, UpeScalarVec3 b);
UpeScalarVec3 UpeScalarVec3_Max(UpeScalarVec3 a, UpeScalarVec3 b);

#ifdef __cplusplus
}
#endif
