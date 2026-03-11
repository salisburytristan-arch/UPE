#include "upe/upe_api.h"
#include "upe/upe_world_body_api.h"
#include <stdio.h>
#include <math.h>

int main() {
    printf("=== Unified Physics Engine - Advanced Vehicle Demo ===\n\n");

    // Create physics world
    UpeWorldDesc world_desc;
    world_desc.fixed_dt = 1.0f / 60.0f;
    UpeWorld *world = UpeWorld_Create(&world_desc);
    printf("Created physics world (dt=%.4f)\n", world_desc.fixed_dt);

    // Create ground plane
    UpeBodyDesc ground_desc;
    ground_desc.type = UPE_BODY_STATIC;
    ground_desc.position = {0.0f, 0.0f, 0.0f};
    ground_desc.rotation = UpeQuat_Identity();
    ground_desc.mass = 0.0f;
    uint32_t ground_id = UpeWorld_CreateBody(world, &ground_desc);

    UpeShapeDesc ground_shape;
    ground_shape.type = UPE_SHAPE_BOX;
    ground_shape.body_id = ground_id;
    ground_shape.box.min = {-100.0f, -1.0f, -100.0f};
    ground_shape.box.max = {100.0f, 0.0f, 100.0f};
    UpeWorld_CreateShape(world, &ground_shape);
    printf("Created ground plane\n");

    // Create vehicle chassis
    UpeBodyDesc chassis_desc;
    chassis_desc.type = UPE_BODY_DYNAMIC;
    chassis_desc.position = {0.0f, 1.5f, 0.0f};
    chassis_desc.rotation = UpeQuat_Identity();
    chassis_desc.mass = 1500.0f;
    uint32_t chassis_id = UpeWorld_CreateBody(world, &chassis_desc);

    UpeShapeDesc chassis_shape;
    chassis_shape.type = UPE_SHAPE_BOX;
    chassis_shape.body_id = chassis_id;
    chassis_shape.box.min = {-0.9f, -0.4f, -1.8f};
    chassis_shape.box.max = {0.9f, 0.8f, 1.8f};
    UpeWorld_CreateShape(world, &chassis_shape);
    printf("Created vehicle chassis (mass=1500kg)\n");

    // Initialize advanced vehicle
    UpeAdvancedVehicle vehicle;
    UpeAdvancedVehicle_Init(&vehicle, chassis_id, 4);
    
    // Configure transmission (automatic)
    vehicle.transmission.type = UPE_TRANSMISSION_AUTOMATIC;
    vehicle.transmission.engine_power = 200000.0f;  // 200 kW
    vehicle.transmission.engine_torque = 400.0f;    // 400 N⋅m
    
    // Enable stability systems
    vehicle.traction_control_enabled = 1;
    vehicle.abs_enabled = 1;
    
    printf("Configured transmission: AUTOMATIC\n");
    printf("Engine: 200 kW, 400 N⋅m\n");
    printf("Stability: Traction Control & ABS ENABLED\n\n");

    // Simulation parameters
    int total_frames = 600;  // 10 seconds at 60 Hz
    int print_interval = 60;  // Print every 1 second

    printf("Starting simulation (10 seconds)...\n");
    printf("Input sequence:\n");
    printf("  0-1s: Accelerate forward\n");
    printf("  1-3s: Steer and drift right\n");
    printf("  3-5s: Brake heavily\n");
    printf("  5-7s: Reverse with handbrake\n");
    printf("  7-10s: Accelerate forward again\n\n");

    printf("Time(s) | Gear | RPM    | Speed   | Drift Angle | Drift Score | Throttle\n");
    printf("--------|------|--------|---------|-------------|-------------|----------\n");

    for (int frame = 0; frame < total_frames; ++frame) {
        float time = frame * world_desc.fixed_dt;
        
        // Dynamic input sequence for racing demo
        float throttle = 0.0f;
        float steering = 0.0f;
        float brake = 0.0f;
        float handbrake = 0.0f;

        if (time < 1.0f) {
            // Straight acceleration
            throttle = 1.0f;
            steering = 0.0f;
        } else if (time < 3.0f) {
            // Drift right
            throttle = 0.8f;
            steering = 0.7f;  // Heavy steering for early drift
            if (time > 2.5f) {
                steering = -0.3f;  // Counter-steer to maintain drift
            }
        } else if (time < 5.0f) {
            // Emergency braking
            throttle = 0.0f;
            steering = 0.2f;
            brake = 1.0f;
        } else if (time < 7.0f) {
            // Reverse with handbrake
            throttle = -0.6f;
            handbrake = 1.0f;
        } else {
            // Accelerate again
            throttle = 1.0f;
            steering = 0.1f;  // Small steering correction
        }

        // Apply input to vehicle
        UpeAdvancedVehicle_SetInput(&vehicle, throttle, steering, brake, handbrake);

        // Update vehicle
        UpeBody *chassis = UpeWorld_GetBody(world, chassis_id);
        if (chassis) {
            UpeAdvancedVehicle_Update(&vehicle, chassis, world_desc.fixed_dt);
        }

        // Step physics world
        UpeWorld_Step(world);

        // Print statistics every second
        if (frame % print_interval == 0) {
            float speed_kmh = UpeVec3_Length(chassis->velocity) * 3.6f;
            int gear = vehicle.transmission.type == UPE_TRANSMISSION_AUTOMATIC ?
                       vehicle.transmission.automatic.current_gear :
                       vehicle.transmission.manual.current_gear;
            
            const char *gear_str = "N";
            if (gear > 0) {
                static char gear_buf[12];
                snprintf(gear_buf, sizeof(gear_buf), "%d", gear);
                gear_str = gear_buf;
            } else if (gear == -1) {
                gear_str = "R";
            }

            printf("%6.1f | %4s   | %6.0f | %7.2f | %11.1f | %11.2f | %+7.2f\n",
                   time,
                   gear_str,
                   vehicle.transmission.engine_rpm,
                   speed_kmh,
                   vehicle.drift_angle,
                   vehicle.drift_score,
                   throttle);
        }
    }

    printf("\n=== Final Statistics ===\n");
    
    UpeBody *final_chassis = UpeWorld_GetBody(world, chassis_id);
    if (final_chassis) {
        float final_speed = UpeVec3_Length(final_chassis->velocity) * 3.6f;
        printf("Final Speed: %.2f km/h\n", final_speed);
        printf("Final Drift Score: %.2f\n", vehicle.drift_score);
        printf("Transmission Type: %s\n", 
               vehicle.transmission.type == UPE_TRANSMISSION_AUTOMATIC ? "AUTOMATIC" : "MANUAL");
        printf("Stability Systems: TCS=%s, ABS=%s\n",
               vehicle.traction_control_enabled ? "ON" : "OFF",
               vehicle.abs_enabled ? "ON" : "OFF");
    }

    printf("\n=== Advanced Vehicle Features Demonstrated ===\n");
    printf("✓ Automatic transmission with RPM-based shifting\n");
    printf("✓ Realistic tire grip model (Pacejka magic formula)\n");
    printf("✓ Drift detection and scoring system\n");
    printf("✓ Traction control system (TCS)\n");
    printf("✓ Anti-lock braking (ABS)\n");
    printf("✓ Temperature and wear effects on grip\n");
    printf("✓ Aerodynamic downforce calculations\n");
    printf("✓ Multi-gear transmission with gear ratios\n");

    UpeWorld_Destroy(world);
    printf("\n=== Demo Complete ===\n");
    
    return 0;
}
