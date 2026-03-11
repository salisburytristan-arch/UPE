#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void UpeAssert_Fail(const char *expr, const char *file, int line);

#define UPE_ASSERT(expr) ((expr) ? (void)0 : UpeAssert_Fail(#expr, __FILE__, __LINE__))

#ifdef __cplusplus
}
#endif
