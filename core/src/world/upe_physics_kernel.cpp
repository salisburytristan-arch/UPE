#include "upe/upe_physics_kernel.h"
#include <stdlib.h>
#include <string.h>

void UpePhysicsKernel_Init(UpePhysicsKernel *kernel, const UpePhysicsKernelDesc *desc) {
    if (!kernel || !desc) {
        return;
    }
    kernel->body_states = (UpeBodyState *)malloc(desc->max_bodies * sizeof(UpeBodyState));
    kernel->body_count = 0;
    kernel->constraints = (UpeSolverConstraint *)malloc(desc->max_contacts * sizeof(UpeSolverConstraint));
    kernel->constraint_count = 0;
    kernel->solver_iterations = desc->solver_iterations;
    kernel->baumgarte_factor = desc->baumgarte_factor;
}

void UpePhysicsKernel_Step(UpePhysicsKernel *kernel, float dt) {
    if (!kernel) {
        return;
    }

    for (uint32_t i = 0; i < kernel->solver_iterations; ++i) {
        UpeSolver_PgsIteration(kernel->body_states, kernel->body_count,
                              kernel->constraints, kernel->constraint_count);
    }
}

void UpePhysicsKernel_Destroy(UpePhysicsKernel *kernel) {
    if (!kernel) {
        return;
    }
    free(kernel->body_states);
    free(kernel->constraints);
    kernel->body_states = nullptr;
    kernel->constraints = nullptr;
}
