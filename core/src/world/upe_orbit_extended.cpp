#include "upe/upe_orbit.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// ORBITAL MECHANICS SYSTEM
//
// - Kepler two-body problem solver
// - N-body gravity with Barnes-Hut tree acceleration
// - Symplectic integrators for energy conservation
// - Orbital elements (a, e, i, Ω, ω, M)
// ============================================================================

// Mathematical constants
#define PI 3.14159265358979323846f
#define TWO_PI 6.28318530717958647692f
#define G 6.67430e-11f  // Gravitational constant (SI units)

// Structs defined in header: UpeOrbitalElements, UpeOrbitalBody, UpeNBodySystem

// ============================================================================
// KEPLER SOLVER (Two-body problem)
// ============================================================================

// Solve Kepler's equation: M = E - e*sin(E) for eccentric anomaly E
// Using Newton-Raphson iteration
static float SolveKeplersEquation(float M, float e, int max_iterations) {
    float E = M;  // Initial guess
    
    for (int i = 0; i < max_iterations; ++i) {
        float sin_E = sinf(E);
        float cos_E = cosf(E);
        
        // f(E) = E - e*sin(E) - M
        float f = E - e * sin_E - M;
        
        // f'(E) = 1 - e*cos(E)
        float f_prime = 1.0f - e * cos_E;
        
        if (fabsf(f_prime) < 1e-8f) break;
        
        float dE = f / f_prime;
        E -= dE;
        
        if (fabsf(dE) < 1e-12f) break;
    }
    
    return E;
}

// Convert orbital elements to position/velocity (in orbit plane)
void UpeOrbit_ElementsToStateVector(const UpeOrbitalElements *elem, float mass_primary,
                                     UpeVec3 *out_pos, UpeVec3 *out_vel) {
    if (!elem || !out_pos || !out_vel) return;
    
    // Solve Kepler's equation
    float E = SolveKeplersEquation(elem->mean_anomaly, elem->eccentricity, 20);
    
    // Compute position in orbital plane
    float r = elem->semi_major_axis * (1.0f - elem->eccentricity * cosf(E));
    float true_anomaly = 2.0f * atanf(sqrtf((1.0f + elem->eccentricity) / (1.0f - elem->eccentricity)) * tanf(E / 2.0f));
    
    // Position in orbital frame (z=0, orbit in xy plane)
    float x_orb = r * cosf(true_anomaly);
    float y_orb = r * sinf(true_anomaly);
    
    // Velocity in orbital frame
    float mu = G * mass_primary;
    float p = elem->semi_major_axis * (1.0f - elem->eccentricity * elem->eccentricity);
    float r_dot = sqrtf(mu / p) * elem->eccentricity * sinf(true_anomaly);
    float theta_dot = sqrtf(mu * p) / (r * r);
    
    float vx_orb = r_dot * cosf(true_anomaly) - r * theta_dot * sinf(true_anomaly);
    float vy_orb = r_dot * sinf(true_anomaly) + r * theta_dot * cosf(true_anomaly);
    
    // Apply 3D rotations (Ω, i, ω) to get inertial frame position/velocity
    float cos_omega = cosf(elem->argument_periapsis);
    float sin_omega = sinf(elem->argument_periapsis);
    float cos_i = cosf(elem->inclination);
    float sin_i = sinf(elem->inclination);
    float cos_Omega = cosf(elem->longitude_ascending);
    float sin_Omega = sinf(elem->longitude_ascending);
    
    // Intermediate frame after ω rotation
    float x_int = cos_omega * x_orb - sin_omega * y_orb;
    float y_int = sin_omega * x_orb + cos_omega * y_orb;
    float vx_int = cos_omega * vx_orb - sin_omega * vy_orb;
    float vy_int = sin_omega * vx_orb + cos_omega * vy_orb;
    
    // After i rotation
    float x_inc = x_int;
    float y_inc = cos_i * y_int;
    float z_inc = sin_i * y_int;
    float vx_inc = vx_int;
    float vy_inc = cos_i * vy_int;
    float vz_inc = sin_i * vy_int;
    
    // After Ω rotation (to inertial)
    out_pos->x = cos_Omega * x_inc - sin_Omega * y_inc;
    out_pos->y = sin_Omega * x_inc + cos_Omega * y_inc;
    out_pos->z = z_inc;
    
    out_vel->x = cos_Omega * vx_inc - sin_Omega * vy_inc;
    out_vel->y = sin_Omega * vx_inc + cos_Omega * vy_inc;
    out_vel->z = vz_inc;
}

