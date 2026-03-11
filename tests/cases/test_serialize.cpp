#include "test_framework.h"
#include "upe/upe_serialize.h"

TEST(TestSerializeU32) {
    uint8_t buffer[256];
    UpeSerializeContext ctx;
    UpeSerialize_Init(&ctx, buffer, sizeof(buffer));

    UpeSerialize_WriteU32(&ctx, 12345);
    REQUIRE(ctx.size == 4);

    ctx.size = 0;
    uint32_t value = UpeDeserialize_ReadU32(&ctx);
    REQUIRE(value == 12345);
}

TEST(TestSerializeF32) {
    uint8_t buffer[256];
    UpeSerializeContext ctx;
    UpeSerialize_Init(&ctx, buffer, sizeof(buffer));

    UpeSerialize_WriteF32(&ctx, 3.14159f);
    REQUIRE(ctx.size == 4);

    ctx.size = 0;
    float value = UpeDeserialize_ReadF32(&ctx);
    REQUIRE(value > 3.14f && value < 3.15f);
}
