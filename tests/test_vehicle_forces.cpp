#include "test_framework.h"
#include "upe/upe_vehicle.h"
#include "upe/upe_body.h"

TEST(TestVehicleForceApplication)
{
    UpeVehicleDesc desc = {};
    desc.chassis_body = 0;
    desc.wheel_radius = 0.3f;
    desc.wheel_width = 0.2f;
    desc.suspension_stiffness = 10000.0f;
    desc.suspension_damping = 100.0f;
    desc.wheel_count = 1;

    UpeVehicle vehicle;
    UpeVehicle_Init(&vehicle, &desc);

    UpeBody chassis;
    UpeBodyDesc bd;
    bd.type = UPE_BODY_DYNAMIC;
    bd.position = {0.0f, 0.0f, 0.0f};
    bd.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    bd.mass = 1000.0f;
    bd.collision_layer = 1;
    bd.collision_mask = 0xFFFFFFFF;
    UpeBody_Init(&chassis, 0, &bd);

    vehicle.wheel_count = 1;
    UpeWheel *wheel = &vehicle.wheels[0];
    wheel->is_touching = 1;
    wheel->contact_point = {0.5f, 0.0f, 0.0f};
    wheel->contact_normal = {0.0f, 1.0f, 0.0f};
    /* Negative suspension_force indicates spring compression; normal load = fabs(suspension_force) */
    wheel->suspension_force = -150.0f;
    wheel->angular_velocity = 10.0f;

    vehicle.wheel_friction = 1.0f;

    chassis.force = {0.0f, 0.0f, 0.0f};
    chassis.torque = {0.0f, 0.0f, 0.0f};

    UpeVehicle_UpdateTireForces(&vehicle, &chassis, 1.0f / 60.0f);

    /* Expect an upward normal force applied to chassis */
    REQUIRE(chassis.force.y > 0.0f);

    /* And an associated torque from applying force at an offset point */
    REQUIRE(!(chassis.torque.x == 0.0f && chassis.torque.y == 0.0f && chassis.torque.z == 0.0f));
}
