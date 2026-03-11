#include "test_framework.h"
#include "upe/upe_ringbuffer.h"

TEST(TestRingBufferOrder) {
    UpeRingBuffer buffer;
    UpeRingBuffer_Init(&buffer, 3, sizeof(int), nullptr);

    int a = 1;
    int b = 2;
    int c = 3;
    int d = 4;

    REQUIRE(UpeRingBuffer_Push(&buffer, &a));
    REQUIRE(UpeRingBuffer_Push(&buffer, &b));
    REQUIRE(UpeRingBuffer_Push(&buffer, &c));
    REQUIRE(!UpeRingBuffer_Push(&buffer, &d));

    int out = 0;
    REQUIRE(UpeRingBuffer_Pop(&buffer, &out));
    REQUIRE(out == 1);
    REQUIRE(UpeRingBuffer_Push(&buffer, &d));

    REQUIRE(UpeRingBuffer_Pop(&buffer, &out));
    REQUIRE(out == 2);
    REQUIRE(UpeRingBuffer_Pop(&buffer, &out));
    REQUIRE(out == 3);
    REQUIRE(UpeRingBuffer_Pop(&buffer, &out));
    REQUIRE(out == 4);

    UpeRingBuffer_Destroy(&buffer);
}
