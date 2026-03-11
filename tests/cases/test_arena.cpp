#include "test_framework.h"
#include "upe/upe_arena.h"

TEST(TestArenaAllocReset) {
    UpeArena arena;
    UpeArena_Init(&arena, 64, nullptr);

    void *a = UpeArena_Alloc(&arena, 16, 8);
    void *b = UpeArena_Alloc(&arena, 16, 8);
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);

    UpeArena_Reset(&arena);
    void *c = UpeArena_Alloc(&arena, 32, 16);
    REQUIRE(c != nullptr);

    UpeArena_Destroy(&arena);
}
