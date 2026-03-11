#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Swarm & Boid Dynamics
  
  - Flocking behaviors (Reynolds' boids)
  - Potential field navigation
  - Predator-prey dynamics
  - Emergent group behaviors
*/

#define UPE_SWARM_MAX_BOIDS 50000

typedef struct UpeSwarmBoid {
    UpeVec3 position;
    UpeVec3 velocity;
    UpeVec3 acceleration;
    float max_speed;
    float max_force;
    float perception_radius;     // How far it sees neighbors
    uint32_t flock_id;           // Flock membership
    uint32_t flags;
} UpeSwarmBoid;

typedef struct UpeSwarmParameters {
    float separation_weight;     // Avoid crowding neighbors
    float alignment_weight;      // Align with neighbor velocities
    float cohesion_weight;       // Move toward center of neighbors
    float separation_distance;
    float neighbor_radius;
    float max_speed;
    float max_steering_force;
} UpeSwarmParameters;

typedef struct UpeSwarmPredator {
    UpeVec3 position;
    UpeVec3 velocity;
    float chase_radius;          // How far it chases prey
    float catch_radius;          // Catching distance
} UpeSwarmPredator;

typedef struct UpeSwarmSystem {
    UpeSwarmBoid *boids;
    uint32_t boid_count;
    uint32_t boid_capacity;
    
    UpeSwarmPredator *predators;
    uint32_t predator_count;
    uint32_t predator_capacity;
    
    UpeSw armParameters params;
    
    // Spatial hashing for neighbor queries
    uint32_t *spatial_hash;
    uint32_t hash_size;
    float cell_size;
    
    // Obstacles (potential field)
    UpeVec3 *obstacles;
    float *obstacle_radii;
    uint32_t obstacle_count;
    
} UpeSwarmSystem;

void UpeSwarm_Init(UpeSwarmSystem *swarm, uint32_t max_boids, uint32_t max_predators);
void UpeSwarm_Destroy(UpeSwarmSystem *swarm);

// Boid management
uint32_t UpeSwarm_AddBoid(UpeSwarmSystem *swarm, UpeVec3 position, UpeVec3 velocity);
void UpeSwarm_RemoveBoid(UpeSwarmSystem *swarm, uint32_t boid_id);

// Predator management
uint32_t UpeSwarm_AddPredator(UpeSwarmSystem *swarm, UpeVec3 position);

// Obstacles
void UpeSwarm_AddObstacle(UpeSwarmSystem *swarm, UpeVec3 position, float radius);

// Simulation
void UpeSwarm_Step(UpeSwarmSystem *swarm, float dt);

// Steering behaviors
UpeVec3 UpeSwarm_Separation(const UpeSwarmBoid *boid, const UpeSwarmBoid *neighbors, uint32_t neighbor_count);
UpeVec3 UpeSwarm_Alignment(const UpeSwarmBoid *boid, const UpeSwarmBoid *neighbors, uint32_t neighbor_count);
UpeVec3 UpeSwarm_Cohesion(const UpeSwarmBoid *boid, const UpeSwarmBoid *neighbors, uint32_t neighbor_count);
UpeVec3 UpeSwarm_AvoidObstacles(const UpeSwarmBoid *boid, const UpeSwarmSystem *swarm);
UpeVec3 UpeSwarm_FleeFrom(const UpeSwarmBoid *boid, UpeVec3 threat_position);

// Neighbor finding
uint32_t UpeSwarm_FindNeighbors(const UpeSwarmSystem *swarm, uint32_t boid_id,
                               uint32_t *out_neighbors, uint32_t max_neighbors);

#ifdef __cplusplus
}
#endif
