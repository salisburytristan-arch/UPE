#include "test_framework.h"
#include "upe/upe_pool.h"

TEST(TestPoolAllocFree) {
    UpePool pool;
    UpePool_Init(&pool, 16, 4, nullptr);

    void *a = UpePool_Alloc(&pool);
    void *b = UpePool_Alloc(&pool);
    void *c = UpePool_Alloc(&pool);
    void *d = UpePool_Alloc(&pool);
    void *e = UpePool_Alloc(&pool);

    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    REQUIRE(c != nullptr);
    REQUIRE(d != nullptr);
    REQUIRE(e == nullptr);

    UpePool_Free(&pool, b);
    void *f = UpePool_Alloc(&pool);
    REQUIRE(f == b);

    UpePool_Destroy(&pool);
}
