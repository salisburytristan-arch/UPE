#pragma once

#include <stdio.h>

using TestFn = void (*)();

void RegisterTest(const char *name, TestFn fn);

#define TEST(name) \
    static void name(); \
    struct name##_Reg { \
        name##_Reg() { RegisterTest(#name, name); } \
    }; \
    static name##_Reg name##_reg; \
    static void name()

#define REQUIRE(cond) \
    do { \
        if (!(cond)) { \
            printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            throw 1; \
        } \
    } while (0)
