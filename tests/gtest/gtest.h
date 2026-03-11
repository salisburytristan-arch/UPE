#pragma once

#include "test_framework.h"

#include <cstdio>
#include <cstring>

namespace testing {

class Test {
public:
    virtual ~Test() = default;
    virtual void SetUp() {}
    virtual void TearDown() {}
};

struct HasSubstrMatcher {
    const char *needle;

    bool Match(const char *actual) const {
        if (!needle || !actual) {
            return false;
        }
        return std::strstr(actual, needle) != nullptr;
    }
};

inline HasSubstrMatcher HasSubstr(const char *needle) {
    return HasSubstrMatcher{needle};
}

} // namespace testing

inline void UpeGTestLite_Fail(const char *file, int line, const char *expr) {
    std::printf("FAIL: %s:%d: %s\n", file, line, expr ? expr : "(unknown)");
    throw 1;
}

#define EXPECT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            UpeGTestLite_Fail(__FILE__, __LINE__, #cond); \
        } \
    } while (0)

#define EXPECT_EQ(a, b) EXPECT_TRUE((a) == (b))
#define EXPECT_NE(a, b) EXPECT_TRUE((a) != (b))
#define EXPECT_GT(a, b) EXPECT_TRUE((a) > (b))
#define EXPECT_GE(a, b) EXPECT_TRUE((a) >= (b))
#define EXPECT_LT(a, b) EXPECT_TRUE((a) < (b))
#define EXPECT_LE(a, b) EXPECT_TRUE((a) <= (b))

#define EXPECT_STREQ(a, b) EXPECT_TRUE(std::strcmp((a), (b)) == 0)
#define EXPECT_STRNE(a, b) EXPECT_TRUE(std::strcmp((a), (b)) != 0)

#define EXPECT_STRLEN(s, lo, hi) \
    do { \
        const char *upe_gtestlite_s_ = (s); \
        EXPECT_TRUE(upe_gtestlite_s_ != nullptr); \
        size_t upe_gtestlite_n_ = std::strlen(upe_gtestlite_s_); \
        EXPECT_TRUE(upe_gtestlite_n_ >= (size_t)(lo)); \
        EXPECT_TRUE(upe_gtestlite_n_ <= (size_t)(hi)); \
    } while (0)

#define EXPECT_THAT(actual, matcher) EXPECT_TRUE((matcher).Match(actual))

#define TEST_F(fixture, test_name) \
    class fixture##_##test_name##_Test : public fixture { \
    public: \
        void TestBody(); \
        static void Run() { \
            fixture##_##test_name##_Test t; \
            t.SetUp(); \
            t.TestBody(); \
            t.TearDown(); \
        } \
    }; \
    struct fixture##_##test_name##_Reg { \
        fixture##_##test_name##_Reg() { RegisterTest(#fixture "." #test_name, fixture##_##test_name##_Test::Run); } \
    }; \
    static fixture##_##test_name##_Reg fixture##_##test_name##_reg; \
    void fixture##_##test_name##_Test::TestBody()
