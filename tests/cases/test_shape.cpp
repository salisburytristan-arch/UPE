#include "test_framework.h"
#include "upe/upe_shape.h"

TEST(TestShapeInit) {
    UpeShapeDesc desc;
    desc.type = UPE_SHAPE_SPHERE;
    desc.body_id = 10;
    desc.sphere.center = {0.0f, 0.0f, 0.0f};
    desc.sphere.radius = 2.0f;

    UpeShape shape;
    UpeShape_Init(&shape, 100, &desc);
    REQUIRE(shape.id == 100);
    REQUIRE(shape.type == UPE_SHAPE_SPHERE);
    REQUIRE(shape.body_id == 10);
    REQUIRE(shape.sphere.radius == 2.0f);
}
