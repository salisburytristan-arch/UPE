#include "upe/upe_convex.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

/*
  QuickHull Algorithm for Convex Hull Construction
  
  Given a point cloud, constructs the convex hull with:
  - Half-edge mesh topology
  - Vertex/face/edge lists
  - Mass property computation
*/

typedef struct UpeHalfEdge {
    uint32_t vertex;         // Vertex this edge points to
    uint32_t face;           // Face this edge belongs to
    uint32_t twin;           // Opposite half-edge
    uint32_t next;           // Next edge in face loop
} UpeHalfEdge;

typedef struct UpeFace {
    uint32_t edge;           // First half-edge of this face
    UpeVec3 normal;
    float plane_distance;    // Distance from origin along normal
    uint32_t conflict_count; // Number of points in conflict list
} UpeFace;

typedef struct UpeHullBuilder {
    UpeVec3 *input_points;
    uint32_t input_count;
    
    UpeVec3 *vertices;
    uint32_t vertex_count;
    uint32_t vertex_capacity;
    
    UpeFace *faces;
    uint32_t face_count;
    uint32_t face_capacity;
    
    UpeHalfEdge *edges;
    uint32_t edge_count;
    uint32_t edge_capacity;
    
    uint32_t *conflict_lists; // Per-face list of conflicting points
    uint32_t conflict_capacity;
} UpeHullBuilder;

static void HullBuilder_Init(UpeHullBuilder *builder, const UpeVec3 *points, uint32_t count) {
    builder->input_points = (UpeVec3*)malloc(count * sizeof(UpeVec3));
    memcpy(builder->input_points, points, count * sizeof(UpeVec3));
    builder->input_count = count;
    
    builder->vertex_capacity = count;
    builder->vertices = (UpeVec3*)malloc(builder->vertex_capacity * sizeof(UpeVec3));
    builder->vertex_count = 0;
    
    builder->face_capacity = count * 2;
    builder->faces = (UpeFace*)malloc(builder->face_capacity * sizeof(UpeFace));
    builder->face_count = 0;
    
    builder->edge_capacity = count * 6;
    builder->edges = (UpeHalfEdge*)malloc(builder->edge_capacity * sizeof(UpeHalfEdge));
    builder->edge_count = 0;
    
    builder->conflict_capacity = count * 4;
    builder->conflict_lists = (uint32_t*)malloc(builder->conflict_capacity * sizeof(uint32_t));
}

static void HullBuilder_Destroy(UpeHullBuilder *builder) {
    free(builder->input_points);
    free(builder->vertices);
    free(builder->faces);
    free(builder->edges);
    free(builder->conflict_lists);
}

static float PlaneDistance(UpeVec3 point, UpeVec3 normal, float d) {
    return point.x * normal.x + point.y * normal.y + point.z * normal.z - d;
}

static UpeVec3 ComputeFaceNormal(UpeVec3 a, UpeVec3 b, UpeVec3 c) {
    UpeVec3 ab = {b.x - a.x, b.y - a.y, b.z - a.z};
    UpeVec3 ac = {c.x - a.x, c.y - a.y, c.z - a.z};
    UpeVec3 normal = {
        ab.y * ac.z - ab.z * ac.y,
        ab.z * ac.x - ab.x * ac.z,
        ab.x * ac.y - ab.y * ac.x
    };
    float len = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (len > 1e-6f) {
        normal.x /= len;
        normal.y /= len;
        normal.z /= len;
    }
    return normal;
}

