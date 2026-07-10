#pragma once

#include "upe/upe_body.h"

#ifdef __cplusplus
extern "C" {
#endif

void UpeIntegrator_Euler(UpeBody *body, float dt, UpeVec3 gravity);
void UpeIntegrator_SemiImplicit(UpeBody *body, float dt, UpeVec3 gravity);
void UpeIntegrator_VelocityVerlet(UpeBody *body, float dt, UpeVec3 gravity);

#ifdef __cplusplus
}
#endif
