#include "upe/upe_assert.h"
#include <stdio.h>
#include <stdlib.h>

void UpeAssert_Fail(const char *expr, const char *file, int line) {
    fprintf(stderr, "ASSERT FAILED: %s (%s:%d)\n", expr, file, line);
    abort();
}
