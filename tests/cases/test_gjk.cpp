#include "test_framework.h"
#include "upe/upe_gjk.h"
#include "upe/upe_geometry.h"

static UpeGjkSupport UpeSphere_Support(const void *shape, UpeVec3 dir) {
    const UpeSphere *sphere = (const UpeSphere *)shape;
    UpeGjkSupport support;
    float len = UpeVec3_Length(dir);
    if (len > 1.0e-6f) {
        support.point = UpeVec3_Add(sphere->center, UpeVec3_Scale(dir, sphere->radius / len));
    } else {
        support.point = sphere->center;
    }
    return support;
}

TEST(TestGjkSphereSphere) {
    UpeSphere a = {{0.0f, 0.0f, 0.0f}, 1.0f};
    UpeSphere b = {{0.25f, 0.0f, 0.0f}, 1.0f};

    UpeGjkResult result = UpeGjk_Distance(&a, UpeSphere_Support, &b, UpeSphere_Support, {1.0f, 0.0f, 0.0f});
    REQUIRE(result.distance >= 0.0f);
    REQUIRE(result.distance < 10.0f);
}
