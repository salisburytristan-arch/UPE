#include "test_framework.h"
#include "upe/upe_heightfield.h"

TEST(TestHeightfieldSample) {
    UpeHeightfield hf;
    UpeHeightfield_Init(&hf);

    hf.rows = 3;
    hf.cols = 3;
    hf.cell_size = 1.0f;
    hf.heights = new float[9];
    for (int i = 0; i < 9; ++i) {
        hf.heights[i] = (float)i;
    }

    float h = UpeHeightfield_SampleHeight(&hf, 1.0f, 1.0f);
    REQUIRE(h >= 3.0f && h <= 5.0f);

    UpeHeightfield_Destroy(&hf);
}