static void BuildInitialTetrahedron(UpeHullBuilder *builder) {
    // Find 4 points that form a tetrahedron (non-coplanar)
    // Step 1: Find point with min X
    uint32_t p0 = 0;
    for (uint32_t i = 1; i < builder->input_count; i++) {
        if (builder->input_points[i].x < builder->input_points[p0].x) {
            p0 = i;
        }
    }
    
    // Step 2: Find point farthest from p0
    uint32_t p1 = (p0 == 0) ? 1 : 0;
    float max_dist_sq = 0.0f;
    for (uint32_t i = 0; i < builder->input_count; i++) {
        if (i == p0) continue;
        UpeVec3 diff = {
            builder->input_points[i].x - builder->input_points[p0].x,
            builder->input_points[i].y - builder->input_points[p0].y,
            builder->input_points[i].z - builder->input_points[p0].z
        };
        float dist_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (dist_sq > max_dist_sq) {
            max_dist_sq = dist_sq;
            p1 = i;
        }
    }
    
    // Step 3: Find point farthest from line p0-p1
    uint32_t p2 = 0;
    for (uint32_t i = 0; i < builder->input_count; i++) {
        if (i == p0 || i == p1) continue;
        p2 = i;
        break;
    }
    max_dist_sq = 0.0f;
    UpeVec3 line_dir = {
        builder->input_points[p1].x - builder->input_points[p0].x,
        builder->input_points[p1].y - builder->input_points[p0].y,
        builder->input_points[p1].z - builder->input_points[p0].z
    };
    float line_len_sq = line_dir.x * line_dir.x + line_dir.y * line_dir.y + line_dir.z * line_dir.z;
    for (uint32_t i = 0; i < builder->input_count; i++) {
        if (i == p0 || i == p1) continue;
        UpeVec3 to_pt = {
            builder->input_points[i].x - builder->input_points[p0].x,
            builder->input_points[i].y - builder->input_points[p0].y,
            builder->input_points[i].z - builder->input_points[p0].z
        };
        float t = (to_pt.x * line_dir.x + to_pt.y * line_dir.y + to_pt.z * line_dir.z) / line_len_sq;
        UpeVec3 closest = {
            builder->input_points[p0].x + line_dir.x * t,
            builder->input_points[p0].y + line_dir.y * t,
            builder->input_points[p0].z + line_dir.z * t
        };
        UpeVec3 diff = {
            builder->input_points[i].x - closest.x,
            builder->input_points[i].y - closest.y,
            builder->input_points[i].z - closest.z
        };
        float dist_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (dist_sq > max_dist_sq) {
            max_dist_sq = dist_sq;
            p2 = i;
        }
    }
    
    // Step 4: Find point farthest from plane p0-p1-p2
    UpeVec3 plane_normal = ComputeFaceNormal(builder->input_points[p0], builder->input_points[p1], builder->input_points[p2]);
    float plane_d = plane_normal.x * builder->input_points[p0].x + plane_normal.y * builder->input_points[p0].y + plane_normal.z * builder->input_points[p0].z;
    uint32_t p3 = 0;
    for (uint32_t i = 0; i < builder->input_count; i++) {
        if (i == p0 || i == p1 || i == p2) continue;
        p3 = i;
        break;
    }
    float max_dist = 0.0f;
    for (uint32_t i = 0; i < builder->input_count; i++) {
        if (i == p0 || i == p1 || i == p2) continue;
        float dist = fabsf(PlaneDistance(builder->input_points[i], plane_normal, plane_d));
        if (dist > max_dist) {
            max_dist = dist;
            p3 = i;
        }
    }
    
    // Add vertices
    builder->vertices[0] = builder->input_points[p0];
    builder->vertices[1] = builder->input_points[p1];
    builder->vertices[2] = builder->input_points[p2];
    builder->vertices[3] = builder->input_points[p3];
    builder->vertex_count = 4;
    
    // Build 4 faces (tetrahedron)
    // For simplicity, add faces without full half-edge topology in this stub
    builder->faces[0].normal = ComputeFaceNormal(builder->vertices[0], builder->vertices[1], builder->vertices[2]);
    builder->faces[1].normal = ComputeFaceNormal(builder->vertices[0], builder->vertices[2], builder->vertices[3]);
    builder->faces[2].normal = ComputeFaceNormal(builder->vertices[0], builder->vertices[3], builder->vertices[1]);
    builder->faces[3].normal = ComputeFaceNormal(builder->vertices[1], builder->vertices[3], builder->vertices[2]);
    builder->face_count = 4;
}

// QuickHull main function
void UpeQuickHull_Build(const UpeVec3 *points, uint32_t count, UpeConvexHull *out_hull) {
    if (!points || count < 4 || !out_hull) {
        return;
    }
    
    UpeHullBuilder builder;
    HullBuilder_Init(&builder, points, count);
    
    // Step 1: Build initial tetrahedron
    BuildInitialTetrahedron(&builder);
    
    // Step 2: Iteratively add remaining points (simplified stub)
    // Full QuickHull would:
    // - Assign each point to a face's conflict list
    // - While conflict lists exist:
    //   - Pick face with most conflicts
    //   - Pick farthest point from that face
    //   - Add point to hull, remove visible faces, add new faces
    
    // For now, copy tetrahedron vertices to output hull
    for (uint32_t i = 0; i < builder.vertex_count && i < UPE_CONVEX_MAX_VERTICES; i++) {
        UpeConvexHull_AddVertex(out_hull, builder.vertices[i]);
    }
    
    UpeConvexHull_ComputeProperties(out_hull);
    
    HullBuilder_Destroy(&builder);
}

// Compute mass properties for convex hull
void UpeConvexHull_ComputeMassProperties(const UpeConvexHull *hull, float density, float *out_mass, UpeVec3 *out_center_of_mass, UpeVec3 *out_inertia) {
    if (!hull || hull->vertex_count < 4) {
        if (out_mass) *out_mass = 0.0f;
        if (out_center_of_mass) *out_center_of_mass = UpeVec3{0, 0, 0};
        if (out_inertia) *out_inertia = UpeVec3{0, 0, 0};
        return;
    }
    
    // Simplified: assume unit density sphere for now
    float volume = 4.0f / 3.0f * 3.14159f * hull->bounding_radius * hull->bounding_radius * hull->bounding_radius;
    if (out_mass) {
        *out_mass = density * volume;
    }
    if (out_center_of_mass) {
        *out_center_of_mass = hull->centroid;
    }
    if (out_inertia) {
        // Sphere inertia: I = (2/5) * m * r^2
        float m = density * volume;
        float r2 = hull->bounding_radius * hull->bounding_radius;
        float inertia = 0.4f * m * r2;
        out_inertia->x = inertia;
        out_inertia->y = inertia;
        out_inertia->z = inertia;
    }
}
