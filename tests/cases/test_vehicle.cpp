#include "test_framework.h"
#include "upe/upe_vehicle.h"

TEST(TestVehicleInit) {
    UpeVehicleDesc desc;
    desc.chassis_body = 0;
    desc.wheel_radius = 0.3f;
    desc.wheel_width = 0.2f;
    desc.suspension_stiffness = 1000.0f;
    desc.suspension_damping = 100.0f;
    desc.wheel_count = 4;

    UpeVehicle vehicle;
    UpeVehicle_Init(&vehicle, &desc);
    REQUIRE(vehicle.chassis_body == 0);
    REQUIRE(vehicle.wheel_count == 4);
}
