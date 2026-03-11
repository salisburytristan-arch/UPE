#include "test_framework.h"
#include "upe/upe_compound_shapes.h"

TEST(TestCompoundInit) {
    UpeShape shapes[2];
    shapes[0].type = UPE_SHAPE_SPHERE;
    shapes[0].sphere.radius = 1.0f;
    shapes[1].type = UPE_SHAPE_BOX;

    UpeCompoundDesc desc;
    desc.body_id = 1;
    desc.max_shapes = 2;

    UpeCompound *compound = UpeCompound_Create(&desc);
    REQUIRE(compound != nullptr);

    UpeVec3 offset = {0.0f, 0.0f, 0.0f};
    UpeQuat rotation = UpeQuat_Identity();

    REQUIRE(UpeCompound_AddShape(compound, &shapes[0], offset, rotation) != (uint32_t)-1);
    REQUIRE(UpeCompound_AddShape(compound, &shapes[1], offset, rotation) != (uint32_t)-1);
    REQUIRE(compound->shape_count == 2);

    UpeCompound_Destroy(compound);
}
