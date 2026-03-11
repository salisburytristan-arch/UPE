#include "upe/upe_transform.h"
#include <math.h>

static UpeVec2 UpeRotate2(UpeVec2 v, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    return {v.x * c - v.y * s, v.x * s + v.y * c};
}

UpeTransform2 UpeTransform2_Identity() {
    UpeTransform2 t;
    t.position = {0.0f, 0.0f};
    t.rotation = 0.0f;
    return t;
}

UpeTransform2 UpeTransform2_Compose(UpeTransform2 a, UpeTransform2 b) {
    UpeTransform2 out;
    out.rotation = a.rotation + b.rotation;
    out.position = UpeVec2_Add(a.position, UpeRotate2(b.position, a.rotation));
    return out;
}

UpeTransform2 UpeTransform2_Inverse(UpeTransform2 t) {
    UpeTransform2 out;
    out.rotation = -t.rotation;
    UpeVec2 neg = UpeVec2_Scale(t.position, -1.0f);
    out.position = UpeRotate2(neg, out.rotation);
    return out;
}

UpeVec2 UpeTransform2_TransformPoint(UpeTransform2 t, UpeVec2 p) {
    return UpeVec2_Add(UpeRotate2(p, t.rotation), t.position);
}

UpeTransform3 UpeTransform3_Identity() {
    UpeTransform3 t;
    t.position = {0.0f, 0.0f, 0.0f};
    t.rotation = UpeQuat_Identity();
    return t;
}

UpeTransform3 UpeTransform3_Compose(UpeTransform3 a, UpeTransform3 b) {
    UpeTransform3 out;
    out.rotation = UpeQuat_Mul(a.rotation, b.rotation);
    out.position = UpeVec3_Add(a.position, UpeQuat_RotateVec3(a.rotation, b.position));
    return out;
}

UpeTransform3 UpeTransform3_Inverse(UpeTransform3 t) {
    UpeTransform3 out;
    out.rotation = {-t.rotation.x, -t.rotation.y, -t.rotation.z, t.rotation.w};
    UpeVec3 neg = UpeVec3_Scale(t.position, -1.0f);
    out.position = UpeQuat_RotateVec3(out.rotation, neg);
    return out;
}

UpeVec3 UpeTransform3_TransformPoint(UpeTransform3 t, UpeVec3 p) {
    return UpeVec3_Add(UpeQuat_RotateVec3(t.rotation, p), t.position);
}
