/**
 * Vehicle + Terrain Demo (v0.12.0 Feature Showcase)
 * 
 * Demonstrates:
 * - Advanced vehicle physics (transmission, tires, suspensions)
 * - Heightfield terrain
 * - Player-controlled vehicle driving
 * - Physics integration
 */

#include <stdio.h>
#include <math.h>
#include "upe/upe_api.h"
#include "upe/upe_advanced_vehicle.h"
#include "upe/upe_heightfield.h"

/* Terrain dimensions */
#define TERRAIN_COLS 64
#define TERRAIN_ROWS 64
#define TERRAIN_CELL_SIZE 2.0f

int main(void) {
    printf("=== UPE v0.12.0 Vehicle + Terrain Demo ===\n");

    /* Create world */
    UpeWorldDesc world_desc = {
        .fixed_dt = 1.0f / 60.0f,
    };
    UpeWorld *world = UpeWorld_Create(&world_desc);
    if (!world) {
        printf("ERROR: Failed to create world\n");
        return 1;
    }
    printf("✓ World created\n");

    /* Create heightfield terrain */
    UpeHeightfield terrain = {};
    UpeHeightfield_Init(&terrain);
    if (!UpeHeightfield_Allocate(&terrain, TERRAIN_ROWS, TERRAIN_COLS, TERRAIN_CELL_SIZE)) {
        printf("ERROR: Failed to allocate heightfield\n");
        return 1;
    }

    /* Simple rolling hills */
    for (uint32_t row = 0; row < TERRAIN_ROWS; row++) {
        for (uint32_t col = 0; col < TERRAIN_COLS; col++) {
            float x = (float)col * TERRAIN_CELL_SIZE;
            float z = (float)row * TERRAIN_CELL_SIZE;
            /* Wave-like terrain */
            float height = 10.0f + 
                          5.0f * sinf(x * 0.1f) + 
                          3.0f * cosf(z * 0.15f);
            terrain.heights[row * TERRAIN_COLS + col] = height;
        }
    }
    printf("✓ Terrain allocated: %u×%u cells, %.1f m cell size\n", 
           TERRAIN_ROWS, TERRAIN_COLS, TERRAIN_CELL_SIZE);

    /* Create vehicle chassis body */
    UpeBodyDesc chassis_desc = {
        .type = UPE_BODY_DYNAMIC,
        .position = {32.0f, 15.0f, 32.0f},  /* Center of terrain */
        .mass = 1500.0f,  /* 1500 kg vehicle */
    };
    uint32_t chassis_id = UpeWorld_CreateBody(world, &chassis_desc);
    if (chassis_id == UINT32_MAX) {
        printf("ERROR: Failed to create chassis body\n");
        return 1;
    }
    printf("✓ Vehicle chassis created (1500 kg)\n");

    /* Create advanced vehicle */
    UpeAdvancedVehicle vehicle = {};
    UpeAdvancedVehicle_Init(&vehicle, chassis_id, 4);
    UpeTransmission_Init(&vehicle.transmission, UPE_TRANSMISSION_AUTOMATIC);
    printf("✓ Vehicle initialized with automatic transmission\n");

    /* Simulation loop */
    printf("\n=== Running simulation ===\n");
    float simulation_time = 0.0f;
    float max_sim_time = 5.0f;  /* 5 seconds */
    int step = 0;

    while (simulation_time < max_sim_time) {
        /* User input simulation */
        float throttle = 0.7f;  /* 70% throttle */
        float steering = 0.1f * sinf(simulation_time);  /* Gentle steering */
        float brake = 0.0f;
        float handbrake = 0.0f;

        /* Apply vehicle inputs */
        UpeAdvancedVehicle_SetInput(&vehicle, throttle, steering, brake, handbrake);

        /* Get chassis body for physics update */
        UpeBody *chassis = UpeWorld_GetBody(world, chassis_id);
        if (chassis) {
            /* Update vehicle dynamics */
            UpeAdvancedVehicle_Update(&vehicle, chassis, world_desc.fixed_dt);
        }

        /* Step physics world */
        UpeWorld_Step(world);

        simulation_time += world_desc.fixed_dt;
        step++;

        /* Print status every 60 frames (1 second at 60 FPS) */
        if (step % 60 == 0) {
            if (chassis) {
                float speed = sqrtf(chassis->velocity.x * chassis->velocity.x +
                                   chassis->velocity.z * chassis->velocity.z);
                printf("[%.1fs] Speed: %.1f m/s, Engine RPM: %.0f, Gear: %d\n",
                       simulation_time,
                       speed,
                       vehicle.transmission.engine_rpm,
                       vehicle.transmission.type == UPE_TRANSMISSION_AUTOMATIC ?
                           vehicle.transmission.automatic.current_gear :
                           vehicle.transmission.manual.current_gear);
            }
        }
    }

    /* Cleanup */
    UpeHeightfield_Destroy(&terrain);
    UpeWorld_Destroy(world);

    printf("\n✓ Demo completed successfully\n");
    printf("  Simulation time: %.1f seconds\n", simulation_time);
    printf("  Physics steps: %d\n", step);

    return 0;
}
