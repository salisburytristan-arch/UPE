#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeVec2 {
    float x;
    float y;
} UpeVec2;

typedef struct UpeVec3 {
    float x;
    float y;
    float z;
} UpeVec3;

typedef struct UpeVec4 {
    float x;
    float y;
    float z;
    float w;
} UpeVec4;

typedef struct UpeMat2 {
    float m[2][2];
} UpeMat2;

typedef struct UpeMat3 {
    float m[3][3];
} UpeMat3;

typedef UpeMat3 UpeMatrix3;

typedef struct UpeMat4 {
    float m[4][4];
} UpeMat4;

typedef struct UpeQuat {
    float x;
    float y;
    float z;
    float w;
} UpeQuat;

UpeVec2 UpeVec2_Add(UpeVec2 a, UpeVec2 b);
UpeVec2 UpeVec2_Sub(UpeVec2 a, UpeVec2 b);
float UpeVec2_Dot(UpeVec2 a, UpeVec2 b);
UpeVec2 UpeVec2_Scale(UpeVec2 v, float s);
float UpeVec2_Length(UpeVec2 v);
UpeVec2 UpeVec2_Normalize(UpeVec2 v);
UpeVec2 UpeVec2_Min(UpeVec2 a, UpeVec2 b);
UpeVec2 UpeVec2_Max(UpeVec2 a, UpeVec2 b);

UpeVec3 UpeVec3_Add(UpeVec3 a, UpeVec3 b);
UpeVec3 UpeVec3_Sub(UpeVec3 a, UpeVec3 b);
float UpeVec3_Dot(UpeVec3 a, UpeVec3 b);
UpeVec3 UpeVec3_Scale(UpeVec3 v, float s);
float UpeVec3_Length(UpeVec3 v);
UpeVec3 UpeVec3_Normalize(UpeVec3 v);
UpeVec3 UpeVec3_Cross(UpeVec3 a, UpeVec3 b);
UpeVec3 UpeVec3_Min(UpeVec3 a, UpeVec3 b);
UpeVec3 UpeVec3_Max(UpeVec3 a, UpeVec3 b);

UpeVec4 UpeVec4_Add(UpeVec4 a, UpeVec4 b);
UpeVec4 UpeVec4_Sub(UpeVec4 a, UpeVec4 b);
float UpeVec4_Dot(UpeVec4 a, UpeVec4 b);

UpeMat2 UpeMat2_Identity();
UpeVec2 UpeMat2_MulVec2(UpeMat2 m, UpeVec2 v);

UpeMat3 UpeMat3_Identity();
UpeVec3 UpeMat3_MulVec3(UpeMat3 m, UpeVec3 v);

UpeMat4 UpeMat4_Identity();
UpeVec4 UpeMat4_MulVec4(UpeMat4 m, UpeVec4 v);

UpeQuat UpeQuat_Identity();
UpeQuat UpeQuat_Mul(UpeQuat a, UpeQuat b);
UpeQuat UpeQuat_Multiply(UpeQuat a, UpeQuat b);  /* Alias for compatibility */
UpeQuat UpeQuat_Slerp(UpeQuat a, UpeQuat b, float t);  /* Spherical linear interpolation */
UpeVec3 UpeQuat_RotateVec3(UpeQuat q, UpeVec3 v);

#ifdef __cplusplus
}
#endif
