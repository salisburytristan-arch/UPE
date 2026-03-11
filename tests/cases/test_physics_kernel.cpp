#include "test_framework.h"
#include "upe/upe_physics_kernel.h"

TEST(TestPhysicsKernelInit) {
    UpePhysicsKernelDesc desc;
    desc.max_bodies = 100;
    desc.max_contacts = 1000;
    desc.solver_iterations = 10;
    desc.baumgarte_factor = 0.2f;

    UpePhysicsKernel kernel;
    UpePhysicsKernel_Init(&kernel, &desc);
    REQUIRE(kernel.body_states != nullptr);
    REQUIRE(kernel.solver_iterations == 10);

    UpePhysicsKernel_Destroy(&kernel);
}
