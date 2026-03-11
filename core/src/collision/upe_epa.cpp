/**
 * Expanding Polytope Algorithm (EPA) for Penetration Depth
 * Computes contact normal and penetration depth for intersecting convex shapes
 * 
 * Algorithm:
 * 1. Start with GJK's final simplex (tetrahedron of 4 points)
 * 2. Iteratively expand polytope outward by adding support points along face normals
 * 3. Stop when no support point extends further (closest face found)
 * 4. Return the normal and depth of the closest face
 */

#include "upe/upe_epa.h"
#include <math.h>
#include <string.h>

#define EPA_MAX_VERTICES 64
#define EPA_MAX_FACES 128
#define EPA_MAX_ITERATIONS 32
#define EPA_TOLERANCE 1e-4f

typedef struct {
    int v[3];           /* Vertex indices */
    UpeVec3 normal;     /* Face normal (pointing outward from origin) */
    float distance;     /* Distance from origin to face */
    int removed;        /* Whether face has been marked for removal */
} EpaFace;

typedef struct {
    UpeVec3 vertices[EPA_MAX_VERTICES];
    EpaFace faces[EPA_MAX_FACES];
    int vertex_count;
    int face_count;
} EpaPolytope;

/**
 * Initialize a face and compute its outward normal
 * Returns 1 if valid, 0 if degenerate
 */
static int EpaFace_Init(EpaFace *face, const UpeVec3 *vertices, int a, int b, int c) {
    face->v[0] = a;
    face->v[1] = b;
    face->v[2] = c;
    face->removed = 0;
    
    /* Compute normal */
    UpeVec3 ab = UpeVec3_Sub(vertices[b], vertices[a]);
    UpeVec3 ac = UpeVec3_Sub(vertices[c], vertices[a]);
    UpeVec3 normal = UpeVec3_Cross(ab, ac);
    
    float len = UpeVec3_Length(normal);
    if (len < EPA_TOLERANCE) {
        return 0;  /* Degenerate */
    }
    
    face->normal = UpeVec3_Scale(normal, 1.0f / len);
    
    /* Ensure normal points away from origin */
    float dot = UpeVec3_Dot(vertices[a], face->normal);
    if (dot < 0.0f) {
        face->normal = UpeVec3_Scale(face->normal, -1.0f);
        dot = -dot;
    }
    
    face->distance = dot;
    return 1;
}

static int EpaPolytope_AddFace(EpaPolytope *poly, int a, int b, int c) {
    if (poly->face_count >= EPA_MAX_FACES) {
        return -1;
    }
    
    if (EpaFace_Init(&poly->faces[poly->face_count], poly->vertices, a, b, c)) {
        return poly->face_count++;
    }
    return -1;
}

static int EpaPolytope_FindClosestFace(EpaPolytope *poly) {
    int closest = -1;
    float min_dist = 1e10f;
    
    for (int i = 0; i < poly->face_count; i++) {
        if (!poly->faces[i].removed && poly->faces[i].distance < min_dist) {
            min_dist = poly->faces[i].distance;
            closest = i;
        }
    }
    
    return closest;
}

