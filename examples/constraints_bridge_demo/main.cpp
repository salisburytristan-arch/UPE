#include "upe/upe_advanced_movement.h"
#include "upe/upe_api.h"
#include "upe/upe_character_controller.h"
#include "upe/upe_world_body_api.h"

#include <cstdio>

int main() {
    std::printf("=== UPE Constraints Bridge Demo ===\n");

    UpeWorldDesc desc = UpeWorldDesc_Default();
    desc.fixed_dt = 1.0f / 60.0f;
    desc.enable_determinism_mode = 1;

    UpeWorld *world = UpeWorld_Create(&desc);
    if (!world) {
        std::printf("Failed to create world\n");
        return 1;
    }

    UpeBodyDesc ground = {};
    ground.type = UPE_BODY_STATIC;
    ground.position = {0.0f, -0.5f, 0.0f};
    ground.rotation = UpeQuat_Identity();
    ground.mass = 0.0f;
    uint32_t ground_id = UpeWorld_CreateBody(world, &ground);

    UpeShapeDesc ground_shape = {};
    ground_shape.type = UPE_SHAPE_BOX;
    ground_shape.body_id = ground_id;
    ground_shape.box.min = {-10.0f, -0.5f, -10.0f};
    ground_shape.box.max = {10.0f, 0.5f, 10.0f};
    UpeWorld_CreateShape(world, &ground_shape);

    UpeBodyDesc stair = {};
    stair.type = UPE_BODY_STATIC;
    stair.position = {2.0f, 0.0f, 0.0f};
    stair.rotation = UpeQuat_Identity();
    stair.mass = 0.0f;
    uint32_t stair_id = UpeWorld_CreateBody(world, &stair);

    UpeShapeDesc stair_shape = {};
    stair_shape.type = UPE_SHAPE_BOX;
    stair_shape.body_id = stair_id;
    stair_shape.box.min = {-1.0f, 0.0f, -1.0f};
    stair_shape.box.max = {1.0f, 1.0f, 1.0f};
    UpeWorld_CreateShape(world, &stair_shape);

    UpeCharacterControllerDesc controller_desc = UpeCharacterController_GetDefaultConfig();
    UpeVec3 start_position = {0.0f, 1.8f, 0.0f};
    UpeCharacterController *controller = UpeCharacterController_Create(world, start_position, &controller_desc);
    if (!controller) {
        std::printf("Failed to create controller\n");
        UpeWorld_Destroy(world);
        return 1;
    }

    UpeAdvancedMovement *advanced = UpeAdvancedMovement_Create(controller);
    if (!advanced) {
        std::printf("Failed to create advanced movement bridge\n");
        UpeCharacterController_Destroy(controller);
        UpeWorld_Destroy(world);
        return 1;
    }

    UpeAdvancedMovement_SetAutoStepping(advanced, true);
    UpeAdvancedMovement_SetSlopeSliding(advanced, true);

    const float dt = 1.0f / 60.0f;
    for (int frame = 0; frame < 120; ++frame) {
        UpeVec2 movement_input = {1.0f, 0.0f};
        UpeCharacterController_SetMovementInput(controller, movement_input, UPE_CHARCTRL_LOCOMOTION_WALKING);
        UpeAdvancedMovement_UpdateSlopes(advanced, world, dt);
        UpeAdvancedMovement_UpdateStairs(advanced, world, dt);
        UpeCharacterController_Update(controller, dt, world);
        UpeCharacterController_ApplyVelocity(controller, world);
        UpeWorld_Step(world);
    }

    float speed = 0.0f;
    float direction = 0.0f;
    bool in_air = false;
    UpeCharacterController_GetAnimationParameters(controller, &speed, &direction, &in_air);

    std::printf("Locomotion: %d\n", (int)UpeCharacterController_GetLocomotion(controller));
    std::printf("Grounded: %s\n", UpeCharacterController_IsGrounded(controller) ? "yes" : "no");
    std::printf("Position: (%.3f, %.3f, %.3f)\n",
                UpeCharacterController_GetPosition(controller).x,
                UpeCharacterController_GetPosition(controller).y,
                UpeCharacterController_GetPosition(controller).z);
    std::printf("Animation blend: speed=%.3f direction=%.3f in_air=%s\n",
                speed, direction, in_air ? "yes" : "no");
    std::printf("Slope angle: %.2f degrees\n", UpeAdvancedMovement_GetSlopeAngle(advanced));

    UpeAdvancedMovement_Destroy(advanced);
    UpeCharacterController_Destroy(controller);
    UpeWorld_Destroy(world);
    return 0;
}