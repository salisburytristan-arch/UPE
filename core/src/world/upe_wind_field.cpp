#include "upe/upe_field_system.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

void UpeWindField_Init(UpeWindField *field, uint32_t width, uint32_t height, uint32_t depth, UpeVec3 cell_size) {
    if (!field) return;
    
    field->width = width;
    field->height = height;
    field->depth = depth;
    field->cell_size = cell_size;
    field->total_cells = width * height * depth;
    
    // Allocate velocity grid
    field->velocity = (UpeVec3 *)malloc(field->total_cells * sizeof(UpeVec3));
    field->turbulence = (float *)malloc(field->total_cells * sizeof(float));
    field->temperature = (float *)malloc(field->total_cells * sizeof(float));
    
    // Initialize with zero wind
    for (uint32_t i = 0; i < field->total_cells; ++i) {
        field->velocity[i].x = 0.0f;
        field->velocity[i].y = 0.0f;
        field->velocity[i].z = 0.0f;
        field->turbulence[i] = 0.0f;
        field->temperature[i] = 300.0f;
    }
    
    field->base_wind.x = 0.0f;
    field->base_wind.y = 0.0f;
    field->base_wind.z = 0.0f;
    field->wind_strength = 1.0f;
    field->turbulence_scale = 0.5f;
    field->gust_frequency = 0.5f;
    field->gust_strength = 0.3f;
    field->time_accumulator = 0.0f;
}

void UpeWindField_Destroy(UpeWindField *field) {
    if (!field) return;
    if (field->velocity) free(field->velocity);
    if (field->turbulence) free(field->turbulence);
    if (field->temperature) free(field->temperature);
}

uint32_t UpeWindField_GetCellIndex(UpeWindField *field, UpeVec3 position) {
    if (!field) return 0;
    
    // Convert world position to grid coordinates
    int32_t x = (int32_t)(position.x / field->cell_size.x);
    int32_t y = (int32_t)(position.y / field->cell_size.y);
    int32_t z = (int32_t)(position.z / field->cell_size.z);
    
    // Clamp to valid range
    if (x < 0) x = 0;
    if (x >= (int32_t)field->width) x = field->width - 1;
    if (y < 0) y = 0;
    if (y >= (int32_t)field->height) y = field->height - 1;
    if (z < 0) z = 0;
    if (z >= (int32_t)field->depth) z = field->depth - 1;
    
    return (uint32_t)(x + y * field->width + z * field->width * field->height);
}

UpeVec3 UpeWindField_SampleVelocity(UpeWindField *field, UpeVec3 position) {
    UpeVec3 zero;
    if (!field) {
        zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
        return zero;
    }
    
    // Simple nearest-neighbor lookup for now (no interpolation to keep it simple)
    uint32_t idx = UpeWindField_GetCellIndex(field, position);
    return field->velocity[idx];
}

void UpeWindField_SetBaseWind(UpeWindField *field, UpeVec3 wind) {
    if (!field) return;
    field->base_wind = wind;
}

void UpeWindField_AddGust(UpeWindField *field, UpeVec3 wind_impulse) {
    if (!field) return;
    // Simple additive gust - would be more sophisticated with decay time
    for (uint32_t i = 0; i < field->total_cells; ++i) {
        field->velocity[i].x += wind_impulse.x * 0.01f;
        field->velocity[i].y += wind_impulse.y * 0.01f;
        field->velocity[i].z += wind_impulse.z * 0.01f;
    }
}

void UpeWindField_UpdateGustTurbulence(UpeWindField *field, float dt) {
    if (!field) return;
    
    field->time_accumulator += dt;
    
    // Update base turbulence from gust pattern
    // Simple sine-wave based gust simulation
    float gust_phase = field->time_accumulator * field->gust_frequency;
    float gust_factor = sinf(gust_phase) * 0.5f + 0.5f;  // 0 to 1
    
    for (uint32_t i = 0; i < field->total_cells; ++i) {
        // Add gust component
        float local_turbulence = field->turbulence[i];
        
        // Smooth turbulence toward base + gust
        float target_scale = 1.0f + gust_factor * field->gust_strength;
        
        // Apply to velocity grid
        field->velocity[i].x = field->base_wind.x * target_scale + local_turbulence * field->turbulence_scale;
        field->velocity[i].y = field->base_wind.y * target_scale;
        field->velocity[i].z = field->base_wind.z * target_scale;
        
        // Decay turbulence
        field->turbulence[i] *= 0.95f;
    }
}

void UpeWindField_AffectClothParticle(UpeWindField *field, UpeVec3 position, float mass, float drag_coeff, UpeVec3 *out_force) {
    if (!field || !out_force) return;
    (void)mass;
    
    UpeVec3 wind_vel = UpeWindField_SampleVelocity(field, position);
    
    // Drag force from wind: F = 0.5 * rho * v^2 * C_d * A
    // Simplified: F = C_d * v, where v is relative to wind
    out_force->x = wind_vel.x * drag_coeff;
    out_force->y = wind_vel.y * drag_coeff;
    out_force->z = wind_vel.z * drag_coeff;
}

void UpeWindField_Step(UpeWindField *field, float dt) {
    if (!field) return;
    
    // Update gust and turbulence patterns
    UpeWindField_UpdateGustTurbulence(field, dt);
    
    // Could add additional physics here like wind propagation, obstruction effects, etc.
}