UpeEpaResult UpeEpa_Penetration(const void *shape_a, UpeGjkSupportFn support_a,
                                const void *shape_b, UpeGjkSupportFn support_b) {
    UpeEpaResult result = {};
    result.normal = {0.0f, 1.0f, 0.0f};
    result.depth = 0.0f;
    
    /* Run GJK to confirm intersection */
    UpeVec3 initial_dir = {1.0f, 0.0f, 0.0f};
    UpeGjkResult gjk = UpeGjk_Distance(shape_a, support_a, shape_b, support_b, initial_dir);
    
    if (!gjk.intersecting) {
        return result;  /* No intersection */
    }
    
    /* Build initial tetrahedron using support points */
    EpaPolytope poly = {};
    
    UpeVec3 dirs[4] = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {-0.577f, -0.577f, -0.577f}
    };
    
    for (int i = 0; i < 4; i++) {
        UpeGjkSupport sup_a = support_a(shape_a, dirs[i]);
        UpeVec3 neg_dir = UpeVec3_Scale(dirs[i], -1.0f);
        UpeGjkSupport sup_b = support_b(shape_b, neg_dir);
        poly.vertices[i] = UpeVec3_Sub(sup_a.point, sup_b.point);
    }
    poly.vertex_count = 4;
    
    /* Create 4 faces */
    EpaPolytope_AddFace(&poly, 0, 1, 2);
    EpaPolytope_AddFace(&poly, 0, 3, 1);
    EpaPolytope_AddFace(&poly, 0, 2, 3);
    EpaPolytope_AddFace(&poly, 1, 3, 2);
    
    /* Expand polytope */
    for (int iter = 0; iter < EPA_MAX_ITERATIONS; iter++) {
        int closest_idx = EpaPolytope_FindClosestFace(&poly);
        if (closest_idx == -1) break;
        
        EpaFace *closest_face = &poly.faces[closest_idx];
        
        /* Get support point in face normal direction */
        UpeGjkSupport sup_a = support_a(shape_a, closest_face->normal);
        UpeVec3 neg_normal = UpeVec3_Scale(closest_face->normal, -1.0f);
        UpeGjkSupport sup_b = support_b(shape_b, neg_normal);
        UpeVec3 new_support = UpeVec3_Sub(sup_a.point, sup_b.point);
        
        float support_dist = UpeVec3_Dot(new_support, closest_face->normal);
        
        /* Check convergence */
        if (support_dist - closest_face->distance < EPA_TOLERANCE) {
            result.normal = closest_face->normal;
            result.depth = closest_face->distance;
            return result;
        }
        
        /* Add new vertex */
        if (poly.vertex_count >= EPA_MAX_VERTICES) {
            result.normal = closest_face->normal;
            result.depth = closest_face->distance;
            return result;
        }
        
        int new_idx = poly.vertex_count++;
        poly.vertices[new_idx] = new_support;
        
        /* Mark visible faces for removal */
        for (int i = 0; i < poly.face_count; i++) {
            if (!poly.faces[i].removed) {
                UpeVec3 to_new = UpeVec3_Sub(new_support, poly.vertices[poly.faces[i].v[0]]);
                if (UpeVec3_Dot(poly.faces[i].normal, to_new) > EPA_TOLERANCE) {
                    poly.faces[i].removed = 1;
                }
            }
        }
        
        /* Add new faces */
        for (int i = 0; i < poly.face_count; i++) {
            if (poly.faces[i].removed) {
                for (int edge = 0; edge < 3; edge++) {
                    int v0 = poly.faces[i].v[edge];
                    int v1 = poly.faces[i].v[(edge + 1) % 3];
                    
                    /* Check if edge is shared with another removed face */
                    int shared = 0;
                    for (int j = i + 1; j < poly.face_count; j++) {
                        if (poly.faces[j].removed) {
                            for (int e2 = 0; e2 < 3; e2++) {
                                int u0 = poly.faces[j].v[e2];
                                int u1 = poly.faces[j].v[(e2 + 1) % 3];
                                if ((v0 == u1 && v1 == u0) || (v0 == u0 && v1 == u1)) {
                                    shared = 1;
                                    break;
                                }
                            }
                            if (shared) break;
                        }
                    }
                    
                    if (!shared) {
                        EpaPolytope_AddFace(&poly, v0, v1, new_idx);
                    }
                }
            }
        }
    }
    
    /* Return best face found */
    int closest_idx = EpaPolytope_FindClosestFace(&poly);
    if (closest_idx != -1) {
        result.normal = poly.faces[closest_idx].normal;
        result.depth = poly.faces[closest_idx].distance;
    }
    
    return result;
}
