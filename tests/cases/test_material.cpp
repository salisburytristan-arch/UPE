#include "test_framework.h"
#include "upe/upe_material.h"

TEST(TestMaterialInit) {
    UpeMaterial material;
    UpeMaterial_Init(&material, 0.5f, 0.4f, 0.6f);
    REQUIRE(material.static_friction == 0.5f);
    REQUIRE(material.dynamic_friction == 0.4f);
    REQUIRE(material.restitution == 0.6f);
}
