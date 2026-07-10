#include "test_framework.h"
#include "upe/upe_api.h"
#include "upe/upe_ccd.h"

TEST(TestCcdProjectileStressSphereBox) {
    UpeSphere sphere = {};
    sphere.center = {0.0f, 8.0f, 0.0f};
    sphere.radius = 0.35f;

    UpeAabb3 wall = {};
    wall.min = {-4.0f, -0.5f, -4.0f};
    wall.max = {4.0f, 0.0f, 4.0f};

    int hit_count = 0;

    for (int i = 0; i < 96; ++i) {
        float speed = 15.0f + (float)(i % 8) * 2.5f;
        UpeCcdResult result = UpeCcd_SphereBox(sphere, {0.0f, -speed, 0.0f}, wall, {0.0f, 0.0f, 0.0f}, 1.0f);
        if (result.hit) {
            hit_count++;
            REQUIRE(result.toi >= 0.0f);
            REQUIRE(result.toi <= 1.0f);
        }
        sphere.center.y += 0.01f;
    }

    REQUIRE(hit_count > 0);
}