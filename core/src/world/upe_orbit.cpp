#include "upe/upe_orbit.h"
#include <math.h>

static const float G = 6.67430e-11f;

void UpeOrbit_Init(UpeOrbit *orbit, UpeVec3 position, UpeVec3 velocity) {
    if (!orbit) {
        return;
    }
    orbit->position = position;
    orbit->velocity = velocity;
    orbit->semi_major_axis = 0.0f;
    orbit->eccentricity = 0.0f;
    orbit->inclination = 0.0f;
}

void UpeOrbit_Step(UpeOrbit *orbit, float dt, float central_mass) {
    if (!orbit) {
        return;
    }
    
    float r = sqrtf(orbit->position.x * orbit->position.x + 
                   orbit->position.y * orbit->position.y + 
                   orbit->position.z * orbit->position.z);
    
    if (r < 1.0e-6f) {
        return;
    }
    
    float force_magnitude = G * central_mass / (r * r);
    UpeVec3 direction = UpeVec3_Scale(orbit->position, -1.0f / r);
    UpeVec3 acceleration = UpeVec3_Scale(direction, force_magnitude);
    
    orbit->velocity = UpeVec3_Add(orbit->velocity, UpeVec3_Scale(acceleration, dt));
    orbit->position = UpeVec3_Add(orbit->position, UpeVec3_Scale(orbit->velocity, dt));
}

void UpeOrbit_FromElements(UpeOrbit *orbit, float a, float e, float i) {
    if (!orbit) {
        return;
    }
    orbit->semi_major_axis = a;
    orbit->eccentricity = e;
    orbit->inclination = i;
    orbit->position = {a * (1.0f - e), 0.0f, 0.0f};
    orbit->velocity = {0.0f, 1.0f, 0.0f};
}
