#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Crowd Dynamics & Social Forces
  
  - Agent-based crowd simulation
  - Social force model (Helbing)
  - Density-based flow
  - Panic and evacuation scenarios
  - Formation control
*/

#define UPE_CROWD_MAX_AGENTS 10000

typedef struct UpeCrowdAgent {
    UpeVec3 position;
    UpeVec3 velocity;
    UpeVec3 desired_velocity;    // Where agent wants to go
    UpeVec3 goal;                // Target destination
    float radius;                // Personal space
    float max_speed;
    float mass;
    float panic_level;           // 0 = calm, 1 = panicked
    uint32_t group_id;           // For group behaviors
    uint32_t flags;
} UpeCrowdAgent;

typedef struct UpeCrowdParameters {
    float desired_speed;         // Preferred walking speed (m/s)
    float personal_space;        // Comfortable distance to others (m)
    float relaxation_time;       // Time to reach desired velocity (s)
    float repulsion_strength;    // Social force magnitude
    float attraction_strength;   // Attraction to group members
    float obstacle_force;        // Repulsion from walls
    float max_density;           // People per m² (for density field)
} UpeCrowdParameters;

typedef struct UpeCrowdSystem {
    UpeCrowdAgent *agents;
    uint32_t agent_count;
    uint32_t agent_capacity;
    
    UpeCrowdParameters params;
    
    // Spatial partitioning (for neighbor finding)
    float *density_field;        // Grid of crowd density
    uint32_t grid_width;
    uint32_t grid_height;
    float grid_cell_size;
    
    // Targets/waypoints
    UpeVec3 *waypoints;
    uint32_t waypoint_count;
    
} UpeCrowdSystem;

void UpeCrowd_Init(UpeCrowdSystem *crowd, uint32_t max_agents);
void UpeCrowd_Destroy(UpeCrowdSystem *crowd);

// Agent management
uint32_t UpeCrowd_AddAgent(UpeCrowdSystem *crowd, UpeVec3 position, UpeVec3 goal);
void UpeCrowd_RemoveAgent(UpeCrowdSystem *crowd, uint32_t agent_id);
void UpeCrowd_SetGoal(UpeCrowdSystem *crowd, uint32_t agent_id, UpeVec3 goal);

// Simulation
void UpeCrowd_Step(UpeCrowdSystem *crowd, float dt);

// Social forces
UpeVec3 UpeCrowd_SocialForce(const UpeCrowdAgent *agent_a, const UpeCrowdAgent *agent_b, float strength);
UpeVec3 UpeCrowd_ObstacleForce(const UpeCrowdAgent *agent, UpeVec3 obstacle_point, UpeVec3 normal);
UpeVec3 UpeCrowd_DrivingForce(const UpeCrowdAgent *agent, UpeVec3 desired_direction, float relaxation_time);

// Density field
float UpeCrowd_GetDensityAt(const UpeCrowdSystem *crowd, UpeVec3 position);
void UpeCrowd_UpdateDensityField(UpeCrowdSystem *crowd);

// Pathfinding integration
void UpeCrowd_SetWaypoints(UpeCrowdSystem *crowd, UpeVec3 *waypoints, uint32_t count);
UpeVec3 UpeCrowd_GetNextWaypoint(const UpeCrowdSystem *crowd, uint32_t agent_id);

#ifdef __cplusplus
}
#endif
