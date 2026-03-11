#pragma once

#include <stdint.h>
#include "upe/upe_body.h"
#include "upe/upe_contact.h"
#include "upe/upe_solver.h"
#include "upe/upe_friction.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpePhysicsKernelDesc {
    uint32_t max_bodies;
    uint32_t max_contacts;
    uint32_t solver_iterations;
    float baumgarte_factor;
} UpePhysicsKernelDesc;

typedef struct UpePhysicsKernel {
    UpeBodyState *body_states;
    uint32_t body_count;
    UpeSolverConstraint *constraints;
    uint32_t constraint_count;
    uint32_t solver_iterations;
    float baumgarte_factor;
} UpePhysicsKernel;

void UpePhysicsKernel_Init(UpePhysicsKernel *kernel, const UpePhysicsKernelDesc *desc);
void UpePhysicsKernel_Step(UpePhysicsKernel *kernel, float dt);
void UpePhysicsKernel_Destroy(UpePhysicsKernel *kernel);

#ifdef __cplusplus
}
#endif
