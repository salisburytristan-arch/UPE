#include "upe/upe_pathfinding.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* -----------------------------------------------------------------------
   Helpers
   ----------------------------------------------------------------------- */

static float v3_dist(UpeVec3 a, UpeVec3 b) {
    float dx = a.x-b.x, dy = a.y-b.y, dz = a.z-b.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

static uint32_t find_node_idx(const UpeNavGraph *g, uint32_t id) {
    for (uint32_t i = 0; i < g->node_count; i++)
        if (g->nodes[i].id == id) return i;
    return UPE_PATH_NULL_NODE;
}

/* -----------------------------------------------------------------------
   Graph lifecycle
   ----------------------------------------------------------------------- */

int UpeNavGraph_Init(UpeNavGraph *g, uint32_t max_nodes, uint32_t max_edges) {
    if (!g || max_nodes == 0) return -1;
    memset(g, 0, sizeof(*g));
    g->nodes = (UpeNavNode *)malloc(max_nodes * sizeof(UpeNavNode));
    g->edges = (UpeNavEdge *)malloc(max_edges * sizeof(UpeNavEdge));
    if (!g->nodes || !g->edges) { free(g->nodes); free(g->edges); return -1; }
    g->node_capacity = max_nodes;
    g->edge_capacity = max_edges;
    return 0;
}

void UpeNavGraph_Destroy(UpeNavGraph *g) {
    if (!g) return;
    free(g->nodes); free(g->edges);
    memset(g, 0, sizeof(*g));
}

void UpeNavGraph_Clear(UpeNavGraph *g) {
    if (!g) return;
    g->node_count = 0;
    g->edge_count = 0;
}

int UpeNavGraph_AddNode(UpeNavGraph *g, uint32_t id, UpeVec3 pos, float cost_mod) {
    if (!g || g->node_count >= g->node_capacity) return -1;
    UpeNavNode *n = &g->nodes[g->node_count++];
    n->id = id; n->pos = pos;
    n->cost_modifier = (cost_mod <= 0.0f) ? 1.0f : cost_mod;
    n->passable = 1;
    return 0;
}

int UpeNavGraph_AddEdge(UpeNavGraph *g, uint32_t from_id, uint32_t to_id, float weight) {
    if (!g || g->edge_count >= g->edge_capacity) return -1;
    UpeNavEdge *e = &g->edges[g->edge_count++];
    e->from_id = from_id; e->to_id = to_id; e->weight = weight;
    return 0;
}

int UpeNavGraph_BuildGrid(UpeNavGraph *g, uint32_t nx, uint32_t ny,
                           float cell_size, UpeVec3 origin,
                           const int *passable_flags) {
    if (!g || nx == 0 || ny == 0) return -1;
    UpeNavGraph_Clear(g);
    for (uint32_t y = 0; y < ny; y++)
        for (uint32_t x = 0; x < nx; x++) {
            uint32_t id = y*nx + x;
            UpeVec3 pos = { origin.x + x*cell_size, origin.y, origin.z + y*cell_size };
            if (UpeNavGraph_AddNode(g, id, pos, 1.0f) < 0) return -1;
            if (passable_flags) g->nodes[g->node_count-1].passable = passable_flags[id];
        }
    /* 8-way connectivity */
    int dx8[8] = {-1,0,1,-1,1,-1,0,1};
    int dy8[8] = {-1,-1,-1,0,0,1,1,1};
    for (uint32_t y = 0; y < ny; y++)
        for (uint32_t x = 0; x < nx; x++) {
            uint32_t id = y*nx + x;
            for (int d = 0; d < 8; d++) {
                int nx2 = (int)x + dx8[d];
                int ny2 = (int)y + dy8[d];
                if (nx2 < 0 || ny2 < 0 || (uint32_t)nx2 >= nx || (uint32_t)ny2 >= ny) continue;
                uint32_t nid = (uint32_t)ny2*nx + (uint32_t)nx2;
                float w = (dx8[d] != 0 && dy8[d] != 0) ? cell_size * 1.41421356f : cell_size;
                UpeNavGraph_AddEdge(g, id, nid, w);
            }
        }
    return 0;
}

/* -----------------------------------------------------------------------
   A* search using a simple binary min-heap (open set)
   ----------------------------------------------------------------------- */

typedef struct { float f; uint32_t id; } AStarNode;

static void heap_push(AStarNode *heap, uint32_t *size, AStarNode node) {
    uint32_t i = (*size)++;
    heap[i] = node;
    while (i > 0) {
        uint32_t parent = (i - 1) / 2;
        if (heap[parent].f > heap[i].f) {
            AStarNode tmp = heap[parent]; heap[parent] = heap[i]; heap[i] = tmp;
            i = parent;
        } else break;
    }
}

static AStarNode heap_pop(AStarNode *heap, uint32_t *size) {
    AStarNode r = heap[0];
    heap[0] = heap[--(*size)];
    uint32_t i = 0;
    for (;;) {
        uint32_t l = 2*i+1, ri = 2*i+2, m = i;
        if (l < *size && heap[l].f < heap[m].f) m = l;
        if (ri < *size && heap[ri].f < heap[m].f) m = ri;
        if (m == i) break;
        AStarNode tmp = heap[i]; heap[i] = heap[m]; heap[m] = tmp;
        i = m;
    }
    return r;
}

void UpeNav_Astar(const UpeNavGraph *g, uint32_t start_id, uint32_t goal_id,
                  UpePathResult *result) {
    if (!g || !result) return;
    memset(result, 0, sizeof(*result));

    uint32_t N = g->node_count;
    if (N == 0 || N > UPE_PATH_MAX_NODES) return;

    float   *g_cost = (float  *)malloc(N * sizeof(float));
    uint32_t*prev   = (uint32_t*)malloc(N * sizeof(uint32_t));
    AStarNode*open  = (AStarNode*)malloc((N+1) * sizeof(AStarNode));
    if (!g_cost || !prev || !open) { free(g_cost); free(prev); free(open); return; }

    for (uint32_t i = 0; i < N; i++) { g_cost[i] = FLT_MAX; prev[i] = UPE_PATH_NULL_NODE; }
    uint32_t open_size = 0;

    uint32_t si = find_node_idx(g, start_id);
    uint32_t gi_idx = find_node_idx(g, goal_id);
    if (si == UPE_PATH_NULL_NODE || gi_idx == UPE_PATH_NULL_NODE) {
        free(g_cost); free(prev); free(open); return;
    }
    g_cost[si] = 0.0f;
    AStarNode start_node = { v3_dist(g->nodes[si].pos, g->nodes[gi_idx].pos), start_id };
    heap_push(open, &open_size, start_node);

    while (open_size > 0) {
        AStarNode cur = heap_pop(open, &open_size);
        uint32_t ci = find_node_idx(g, cur.id);
        if (ci == UPE_PATH_NULL_NODE) continue;
        if (cur.id == goal_id) break;

        for (uint32_t e = 0; e < g->edge_count; e++) {
            if (g->edges[e].from_id != cur.id) continue;
            uint32_t ni = find_node_idx(g, g->edges[e].to_id);
            if (ni == UPE_PATH_NULL_NODE || !g->nodes[ni].passable) continue;
            float w = g->edges[e].weight;
            if (w <= 0.0f) w = v3_dist(g->nodes[ci].pos, g->nodes[ni].pos);
            w *= g->nodes[ni].cost_modifier;
            float new_g = g_cost[ci] + w;
            if (new_g < g_cost[ni]) {
                g_cost[ni] = new_g;
                prev[ni]   = ci;
                float h = v3_dist(g->nodes[ni].pos, g->nodes[gi_idx].pos);
                AStarNode nn = { new_g + h, g->nodes[ni].id };
                heap_push(open, &open_size, nn);
            }
        }
    }

    /* Reconstruct path */
    if (g_cost[gi_idx] < FLT_MAX) {
        result->total_cost = g_cost[gi_idx];
        result->found = 1;
        uint32_t cur = gi_idx;
        uint32_t tmp[UPE_PATH_MAX_RESULT];
        uint32_t cnt = 0;
        while (cur != UPE_PATH_NULL_NODE && cnt < UPE_PATH_MAX_RESULT) {
            tmp[cnt++] = g->nodes[cur].id;
            if (g->nodes[cur].id == start_id) break;
            cur = prev[cur];
        }
        /* Reverse */
        for (uint32_t i = 0; i < cnt; i++)
            result->node_ids[i] = tmp[cnt - 1 - i];
        result->count = cnt;
    }
    free(g_cost); free(prev); free(open);
}

void UpeNav_Dijkstra(const UpeNavGraph *g, uint32_t start_id,
                     float *cost_out, uint32_t *prev_out) {
    if (!g || !cost_out) return;
    uint32_t N = g->node_count;
    for (uint32_t i = 0; i < N; i++) { cost_out[i] = FLT_MAX; if(prev_out) prev_out[i] = UPE_PATH_NULL_NODE; }

    AStarNode *open = (AStarNode *)malloc((N+1) * sizeof(AStarNode));
    if (!open) return;
    uint32_t open_size = 0;

    uint32_t si = find_node_idx(g, start_id);
    if (si == UPE_PATH_NULL_NODE) { free(open); return; }
    cost_out[si] = 0.0f;
    AStarNode start = {0.0f, start_id};
    heap_push(open, &open_size, start);

    while (open_size > 0) {
        AStarNode cur = heap_pop(open, &open_size);
        uint32_t ci = find_node_idx(g, cur.id);
        if (ci == UPE_PATH_NULL_NODE) continue;
        for (uint32_t e = 0; e < g->edge_count; e++) {
            if (g->edges[e].from_id != cur.id) continue;
            uint32_t ni = find_node_idx(g, g->edges[e].to_id);
            if (ni == UPE_PATH_NULL_NODE) continue;
            float w = g->edges[e].weight;
            if (w <= 0.0f) w = v3_dist(g->nodes[ci].pos, g->nodes[ni].pos);
            float nc = cost_out[ci] + w;
            if (nc < cost_out[ni]) {
                cost_out[ni] = nc;
                if (prev_out) prev_out[ni] = ci;
                AStarNode next = {nc, g->nodes[ni].id};
                heap_push(open, &open_size, next);
            }
        }
    }
    free(open);
}

/* -----------------------------------------------------------------------
   Flow field
   ----------------------------------------------------------------------- */

int UpeFlowField_Init(UpeFlowField *ff, uint32_t w, uint32_t h,
                       float cell_size, UpeVec3 origin) {
    if (!ff || w == 0 || h == 0) return -1;
    memset(ff, 0, sizeof(*ff));
    ff->width     = w;
    ff->height    = h;
    ff->cell_size = cell_size;
    ff->origin    = origin;
    ff->directions = (UpeVec3 *)malloc(w * h * sizeof(UpeVec3));
    ff->costs      = (float  *)malloc(w * h * sizeof(float));
    if (!ff->directions || !ff->costs) {
        free(ff->directions); free(ff->costs); memset(ff, 0, sizeof(*ff)); return -1;
    }
    return 0;
}

void UpeFlowField_Destroy(UpeFlowField *ff) {
    if (!ff) return;
    free(ff->directions); free(ff->costs);
    memset(ff, 0, sizeof(*ff));
}

void UpeFlowField_Compute(UpeFlowField *ff, uint32_t goal_x, uint32_t goal_y,
                           const int *passable_flags) {
    if (!ff) return;
    uint32_t W = ff->width, H = ff->height, N = W*H;
    for (uint32_t i = 0; i < N; i++) ff->costs[i] = FLT_MAX;

    /* BFS / Dijkstra from goal */
    uint32_t *queue = (uint32_t *)malloc(N * sizeof(uint32_t));
    if (!queue) return;
    uint32_t head = 0, tail = 0;
    uint32_t gi = goal_y * W + goal_x;
    ff->costs[gi] = 0.0f;
    queue[tail++] = gi;

    int dxb[8] = {-1,0,1,-1,1,-1,0,1};
    int dyb[8] = {-1,-1,-1,0,0,1,1,1};

    while (head < tail) {
        uint32_t cur = queue[head++];
        uint32_t cx = cur % W, cy = cur / W;
        for (int d = 0; d < 8; d++) {
            int nx = (int)cx + dxb[d];
            int ny = (int)cy + dyb[d];
            if (nx < 0 || ny < 0 || (uint32_t)nx >= W || (uint32_t)ny >= H) continue;
            uint32_t nid = (uint32_t)ny*W + (uint32_t)nx;
            if (passable_flags && !passable_flags[nid]) continue;
            float step = (dxb[d] != 0 && dyb[d] != 0) ? 1.41421356f : 1.0f;
            float nc = ff->costs[cur] + step;
            if (nc < ff->costs[nid]) {
                ff->costs[nid] = nc;
                queue[tail < N ? tail++ : tail-1] = nid;
            }
        }
    }
    free(queue);

    /* Compute direction vectors (toward min-cost neighbor) */
    for (uint32_t y = 0; y < H; y++)
        for (uint32_t x = 0; x < W; x++) {
            uint32_t id = y*W + x;
            float best = ff->costs[id];
            UpeVec3 dir = {0.0f, 0.0f, 0.0f};
            for (int d = 0; d < 8; d++) {
                int nx = (int)x + dxb[d];
                int ny = (int)y + dyb[d];
                if (nx < 0 || ny < 0 || (uint32_t)nx >= W || (uint32_t)ny >= H) continue;
                float nc = ff->costs[(uint32_t)ny*W + (uint32_t)nx];
                if (nc < best) {
                    best = nc;
                    float len = (dxb[d] != 0 && dyb[d] != 0) ? 1.41421356f : 1.0f;
                    dir.x = (float)dxb[d] / len;
                    dir.z = (float)dyb[d] / len;
                }
            }
            ff->directions[id] = dir;
        }
}

UpeVec3 UpeFlowField_Sample(const UpeFlowField *ff, UpeVec3 world_pos) {
    UpeVec3 zero = {0.0f,0.0f,0.0f};
    if (!ff || ff->cell_size <= 0.0f) return zero;
    int x = (int)((world_pos.x - ff->origin.x) / ff->cell_size);
    int y = (int)((world_pos.z - ff->origin.z) / ff->cell_size);
    if (x < 0 || y < 0 || (uint32_t)x >= ff->width || (uint32_t)y >= ff->height) return zero;
    return ff->directions[(uint32_t)y * ff->width + (uint32_t)x];
}

void UpeNav_SmoothPath(const UpeNavGraph *g, UpePathResult *path) {
    if (!g || !path || path->count < 3) return;
    /* Greedy string-pull: skip waypoints whose segment is clear (simplified) */
    uint32_t new_path[UPE_PATH_MAX_RESULT];
    uint32_t cnt = 0;
    new_path[cnt++] = path->node_ids[0];
    for (uint32_t i = 2; i < path->count; i++) {
        uint32_t ai = find_node_idx(g, new_path[cnt-1]);
        uint32_t bi = find_node_idx(g, path->node_ids[i]);
        if (ai == UPE_PATH_NULL_NODE || bi == UPE_PATH_NULL_NODE) {
            new_path[cnt++] = path->node_ids[i-1];
            continue;
        }
        /* If no direct edge exists, keep intermediate */
        int found_edge = 0;
        for (uint32_t e = 0; e < g->edge_count; e++) {
            if (g->edges[e].from_id == g->nodes[ai].id &&
                g->edges[e].to_id   == g->nodes[bi].id) {
                found_edge = 1; break;
            }
        }
        if (!found_edge) new_path[cnt++] = path->node_ids[i-1];
    }
    new_path[cnt++] = path->node_ids[path->count-1];
    for (uint32_t i = 0; i < cnt; i++) path->node_ids[i] = new_path[i];
    path->count = cnt;
}

/* -----------------------------------------------------------------------
   Steering
   ----------------------------------------------------------------------- */

UpeSteerForce UpeSteer_Seek(UpeVec3 agent_pos, UpeVec3 agent_vel,
                              UpeVec3 target, float max_speed) {
    UpeSteerForce f = {{0.0f,0.0f,0.0f}, 0.0f};
    float dx = target.x - agent_pos.x;
    float dz = target.z - agent_pos.z;
    float dist = sqrtf(dx*dx + dz*dz);
    if (dist < 1e-4f) return f;
    f.linear.x = dx / dist * max_speed - agent_vel.x;
    f.linear.z = dz / dist * max_speed - agent_vel.z;
    return f;
}

UpeSteerForce UpeSteer_Flee(UpeVec3 agent_pos, UpeVec3 agent_vel,
                              UpeVec3 threat, float max_speed) {
    UpeVec3 inv_target = { agent_pos.x*2 - threat.x, agent_pos.y, agent_pos.z*2 - threat.z };
    return UpeSteer_Seek(agent_pos, agent_vel, inv_target, max_speed);
}

UpeSteerForce UpeSteer_Arrive(UpeVec3 agent_pos, UpeVec3 agent_vel,
                               UpeVec3 target, float max_speed, float slow_radius) {
    UpeSteerForce f = {{0.0f,0.0f,0.0f}, 0.0f};
    float dx = target.x - agent_pos.x;
    float dz = target.z - agent_pos.z;
    float dist = sqrtf(dx*dx + dz*dz);
    if (dist < 0.01f) return f;
    float speed = (dist < slow_radius) ? max_speed * (dist / slow_radius) : max_speed;
    float inv = 1.0f / dist;
    f.linear.x = dx * inv * speed - agent_vel.x;
    f.linear.z = dz * inv * speed - agent_vel.z;
    return f;
}

UpeSteerForce UpeSteer_FollowPath(UpeVec3 agent_pos, UpeVec3 agent_vel,
                                   const UpeNavNode *waypoints, uint32_t count,
                                   uint32_t *current_wp_idx, float waypoint_radius,
                                   float max_speed) {
    UpeSteerForce f = {{0.0f,0.0f,0.0f}, 0.0f};
    if (!waypoints || count == 0 || !current_wp_idx) return f;
    /* Advance to next waypoint if close enough */
    if (*current_wp_idx < count) {
        UpeVec3 wp = waypoints[*current_wp_idx].pos;
        float dx = agent_pos.x - wp.x;
        float dz = agent_pos.z - wp.z;
        if (sqrtf(dx*dx + dz*dz) < waypoint_radius && *current_wp_idx + 1 < count)
            (*current_wp_idx)++;
    }
    if (*current_wp_idx >= count) return f;
    return UpeSteer_Seek(agent_pos, agent_vel, waypoints[*current_wp_idx].pos, max_speed);
}
