#pragma once

#include <stdint.h>
#include "upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 2D Body type */
typedef enum Upe2DBodyType {
    UPE_2D_BODY_STATIC = 0,
    UPE_2D_BODY_DYNAMIC = 1,
    UPE_2D_BODY_KINEMATIC = 2
} Upe2DBodyType;

// 2D Shape type
typedef enum Upe2DShapeType {
    UPE_2D_SHAPE_CIRCLE,
    UPE_2D_SHAPE_BOX,
    UPE_2D_SHAPE_POLYGON
} Upe2DShapeType;

// 2D Circle shape
typedef struct Upe2DCircle {
    UpeVec2 center;
    float radius;
} Upe2DCircle;

// 2D Box shape (axis-aligned or rotated)
typedef struct Upe2DBox {
    UpeVec2 center;
    float half_width;
    float half_height;
} Upe2DBox;

// 2D Polygon shape (convex)
typedef struct Upe2DPolygon {
    UpeVec2 vertices[8];  // Max 8 vertices
    uint32_t vertex_count;
} Upe2DPolygon;

// 2D Body
typedef struct Upe2DBody {
    uint32_t id;
    Upe2DBodyType type;
    UpeVec2 position;
    float rotation;           // Radians
    UpeVec2 velocity;
    float angular_velocity;   // Radians/s
    float mass;
    float inv_mass;
    float inertia;
    float inv_inertia;
    float restitution;        // Bounce factor (0-1)
    float friction;           // Friction coefficient
    float linear_damping;     // Velocity damping
    float angular_damping;    // Angular velocity damping
    UpeVec2 force;            // Accumulated force
    float torque;             // Accumulated torque
    uint32_t shape_id;
    int is_asleep;            // Sleep optimization
} Upe2DBody;

// 2D Shape
typedef struct Upe2DShape {
    uint32_t id;
    Upe2DShapeType type;
    uint32_t body_id;
    union {
        Upe2DCircle circle;
        Upe2DBox box;
        Upe2DPolygon polygon;
    };
} Upe2DShape;

// 2D Contact
typedef struct Upe2DContact {
    uint32_t body_a;
    uint32_t body_b;
    UpeVec2 point;
    UpeVec2 normal;
    float penetration;
    float normal_impulse;
    float tangent_impulse;
} Upe2DContact;

// 2D World
typedef struct Upe2DWorld {
    Upe2DBody *bodies;
    uint32_t body_count;
    uint32_t body_capacity;
    
    Upe2DShape *shapes;
    uint32_t shape_count;
    uint32_t shape_capacity;
    
    Upe2DContact *contacts;
    uint32_t contact_count;
    uint32_t contact_capacity;
    
    UpeVec2 gravity;
    float dt;
    uint32_t iteration_count;  // Solver iterations

    uint64_t frame_index;
    struct UpeReplayRecorder *replay;
} Upe2DWorld;

// World management
void Upe2DWorld_Init(Upe2DWorld *world, UpeVec2 gravity);
void Upe2DWorld_Destroy(Upe2DWorld *world);
void Upe2DWorld_Step(Upe2DWorld *world, float dt);
void Upe2DWorld_SetReplayRecorder(Upe2DWorld *world, struct UpeReplayRecorder *replay);

// Body management
uint32_t Upe2DWorld_CreateBody(Upe2DWorld *world, Upe2DBodyType type, UpeVec2 position, float rotation, float mass);
void Upe2DWorld_DestroyBody(Upe2DWorld *world, uint32_t body_id);
Upe2DBody *Upe2DWorld_GetBody(Upe2DWorld *world, uint32_t body_id);

// Shape management
uint32_t Upe2DWorld_CreateCircleShape(Upe2DWorld *world, uint32_t body_id, UpeVec2 center, float radius);
uint32_t Upe2DWorld_CreateBoxShape(Upe2DWorld *world, uint32_t body_id, UpeVec2 center, float half_width, float half_height);
void Upe2DWorld_DestroyShape(Upe2DWorld *world, uint32_t shape_id);

// Force application
void Upe2DBody_ApplyForce(Upe2DBody *body, UpeVec2 force);
void Upe2DBody_ApplyImpulse(Upe2DBody *body, UpeVec2 impulse);
void Upe2DBody_ApplyTorque(Upe2DBody *body, float torque);
void Upe2DWorld_ApplyForce(Upe2DWorld *world, uint32_t body_id, UpeVec2 force);
void Upe2DWorld_ApplyImpulse(Upe2DWorld *world, uint32_t body_id, UpeVec2 impulse);
void Upe2DWorld_ApplyTorque(Upe2DWorld *world, uint32_t body_id, float torque);

// Collision detection
int Upe2D_TestCircleCircle(const Upe2DCircle *a, const Upe2DCircle *b, UpeVec2 *normal, float *penetration);
int Upe2D_TestCircleBox(const Upe2DCircle *circle, const Upe2DBox *box, UpeVec2 *normal, float *penetration);

// Math utilities
UpeVec2 UpeVec2_Add(UpeVec2 a, UpeVec2 b);
UpeVec2 UpeVec2_Sub(UpeVec2 a, UpeVec2 b);
UpeVec2 UpeVec2_Scale(UpeVec2 v, float s);
float UpeVec2_Dot(UpeVec2 a, UpeVec2 b);
float UpeVec2_Length(UpeVec2 v);
UpeVec2 UpeVec2_Normalize(UpeVec2 v);

#ifdef __cplusplus
}
#endif
