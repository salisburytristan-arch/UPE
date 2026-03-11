#pragma once

#ifdef __cplusplus
extern "C" {
#endif

float UpeEpsilon_Scalar();
int UpeEpsilon_Equal(float a, float b, float eps);
int UpeEpsilon_EqualDefault(float a, float b);

#ifdef __cplusplus
}
#endif
