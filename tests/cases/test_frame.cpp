#include "test_framework.h"
#include "upe/upe_frame.h"

TEST(TestFrameIdentity) {
    UpeFrame frame = UpeFrame_Identity();
    UpeVec3 point{1.0f, 2.0f, 3.0f};
    UpeVec3 out = UpeFrame_TransformPoint(&frame, point);
    REQUIRE(out.x == point.x);
    REQUIRE(out.y == point.y);
    REQUIRE(out.z == point.z);
}
