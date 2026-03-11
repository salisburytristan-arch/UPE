#include "test_framework.h"
#include "upe/upe_body.h"

TEST(TestBodyInit) {
    UpeBodyDesc desc;
    desc.type = UPE_BODY_DYNAMIC;
    desc.position = {1.0f, 2.0f, 3.0f};
    desc.rotation = UpeQuat_Identity();
    desc.mass = 10.0f;

    UpeBody body;
    UpeBody_Init(&body, 42, &desc);
    REQUIRE(body.id == 42);
    REQUIRE(body.type == UPE_BODY_DYNAMIC);
    REQUIRE(body.mass == 10.0f);
    REQUIRE(body.inv_mass > 0.0f);
}

TEST(TestBodyApplyForce) {
    UpeBodyDesc desc;
    desc.type = UPE_BODY_DYNAMIC;
    desc.position = {0.0f, 0.0f, 0.0f};
    desc.rotation = UpeQuat_Identity();
    desc.mass = 1.0f;

    UpeBody body;
    UpeBody_Init(&body, 1, &desc);
    UpeBody_ApplyForce(&body, {10.0f, 0.0f, 0.0f});
    REQUIRE(body.force.x == 10.0f);

    UpeBody_ClearForces(&body);
    REQUIRE(body.force.x == 0.0f);
}
