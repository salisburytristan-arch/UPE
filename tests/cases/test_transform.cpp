#include "test_framework.h"
#include "upe/upe_transform.h"

TEST(TestTransform2Inverse) {
    UpeTransform2 t;
    t.position = {2.0f, -1.0f};
    t.rotation = 0.5f;

    UpeTransform2 inv = UpeTransform2_Inverse(t);
    UpeTransform2 ident = UpeTransform2_Compose(t, inv);

    REQUIRE(ident.position.x > -1.0e-4f && ident.position.x < 1.0e-4f);
    REQUIRE(ident.position.y > -1.0e-4f && ident.position.y < 1.0e-4f);
}

TEST(TestTransform3Compose) {
    UpeTransform3 a = UpeTransform3_Identity();
    UpeTransform3 b = UpeTransform3_Identity();
    b.position = {1.0f, 2.0f, 3.0f};

    UpeTransform3 out = UpeTransform3_Compose(a, b);
    REQUIRE(out.position.x == 1.0f);
    REQUIRE(out.position.y == 2.0f);
    REQUIRE(out.position.z == 3.0f);
}
