#include "upe/upe_epsilon.h"
#include <math.h>

float UpeEpsilon_Scalar() {
    return 1.0e-5f;
}

int UpeEpsilon_Equal(float a, float b, float eps) {
    return fabsf(a - b) <= eps;
}

int UpeEpsilon_EqualDefault(float a, float b) {
    return UpeEpsilon_Equal(a, b, UpeEpsilon_Scalar());
}
