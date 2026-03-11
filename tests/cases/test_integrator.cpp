#include "test_framework.h"
#include "upe/upe_integrator.h"

TEST(TestIntegratorEuler) {
    UpeBodyDesc desc;
    desc.type = UPE_BODY_DYNAMIC;
    desc.position = {0.0f, 10.0f, 0.0f};
    desc.rotation = UpeQuat_Identity();
    desc.mass = 1.0f;

    UpeBody body;
    UpeBody_Init(&body, 1, &desc);

    // Integrate with gravity (integrator applies gravity internally)
    UpeVec3 gravity = {0.0f, -9.8f, 0.0f};
    UpeIntegrator_Euler(&body, 0.1f, gravity);

    // After one timestep with gravity, velocity should be negative
    REQUIRE(body.velocity.y < 0.0f);
    // Position might not have moved yet (Euler integrates position with OLD velocity)
    // Take SECOND step to see position change
    UpeIntegrator_Euler(&body, 0.1f, gravity);
    REQUIRE(body.position.y < 10.0f);
}