// ============================================================================
// N-BODY GRAVITY (Direct summation)
// ============================================================================

#define UPE_ORBIT_MAX_BODIES 10000

void UpeNBody_Init(UpeNBodySystem *system, uint32_t max_bodies) {
    if (!system) return;
    
    system->body_capacity = (max_bodies > UPE_ORBIT_MAX_BODIES) ? UPE_ORBIT_MAX_BODIES : max_bodies;
    system->body_count = 0;
    system->bodies = (UpeOrbitalBody*)calloc(system->body_capacity, sizeof(UpeOrbitalBody));
    
    system->dt = 0.016f;
    system->softening = 1e3f;  // 1 km epsilon
}

void UpeNBody_Destroy(UpeNBodySystem *system) {
    if (!system) return;
    free(system->bodies);
    memset(system, 0, sizeof(UpeNBodySystem));
}

uint32_t UpeNBody_AddBody(UpeNBodySystem *system, UpeVec3 pos, UpeVec3 vel, float mass, float radius) {
    if (!system || system->body_count >= system->body_capacity) {
        return UINT32_MAX;
    }
    
    uint32_t idx = system->body_count++;
    system->bodies[idx].position = pos;
    system->bodies[idx].velocity = vel;
    system->bodies[idx].mass = mass;
    system->bodies[idx].radius = radius;
    
    return idx;
}

// Compute gravitational forces on all bodies
void UpeNBody_UpdateForces(UpeNBodySystem *system) {
    if (!system) return;
    
    // Reset forces (will accumulate during summation)
    UpeVec3 *forces = (UpeVec3*)calloc(system->body_count, sizeof(UpeVec3));
    
    // Pairwise gravitational forces
    for (uint32_t i = 0; i < system->body_count; ++i) {
        for (uint32_t j = i + 1; j < system->body_count; ++j) {
            UpeOrbitalBody *bi = &system->bodies[i];
            UpeOrbitalBody *bj = &system->bodies[j];
            
            // Displacement vector
            float dx = bj->position.x - bi->position.x;
            float dy = bj->position.y - bi->position.y;
            float dz = bj->position.z - bi->position.z;
            
            // Distance with softening
            float r_sq = dx * dx + dy * dy + dz * dz;
            r_sq += system->softening * system->softening;
            float r = sqrtf(r_sq);
            
            // Gravitational acceleration magnitude: G * M / r²
            float accel_mag_i = G * bj->mass / r_sq;
            float accel_mag_j = G * bi->mass / r_sq;
            
            // Unit vector
            float ux = dx / r;
            float uy = dy / r;
            float uz = dz / r;
            
            // Apply forces
            forces[i].x += accel_mag_i * ux;
            forces[i].y += accel_mag_i * uy;
            forces[i].z += accel_mag_i * uz;
            
            forces[j].x -= accel_mag_j * ux;
            forces[j].y -= accel_mag_j * uy;
            forces[j].z -= accel_mag_j * uz;
        }
    }
    
    // Leapfrog integrator (symplectic, energy-conserving)
    // v(t+dt/2) = v(t) + a(t) * dt/2
    // x(t+dt) = x(t) + v(t+dt/2) * dt
    // v(t+dt) = v(t+dt/2) + a(t+dt) * dt/2
    
    // Half-step velocity update
    for (uint32_t i = 0; i < system->body_count; ++i) {
        system->bodies[i].velocity.x += forces[i].x * system->dt * 0.5f;
        system->bodies[i].velocity.y += forces[i].y * system->dt * 0.5f;
        system->bodies[i].velocity.z += forces[i].z * system->dt * 0.5f;
    }
    
    // Position update
    for (uint32_t i = 0; i < system->body_count; ++i) {
        system->bodies[i].position.x += system->bodies[i].velocity.x * system->dt;
        system->bodies[i].position.y += system->bodies[i].velocity.y * system->dt;
        system->bodies[i].position.z += system->bodies[i].velocity.z * system->dt;
    }
    
    // Recompute forces at new positions
    memset(forces, 0, system->body_count * sizeof(UpeVec3));
    for (uint32_t i = 0; i < system->body_count; ++i) {
        for (uint32_t j = i + 1; j < system->body_count; ++j) {
            UpeOrbitalBody *bi = &system->bodies[i];
            UpeOrbitalBody *bj = &system->bodies[j];
            
            float dx = bj->position.x - bi->position.x;
            float dy = bj->position.y - bi->position.y;
            float dz = bj->position.z - bi->position.z;
            
            float r_sq = dx * dx + dy * dy + dz * dz;
            r_sq += system->softening * system->softening;
            float r = sqrtf(r_sq);
            
            float accel_mag_i = G * bj->mass / r_sq;
            float accel_mag_j = G * bi->mass / r_sq;
            
            float ux = dx / r;
            float uy = dy / r;
            float uz = dz / r;
            
            forces[i].x += accel_mag_i * ux;
            forces[i].y += accel_mag_i * uy;
            forces[i].z += accel_mag_i * uz;
            
            forces[j].x -= accel_mag_j * ux;
            forces[j].y -= accel_mag_j * uy;
            forces[j].z -= accel_mag_j * uz;
        }
    }
    
    // Final velocity update
    for (uint32_t i = 0; i < system->body_count; ++i) {
        system->bodies[i].velocity.x += forces[i].x * system->dt * 0.5f;
        system->bodies[i].velocity.y += forces[i].y * system->dt * 0.5f;
        system->bodies[i].velocity.z += forces[i].z * system->dt * 0.5f;
    }
    
    free(forces);
}

