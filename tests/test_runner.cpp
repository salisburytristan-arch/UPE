#include "test_framework.h"
#include "upe/upe_state_hash.h"
#include <stdlib.h>
#include <string.h>

struct TestEntry {
    const char *name;
    TestFn fn;
};

static TestEntry g_tests[256];
static int g_test_count = 0;

void RegisterTest(const char *name, TestFn fn) {
    if (g_test_count < 256) {
        g_tests[g_test_count++] = {name, fn};
    }
}

int main() {
    int failures = 0;
    UpeStateHash hash = {};
    UpeStateHash_Begin(&hash, 0xC0FFEEULL);
    for (int i = 0; i < g_test_count; ++i) {
        try {
            g_tests[i].fn();
            printf("PASS: %s\n", g_tests[i].name);
            UpeStateHash_PushBytes(&hash, g_tests[i].name, (uint32_t)strlen(g_tests[i].name));
            UpeStateHash_PushU32(&hash, 1u);
        } catch (...) {
            failures++;
            printf("FAIL: %s\n", g_tests[i].name);
            UpeStateHash_PushBytes(&hash, g_tests[i].name, (uint32_t)strlen(g_tests[i].name));
            UpeStateHash_PushU32(&hash, 0u);
        }
    }
    uint64_t final_hash = UpeStateHash_Final(&hash);
    const char *expected = getenv("UPE_EXPECT_HASH");
    if (expected && strlen(expected) > 0) {
        uint64_t expect_val = strtoull(expected, nullptr, 16);
        if (expect_val != final_hash) {
            printf("Determinism hash mismatch. expected=0x%llX actual=0x%llX\n", (unsigned long long)expect_val, (unsigned long long)final_hash);
            return 1;
        }
    }
    printf("Test determinism hash: 0x%llX\n", (unsigned long long)final_hash);
    if (failures) {
        printf("%d tests failed\n", failures);
        return 1;
    }
    printf("All tests passed\n");
    return 0;
}
