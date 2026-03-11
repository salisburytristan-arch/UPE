#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_body.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeClothVertex {
    UpeVec3 position;
    UpeVec3 velocity;
    float inv_mass;
    int pinned;
} UpeClothVertex;

typedef struct UpeClothConstraint {
   uint32_t vertex_a;
    uint32_t vertex_b;
    float rest_length;
    float stiffness;
    int type;                    // 0 = structural, 1 = shear, 2 = bending
    float compliance;            // XPBD: 1/(stiffness * iteration_count)
    float lambda;                // XPBD: Lagrange multiplier for constraint
} UpeClothConstraint;
typedef struct UpeClothWrinkleConstraint {
    uint32_t vertex_a;
    uint32_t vertex_b;
    uint32_t vertex_c;           // Center vertex for wrinkle (angle constraint)
    float rest_angle_cos;         // cos(rest angle)
    float stiffness;
    float compliance;
    float lambda;
} UpeClothWrinkleConstraint;
typedef struct UpeCloth {
    UpeClothVertex *vertices;
    uint32_t vertex_count;
    UpeClothConstraint *constraints;
    uint32_t constraint_count;
    uint32_t constraint_capacity;
    
    // Grid topology (for adjacency checks)
    uint32_t grid_w;
    uint32_t grid_h;
    
    // Self-collision
    int self_collision_enabled;
    float self_collision_radius;
    
    // Tearing
    int tearing_enabled;
    float tear_distance;         // Maximum stretch before tearing
    
    // Wind and aerodynamics
    UpeVec3 wind_velocity;       // Global wind direction and magnitude
    float air_density;           // Air density for drag (typical 1.2 kg/m³)
    float drag_coefficient;      // Drag coefficient for cloth
    
    // XPBD solver features
    UpeClothWrinkleConstraint *wrinkles;
    uint32_t wrinkle_count;
    uint32_t wrinkle_capacity;
    int xpbd_enabled;            // Use XPBD velocity-based solver instead of PBD
    float damping;               // Velocity damping per frame (0.99 = 1% loss)
    float friction_coefficient;  // Friction for object interactions
} UpeCloth;

void UpeCloth_Init(UpeCloth *cloth, uint32_t grid_w, uint32_t grid_h, float spacing);
void UpeCloth_Step(UpeCloth *cloth, float dt, UpeVec3 gravity);
void UpeCloth_Destroy(UpeCloth *cloth);

#ifdef __cplusplus
}
#endif
