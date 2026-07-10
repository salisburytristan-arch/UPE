#include "test_framework.h"
#include "upe/upe_api.h"
#include "upe/upe_ccd.h"

TEST(TestCcdProjectileStressBoxBox) {
    UpeAabb3 moving_box = {};
    moving_box.min = {-0.25f, -0.25f, -0.25f};
    moving_box.max = {0.25f, 0.25f, 0.25f};

    UpeAabb3 target_box = {};
    target_box.min = {-0.5f, -0.5f, -0.5f};
    target_box.max = {0.5f, 0.5f, 0.5f};

    int hits = 0;
    for (int i = 0; i < 64; ++i) {
        float offset = 6.0f + (float)(i % 4);
        UpeCcdResult result = UpeCcd_BoxBox(moving_box, {offset, 0.0f, 0.0f}, target_box, {0.0f, 0.0f, 0.0f}, 1.0f);
        if (result.hit) {
            hits++;
            REQUIRE(result.toi >= 0.0f);
            REQUIRE(result.toi <= 1.0f);
        }
    }

    REQUIRE(hits > 0);
}