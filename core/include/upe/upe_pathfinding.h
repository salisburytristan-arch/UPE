#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Pathfinding (Math/CS — Phase 10)

  Navigation graph construction and traversal:
    - A* search on arbitrary node graphs (grid or navmesh)
    - Dijkstra (all-shortest-paths variant)
    - Steering: seek, flee, arrive, path follow
    - Flow field generation for crowd-scale pathfinding
    - Path smoothing (string-pulling / funnel algorithm)
    - Jump Point Search (JPS) speedup hint flag

  All structures are POD; caller owns node/edge arrays.
  No dynamic allocations after Init.
*/

#define UPE_PATH_MAX_NODES  4096
#define UPE_PATH_NULL_NODE  (~(uint32_t)0)
#define UPE_PATH_MAX_RESULT 512

/* ---- Navigation node ---- */
typedef struct UpeNavNode {
    UpeVec3  pos;
    uint32_t id;
    float    cost_modifier;  /* terrain cost multiplier (1.0 = normal) */
    int      passable;
} UpeNavNode;

/* ---- Navigation edge ---- */
typedef struct UpeNavEdge {
    uint32_t from_id;
    uint32_t to_id;
    float    weight;    /* 0 = use Euclidean distance */
} UpeNavEdge;

/* ---- Navigation graph ---- */
typedef struct UpeNavGraph {
    UpeNavNode *nodes;
    uint32_t    node_count;
    uint32_t    node_capacity;

    UpeNavEdge *edges;
    uint32_t    edge_count;
    uint32_t    edge_capacity;
} UpeNavGraph;

/* ---- Path result ---- */
typedef struct UpePathResult {
    uint32_t node_ids[UPE_PATH_MAX_RESULT];
    uint32_t count;
    float    total_cost;
    int      found;
} UpePathResult;

/* ---- Flow field (grid-based) ---- */
typedef struct UpeFlowField {
    UpeVec3  *directions;      /* normalized direction per cell */
    float    *costs;           /* Dijkstra cost to goal per cell */
    uint32_t  width;
    uint32_t  height;
    float     cell_size;
    UpeVec3   origin;
} UpeFlowField;

/* ---- Steering output ---- */
typedef struct UpeSteerForce {
    UpeVec3 linear;   /* desired velocity change */
    float   angular;  /* yaw adjustment (rad/s) */
} UpeSteerForce;

/* --- Graph lifecycle --- */
int  UpeNavGraph_Init(UpeNavGraph *g, uint32_t max_nodes, uint32_t max_edges);
void UpeNavGraph_Destroy(UpeNavGraph *g);
void UpeNavGraph_Clear(UpeNavGraph *g);

/* --- Add node / edge --- */
int UpeNavGraph_AddNode(UpeNavGraph *g, uint32_t id, UpeVec3 pos, float cost_mod);
int UpeNavGraph_AddEdge(UpeNavGraph *g, uint32_t from_id, uint32_t to_id, float weight);

/* --- Grid graph builder: creates nx×ny grid with 8-way connectivity --- */
int UpeNavGraph_BuildGrid(UpeNavGraph *g, uint32_t nx, uint32_t ny,
                           float cell_size, UpeVec3 origin,
                           const int *passable_flags);   /* ny*nx booleans */

/* --- A* search ---
   Returns path in result->node_ids[0..count-1].  result->found = 1 on success. */
void UpeNav_Astar(const UpeNavGraph *g, uint32_t start_id, uint32_t goal_id,
                  UpePathResult *result);

/* --- Dijkstra: fills cost array (caller provides, size = node_count) --- */
void UpeNav_Dijkstra(const UpeNavGraph *g, uint32_t start_id,
                     float *cost_out, uint32_t *prev_out);

/* --- Flow field: compute Dijkstra from goal cell outward, fill directions --- */
int  UpeFlowField_Init(UpeFlowField *ff, uint32_t w, uint32_t h,
                        float cell_size, UpeVec3 origin);
void UpeFlowField_Destroy(UpeFlowField *ff);
void UpeFlowField_Compute(UpeFlowField *ff, uint32_t goal_x, uint32_t goal_y,
                           const int *passable_flags);
UpeVec3 UpeFlowField_Sample(const UpeFlowField *ff, UpeVec3 world_pos);

/* --- Path smoothing: removes unnecessary waypoints (string-pull) --- */
void UpeNav_SmoothPath(const UpeNavGraph *g, UpePathResult *path);

/* --- Steering behaviors --- */
UpeSteerForce UpeSteer_Seek(UpeVec3 agent_pos, UpeVec3 agent_vel,
                              UpeVec3 target, float max_speed);
UpeSteerForce UpeSteer_Flee(UpeVec3 agent_pos, UpeVec3 agent_vel,
                              UpeVec3 threat, float max_speed);
UpeSteerForce UpeSteer_Arrive(UpeVec3 agent_pos, UpeVec3 agent_vel,
                               UpeVec3 target, float max_speed, float slow_radius);
UpeSteerForce UpeSteer_FollowPath(UpeVec3 agent_pos, UpeVec3 agent_vel,
                                   const UpeNavNode *waypoints, uint32_t count,
                                   uint32_t *current_wp_idx, float waypoint_radius,
                                   float max_speed);

#ifdef __cplusplus
}
#endif
