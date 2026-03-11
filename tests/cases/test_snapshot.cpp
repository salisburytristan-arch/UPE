#include "test_framework.h"
#include "upe/upe_snapshot.h"

TEST(TestSnapshotCreateRestore) {
    struct TestState {
        float x, y, z;
        int frame;
    };

    TestState state = {1.0f, 2.0f, 3.0f, 42};
    UpeSnapshot *snapshot = UpeSnapshot_Create(&state, sizeof(TestState), 42);
    REQUIRE(snapshot != nullptr);
    REQUIRE(snapshot->frame == 42);

    TestState restored = {};
    int result = UpeSnapshot_Restore(snapshot, &restored, sizeof(TestState));
    REQUIRE(result == 1);
    REQUIRE(restored.x == 1.0f);
    REQUIRE(restored.frame == 42);

    UpeSnapshot_Destroy(snapshot);
}
