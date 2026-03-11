#include "upe/upe_buoyancy.h"
#include "upe/upe_shape.h"
#include <math.h>
#include <string.h>

#define UPE_PI 3.14159265359f

static float UpeBuoyancy_ClampPositive(float value, float fallback)
{
    return value > 1e-6f ? value : fallback;
}

static float UpeBuoyancy_ClampUnitSize(float value)
{
    if (value < 0.1f) return 0.1f;
    if (value > 4.0f) return 4.0f;
    return value;
}

static float UpeBuoyancy_InferEffectiveRadius(const UpeBody* body)
{
    if (!body) {
        return 0.5f;
    }

    float mass = UpeBuoyancy_ClampPositive(body->mass, 50.0f);
    float ix = body->inv_inertia.x > 1e-6f ? (1.0f / body->inv_inertia.x) : 0.0f;
    float iy = body->inv_inertia.y > 1e-6f ? (1.0f / body->inv_inertia.y) : 0.0f;
    float iz = body->inv_inertia.z > 1e-6f ? (1.0f / body->inv_inertia.z) : 0.0f;

    if (ix > 1e-6f && iy > 1e-6f && iz > 1e-6f) {
        float i_avg = (ix + iy + iz) / 3.0f;
        float r = sqrtf((2.5f * i_avg) / mass);
        return UpeBuoyancy_ClampUnitSize(r);
    }

    const float assumed_density = 600.0f;
    float volume = mass / assumed_density;
    float r = cbrtf((3.0f * volume) / (4.0f * UPE_PI));
    return UpeBuoyancy_ClampUnitSize(r);
}

static float UpeBuoyancy_InferTotalVolume(const UpeBody* body)
{
    float radius = UpeBuoyancy_InferEffectiveRadius(body);
    return (4.0f / 3.0f) * UPE_PI * radius * radius * radius;
}

// Helper: Get signed distance to plane
static float SignedDistanceToPlane(UpeVec3 point, UpeVec3 plane_normal, float plane_height) {
    return UpeVec3_Dot(point, plane_normal) - plane_height;
}

