#include "upe/upe_epa.h"
#include <math.h>

#define EPA_MAX_FACES 64
#define EPA_MAX_ITERATIONS 32
#define EPA_TOLERANCE 0.0001f

typedef struct UpeEpaFace {
    int v[3];  // Indices into vertex array
    UpeVec3 normal;
    float distance;  // Distance from origin to face
} UpeEpaFace;

typedef struct UpeEpaPoly {
    UpeVec3 vertices[EPA_MAX_FACES * 3];
    UpeEpaFace faces[EPA_MAX_FACES];
    int vertex_count;
    int face_count;
} UpeEpaPoly;

static float UpeEpa_TriangleDistToOrigin(UpeVec3 a, UpeVec3 b, UpeVec3 c, UpeVec3 *normal_out) {
    UpeVec3 ab = UpeVec3_Sub(b, a);
    UpeVec3 ac = UpeVec3_Sub(c, a);
    UpeVec3 normal = UpeVec3_Cross(ab, ac);
    float len = UpeVec3_Length(normal);
    if (len < 1e-6f) {
        *normal_out = {0.0f, 1.0f, 0.0f};
        return 0.0f;
    }
    normal = UpeVec3_Scale(normal, 1.0f / len);
    *normal_out = normal;
    return fabsf(UpeVec3_Dot(a, normal));
}

static int UpeEpa_AddFace(UpeEpaPoly *poly, int a, int b, int c) {
    if (poly->face_count >= EPA_MAX_FACES) {
        return -1;
    }
    UpeEpaFace *face = &poly->faces[poly->face_count];
    face->v[0] = a;
    face->v[1] = b;
    face->v[2] = c;
    face->distance = UpeEpa_TriangleDistToOrigin(
        poly->vertices[a], poly->vertices[b], poly->vertices[c], &face->normal);
    return poly->face_count++;
}

static int UpeEpa_FindClosestFace(UpeEpaPoly *poly) {
    int closest = 0;
    float min_dist = poly->faces[0].distance;
    for (int i = 1; i < poly->face_count; ++i) {
        if (poly->faces[i].distance < min_dist) {
            min_dist = poly->faces[i].distance;
            closest = i;
        }
    }
    return closest;
}

UpeEpaResult UpeEpa_Penetration(const void *shape_a, UpeGjkSupportFn support_a,
                                const void *shape_b, UpeGjkSupportFn support_b) {
    UpeEpaResult result = {};
    
    // First run GJK to check if shapes intersect
    UpeGjkResult gjk = UpeGjk_Distance(shape_a, support_a, shape_b, support_b, {1.0f, 0.0f, 0.0f});
    if (!gjk.intersecting) {
        // Shapes not intersecting
        result.normal = {0.0f, 1.0f, 0.0f};
        result.depth = 0.0f;
        return result;
    }
    
    // Build initial tetrahedron for EPA
    // Start with support points in cardinal directions
    UpeVec3 dirs[4] = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {-0.577f, -0.577f, -0.577f}  // Opposite corner direction
    };
    
    UpeEpaPoly poly = {};
    for (int i = 0; i < 4; ++i) {
        UpeGjkSupport sup_a = support_a(shape_a, dirs[i]);
        UpeVec3 neg_dir = UpeVec3_Scale(dirs[i], -1.0f);
        UpeGjkSupport sup_b = support_b(shape_b, neg_dir);
        poly.vertices[poly.vertex_count++] = UpeVec3_Sub(sup_a.point, sup_b.point);
    }
    
    // Create initial faces (tetrahedron has 4 triangular faces)
    UpeEpa_AddFace(&poly, 0, 1, 2);
    UpeEpa_AddFace(&poly, 0, 3, 1);
    UpeEpa_AddFace(&poly, 0, 2, 3);
    UpeEpa_AddFace(&poly, 1, 3, 2);
    
    // Iteratively expand polytope towards origin
    for (int iteration = 0; iteration < EPA_MAX_ITERATIONS; ++iteration) {
        // Find closest face to origin
        int closest_face = UpeEpa_FindClosestFace(&poly);
        UpeEpaFace *face = &poly.faces[closest_face];
        
        // Get support point in direction of face normal
        UpeGjkSupport sup_a = support_a(shape_a, face->normal);
        UpeVec3 neg_normal = UpeVec3_Scale(face->normal, -1.0f);
        UpeGjkSupport sup_b = support_b(shape_b, neg_normal);
        UpeVec3 support_point = UpeVec3_Sub(sup_a.point, sup_b.point);
        
        float support_dist = UpeVec3_Dot(support_point, face->normal);
        
        // Check if we've found the closest face (can't expand further)
        if (support_dist - face->distance < EPA_TOLERANCE) {
            result.normal = face->normal;
            result.depth = support_dist;
            return result;
        }
        
        // Add new vertex
        if (poly.vertex_count >= EPA_MAX_FACES * 3) {
            // Out of vertex space, return current best
            result.normal = face->normal;
            result.depth = face->distance;
            return result;
        }
        int new_vertex = poly.vertex_count++;
        poly.vertices[new_vertex] = support_point;
        
        // Rebuild polytope by removing faces visible from new point and adding new faces
        // This is a simplified implementation - a full EPA would handle edge connectivity
        // For now, just replace the closest face with 3 new faces connected to the new vertex
        
        // Save face vertices before removing
        int v0 = face->v[0];
        int v1 = face->v[1];
        int v2 = face->v[2];
        
        // Remove closest face by replacing with last face
        poly.faces[closest_face] = poly.faces[--poly.face_count];
        
        // Add 3 new faces
        UpeEpa_AddFace(&poly, v0, v1, new_vertex);
        UpeEpa_AddFace(&poly, v1, v2, new_vertex);
        UpeEpa_AddFace(&poly, v2, v0, new_vertex);
    }
    
    // Max iterations reached, return best result
    int closest_face = UpeEpa_FindClosestFace(&poly);
    result.normal = poly.faces[closest_face].normal;
    result.depth = poly.faces[closest_face].distance;
    return result;
}
