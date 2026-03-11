#include "upe/upe_frame.h"

UpeFrame UpeFrame_Identity() {
    UpeFrame frame;
    frame.origin = {0.0f, 0.0f, 0.0f};
    frame.axis_x = {1.0f, 0.0f, 0.0f};
    frame.axis_y = {0.0f, 1.0f, 0.0f};
    frame.axis_z = {0.0f, 0.0f, 1.0f};
    return frame;
}

UpeVec3 UpeFrame_TransformVector(const UpeFrame *frame, UpeVec3 vector) {
    if (!frame) {
        return vector;
    }
    UpeVec3 x = UpeVec3_Scale(frame->axis_x, vector.x);
    UpeVec3 y = UpeVec3_Scale(frame->axis_y, vector.y);
    UpeVec3 z = UpeVec3_Scale(frame->axis_z, vector.z);
    return UpeVec3_Add(UpeVec3_Add(x, y), z);
}

UpeVec3 UpeFrame_TransformPoint(const UpeFrame *frame, UpeVec3 point) {
    if (!frame) {
        return point;
    }
    UpeVec3 transformed = UpeFrame_TransformVector(frame, point);
    return UpeVec3_Add(transformed, frame->origin);
}