// Helper: Clamp value
static float Clamp(float value, float min_val, float max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

// Compute submerged volume for sphere
float UpeBuoyancy_ComputeSubmergedVolume_Sphere(
    UpeVec3 center, float radius,
    UpeVec3 surface_normal, float surface_height
) {
    float distance = SignedDistanceToPlane(center, surface_normal, surface_height);
    
    // Fully above water
    if (distance > radius) return 0.0f;
    
    // Fully submerged
    if (distance < -radius) {
        return (4.0f / 3.0f) * UPE_PI * radius * radius * radius;
    }
    
    // Partially submerged - spherical cap
    float h = radius - distance;  // Height of cap
    return UPE_PI * h * h * (3.0f * radius - h) / 3.0f;
}

// Compute submerged volume for box using sampling
float UpeBuoyancy_ComputeSubmergedVolume_Box(
    UpeVec3 center, UpeQuat orientation, UpeVec3 half_extents,
    UpeVec3 surface_normal, float surface_height,
    uint32_t sample_count
) {
    if (sample_count < 8) sample_count = 8;
    if (sample_count > 64) sample_count = 64;
    
    uint32_t samples_per_axis = (uint32_t)(powf((float)sample_count, 1.0f / 3.0f) + 0.5f);
    float dx = 2.0f * half_extents.x / (float)(samples_per_axis - 1);
    float dy = 2.0f * half_extents.y / (float)(samples_per_axis - 1);
    float dz = 2.0f * half_extents.z / (float)(samples_per_axis - 1);
    float sample_volume = dx * dy * dz;
    
    uint32_t submerged_count = 0;
    
    for (uint32_t ix = 0; ix < samples_per_axis; ++ix) {
        for (uint32_t iy = 0; iy < samples_per_axis; ++iy) {
            for (uint32_t iz = 0; iz < samples_per_axis; ++iz) {
                UpeVec3 local;
                local.x = -half_extents.x + (float)ix * dx;
                local.y = -half_extents.y + (float)iy * dy;
                local.z = -half_extents.z + (float)iz * dz;
                
                // Rotate to world space
                UpeVec3 world = UpeQuat_RotateVec3(orientation, local);
                world.x += center.x;
                world.y += center.y;
                world.z += center.z;
                
                float dist = SignedDistanceToPlane(world, surface_normal, surface_height);
                if (dist < 0.0f) {
                    submerged_count++;
                }
            }
        }
    }
    
    return (float)submerged_count * sample_volume;
}

// Compute submerged volume for capsule using sampling
float UpeBuoyancy_ComputeSubmergedVolume_Capsule(
    UpeVec3 center, UpeQuat orientation, float radius, float half_height,
    UpeVec3 surface_normal, float surface_height,
    uint32_t sample_count
) {
    if (sample_count < 8) sample_count = 8;
    
    // Simplified: sample along capsule axis and integrate cylindrical slices
    uint32_t height_samples = sample_count / 2;
    
    float dy = 2.0f * half_height / (float)height_samples;
    float submerged_volume = 0.0f;
    
    UpeVec3 axis;
    axis.x = 0; axis.y = 1; axis.z = 0;
    axis = UpeQuat_RotateVec3(orientation, axis);
    
    for (uint32_t i = 0; i < height_samples; ++i) {
        float t = -half_height + (float)i * dy;
        UpeVec3 point;
        point.x = center.x + axis.x * t;
        point.y = center.y + axis.y * t;
        point.z = center.z + axis.z * t;
        
        float dist = SignedDistanceToPlane(point, surface_normal, surface_height);
        
        // Check if this slice is submerged
        if (dist < radius) {
            // Compute submerged radius at this height
            float submerged_r = (dist < -radius) ? radius : sqrtf(radius * radius - dist * dist);
            submerged_volume += UPE_PI * submerged_r * submerged_r * dy;
        }
    }
    
    // Add hemisphere contributions
    UpeVec3 end1_pos;
    end1_pos.x = center.x - axis.x * half_height;
    end1_pos.y = center.y - axis.y * half_height;
    end1_pos.z = center.z - axis.z * half_height;
    
    UpeVec3 end2_pos;
    end2_pos.x = center.x + axis.x * half_height;
    end2_pos.y = center.y + axis.y * half_height;
    end2_pos.z = center.z + axis.z * half_height;
    
    submerged_volume += UpeBuoyancy_ComputeSubmergedVolume_Sphere(
        end1_pos, radius, surface_normal, surface_height
    ) * 0.5f;
    
    submerged_volume += UpeBuoyancy_ComputeSubmergedVolume_Sphere(
        end2_pos, radius, surface_normal, surface_height
    ) * 0.5f;
    
    return submerged_volume;
}

// Calculate center of buoyancy
UpeVec3 UpeBuoyancy_CalculateCenterOfBuoyancy(
    const UpeBody* body,
    UpeVec3 surface_normal,
    float surface_height,
    uint32_t sample_count
) {
    UpeVec3 center_of_buoyancy = {0, 0, 0};
    float total_weight = 0.0f;
    
    // Sample volume and accumulate weighted position
    uint32_t samples_per_axis = (uint32_t)(powf((float)sample_count, 1.0f / 3.0f) + 0.5f);
    
    float radius = UpeBuoyancy_InferEffectiveRadius(body);
    float dx = 2.0f * radius / (float)samples_per_axis;
    float dy = dx;
    
    UpeVec3 world_pos = body->position;
    
    for (uint32_t ix = 0; ix < samples_per_axis; ++ix) {
        for (uint32_t iy = 0; iy < samples_per_axis; ++iy) {
            for (uint32_t iz = 0; iz < samples_per_axis; ++iz) {
                UpeVec3 local;
                local.x = -radius + (float)ix * dx;
                local.y = -radius + (float)iy * dy;
                local.z = -radius + (float)iz * dx;
                
                UpeVec3 world;
                world.x = world_pos.x + local.x;
                world.y = world_pos.y + local.y;
                world.z = world_pos.z + local.z;
                
                float dist = SignedDistanceToPlane(world, surface_normal, surface_height);
                if (dist < 0.0f) {
                    float weight = 1.0f;
                    center_of_buoyancy.x += world.x * weight;
                    center_of_buoyancy.y += world.y * weight;
                    center_of_buoyancy.z += world.z * weight;
                    total_weight += weight;
                }
            }
        }
    }
    
    if (total_weight > 0.0f) {
        center_of_buoyancy.x /= total_weight;
        center_of_buoyancy.y /= total_weight;
        center_of_buoyancy.z /= total_weight;
    } else {
        center_of_buoyancy = world_pos;
    }
    
    return center_of_buoyancy;
}

// Apply buoyancy from uniform fluid
void UpeBuoyancy_ApplyUniform(
    UpeBody* body,
    const UpeBuoyancyDesc* desc,
    UpeBuoyancyState* state_out
) {
    if (!body || !desc) return;
    
    memset(state_out, 0, sizeof(UpeBuoyancyState));
    
    UpeVec3 center = body->position;
    
    // Compute submerged volume based on shape type
    float submerged_volume = 0.0f;
    float total_volume = UpeBuoyancy_InferTotalVolume(body);
    float radius = UpeBuoyancy_InferEffectiveRadius(body);
    submerged_volume = UpeBuoyancy_ComputeSubmergedVolume_Sphere(
        center, radius, desc->fluid_surface_normal, desc->fluid_surface_height
    );
    
    state_out->submerged_volume = submerged_volume;
    state_out->submerged_ratio = Clamp(submerged_volume / total_volume, 0.0f, 1.0f);
    
    // Compute buoyancy force: F = ρ * V_submerged * g
    float buoyancy_magnitude = desc->fluid_density * submerged_volume * 9.81f;
    state_out->buoyancy_force.x = desc->fluid_surface_normal.x * buoyancy_magnitude;
    state_out->buoyancy_force.y = desc->fluid_surface_normal.y * buoyancy_magnitude;
    state_out->buoyancy_force.z = desc->fluid_surface_normal.z * buoyancy_magnitude;
    
    // Compute drag force: F_drag = -0.5 * ρ * Cd * A * v²
    UpeVec3 vel = body->velocity;
    float vel_mag = sqrtf(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
    
    if (vel_mag > 1e-6f) {
        float drag_mag = 0.5f * desc->fluid_density * desc->drag_coefficient * 
                        (UPE_PI * radius * radius) * vel_mag * vel_mag * state_out->submerged_ratio;
        state_out->drag_force.x = -(vel.x / vel_mag) * drag_mag;
        state_out->drag_force.y = -(vel.y / vel_mag) * drag_mag;
        state_out->drag_force.z = -(vel.z / vel_mag) * drag_mag;
    }
    
    // Calculate center of buoyancy
    state_out->center_of_buoyancy = UpeBuoyancy_CalculateCenterOfBuoyancy(
        body, desc->fluid_surface_normal, desc->fluid_surface_height, desc->sample_points
    );
    
    // Compute torque from buoyancy force applied at center of buoyancy
    UpeVec3 r;
    r.x = state_out->center_of_buoyancy.x - center.x;
    r.y = state_out->center_of_buoyancy.y - center.y;
    r.z = state_out->center_of_buoyancy.z - center.z;
    
    state_out->torque = UpeVec3_Cross(r, state_out->buoyancy_force);
    
    // Apply angular drag
    UpeVec3 ang_vel = body->angular_velocity;
    float ang_vel_mag = sqrtf(ang_vel.x * ang_vel.x + ang_vel.y * ang_vel.y + ang_vel.z * ang_vel.z);
    if (ang_vel_mag > 1e-6f) {
        float ang_drag = desc->angular_drag * state_out->submerged_ratio * ang_vel_mag;
        state_out->torque.x -= (ang_vel.x / ang_vel_mag) * ang_drag;
        state_out->torque.y -= (ang_vel.y / ang_vel_mag) * ang_drag;
        state_out->torque.z -= (ang_vel.z / ang_vel_mag) * ang_drag;
    }
    
    // Apply forces to body
    body->force.x += state_out->buoyancy_force.x + state_out->drag_force.x;
    body->force.y += state_out->buoyancy_force.y + state_out->drag_force.y;
    body->force.z += state_out->buoyancy_force.z + state_out->drag_force.z;
    
    body->torque.x += state_out->torque.x;
    body->torque.y += state_out->torque.y;
    body->torque.z += state_out->torque.z;
}

// Apply buoyancy from SPH fluid
void UpeBuoyancy_ApplySph(
    UpeBody* body,
    const UpeFluidSph* fluid,
    float drag_coeff,
    UpeBuoyancyState* state_out
) {
    if (!body || !fluid) return;
    
    memset(state_out, 0, sizeof(UpeBuoyancyState));
    
    UpeVec3 center = body->position;
    float radius = UpeBuoyancy_InferEffectiveRadius(body);
    float h = fluid->smoothing_radius;
    
    // Accumulate buoyancy from nearby SPH particles
    for (uint32_t i = 0; i < fluid->count; ++i) {
        const UpeSphParticle* p = &fluid->particles[i];
        
        UpeVec3 r;
        r.x = center.x - p->position.x;
        r.y = center.y - p->position.y;
        r.z = center.z - p->position.z;
        
        float r2 = r.x * r.x + r.y * r.y + r.z * r.z;
        
        if (r2 < (radius + h) * (radius + h)) {
            // Particle is close to body - contribute buoyancy
            float dist = sqrtf(r2);
            float kernel = (dist < h) ? (1.0f - dist / h) : 0.0f;
            
            // Buoyancy force from this particle
            float particle_buoyancy = p->density * p->mass * 9.81f * kernel;
            state_out->buoyancy_force.y += particle_buoyancy;
            
            // Drag from relative velocity
            UpeVec3 rel_vel;
            rel_vel.x = body->velocity.x - p->velocity.x;
            rel_vel.y = body->velocity.y - p->velocity.y;
            rel_vel.z = body->velocity.z - p->velocity.z;
            
            float rel_vel_mag = sqrtf(rel_vel.x * rel_vel.x + rel_vel.y * rel_vel.y + rel_vel.z * rel_vel.z);
            if (rel_vel_mag > 1e-6f) {
                float drag = drag_coeff * p->density * kernel * rel_vel_mag;
                state_out->drag_force.x -= (rel_vel.x / rel_vel_mag) * drag;
                state_out->drag_force.y -= (rel_vel.y / rel_vel_mag) * drag;
                state_out->drag_force.z -= (rel_vel.z / rel_vel_mag) * drag;
            }
        }
    }
    
    // Apply forces
    body->force.x += state_out->buoyancy_force.x + state_out->drag_force.x;
    body->force.y += state_out->buoyancy_force.y + state_out->drag_force.y;
    body->force.z += state_out->buoyancy_force.z + state_out->drag_force.z;
}
