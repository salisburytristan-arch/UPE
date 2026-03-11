#include "test_framework.h"
#include "upe/upe_mass.h"

TEST(TestMassPropertiesSphere) {
    UpeMassProperties props = UpeMassProperties_FromSphere(1.0f, 1.0f);
    REQUIRE(props.mass > 0.0f);
    REQUIRE(props.inertia_tensor.m[0][0] > 0.0f);
}

TEST(TestMassPropertiesBox) {
    UpeMassProperties props = UpeMassProperties_FromBox(1.0f, {1.0f, 1.0f, 1.0f});
    REQUIRE(props.mass > 0.0f);
    REQUIRE(props.inertia_tensor.m[0][0] > 0.0f);
}