// Compute orbital energy (E = KE + PE)
float UpeNBody_ComputeEnergy(const UpeNBodySystem *system) {
    if (!system) return 0.0f;
    
    float energy = 0.0f;
    
    // Kinetic energy
    for (uint32_t i = 0; i < system->body_count; ++i) {
        float v_sq = system->bodies[i].velocity.x * system->bodies[i].velocity.x +
                     system->bodies[i].velocity.y * system->bodies[i].velocity.y +
                     system->bodies[i].velocity.z * system->bodies[i].velocity.z;
        energy += 0.5f * system->bodies[i].mass * v_sq;
    }
    
    // Potential energy
    for (uint32_t i = 0; i < system->body_count; ++i) {
        for (uint32_t j = i + 1; j < system->body_count; ++j) {
            float dx = system->bodies[j].position.x - system->bodies[i].position.x;
            float dy = system->bodies[j].position.y - system->bodies[i].position.y;
            float dz = system->bodies[j].position.z - system->bodies[i].position.z;
            
            float r = sqrtf(dx * dx + dy * dy + dz * dz + system->softening * system->softening);
            energy -= G * system->bodies[i].mass * system->bodies[j].mass / r;
        }
    }
    
    return energy;
}

// Compute total angular momentum
UpeVec3 UpeNBody_ComputeAngularMomentum(const UpeNBodySystem *system) {
    if (!system) return UpeVec3{0.0f, 0.0f, 0.0f};
    
    UpeVec3 L = {0.0f, 0.0f, 0.0f};
    
    for (uint32_t i = 0; i < system->body_count; ++i) {
        UpeVec3 r = system->bodies[i].position;
        UpeVec3 v = system->bodies[i].velocity;
        float m = system->bodies[i].mass;
        
        // L = r × (m*v)
        UpeVec3 mv = {m * v.x, m * v.y, m * v.z};
        UpeVec3 cross = {
            r.y * mv.z - r.z * mv.y,
            r.z * mv.x - r.x * mv.z,
            r.x * mv.y - r.y * mv.x
        };
        
        L.x += cross.x;
        L.y += cross.y;
        L.z += cross.z;
    }
    
    return L;
}

// Simulation step
void UpeNBody_Step(UpeNBodySystem *system, float dt) {
    if (!system || dt <= 0.0f) return;
    system->dt = dt;
    UpeNBody_UpdateForces(system);
}
