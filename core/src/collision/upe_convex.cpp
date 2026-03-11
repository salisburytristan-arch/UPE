#include "upe/upe_convex.h"
#include "upe/upe_transform.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

typedef struct UpeConvexSupportPair {
    UpeVec3 a;
    UpeVec3 b;
    UpeVec3 minkowski;
} UpeConvexSupportPair;

typedef struct UpeEpaFace {
    int v[3];
    UpeVec3 normal;
    float distance;
    int removed;
} UpeEpaFace;

#define UPE_EPA_MAX_VERTICES 64
#define UPE_EPA_MAX_FACES 128
#define UPE_EPA_TOLERANCE 1.0e-4f

typedef struct UpeEpaPolytope {
    UpeConvexSupportPair vertices[UPE_EPA_MAX_VERTICES];
    UpeEpaFace faces[UPE_EPA_MAX_FACES];
    uint32_t vertex_count;
    uint32_t face_count;
} UpeEpaPolytope;

static UpeVec3 UpeConvex_NormalizeSafe(UpeVec3 v, UpeVec3 fallback) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 1.0e-6f) {
        return fallback;
    }
    float inv = 1.0f / len;
    UpeVec3 out;
    out.x = v.x * inv;
    out.y = v.y * inv;
    out.z = v.z * inv;
    return out;
}

static UpeVec3 UpeConvex_ConjugateRotate(UpeQuat q, UpeVec3 v) {
    UpeQuat inv;
    inv.x = -q.x;
    inv.y = -q.y;
    inv.z = -q.z;
    inv.w = q.w;
    return UpeQuat_RotateVec3(inv, v);
}

static UpeConvexSupportPair UpeConvex_SupportWorld(
    const void *shape_a, UpeSupportFunction support_a, UpeTransform3 xform_a,
    const void *shape_b, UpeSupportFunction support_b, UpeTransform3 xform_b,
    UpeVec3 direction_world) {

    UpeVec3 dir_world = UpeConvex_NormalizeSafe(direction_world, {1.0f, 0.0f, 0.0f});
    UpeVec3 dir_a_local = UpeConvex_ConjugateRotate(xform_a.rotation, dir_world);
    UpeVec3 dir_b_world = UpeConvex_ConjugateRotate(xform_b.rotation, dir_world);
    UpeVec3 dir_b_local;
    dir_b_local.x = -dir_b_world.x;
    dir_b_local.y = -dir_b_world.y;
    dir_b_local.z = -dir_b_world.z;

    UpeVec3 local_a = support_a(shape_a, dir_a_local);
    UpeVec3 local_b = support_b(shape_b, dir_b_local);

    UpeConvexSupportPair pair;
    pair.a = UpeTransform3_TransformPoint(xform_a, local_a);
    pair.b = UpeTransform3_TransformPoint(xform_b, local_b);
    pair.minkowski.x = pair.a.x - pair.b.x;
    pair.minkowski.y = pair.a.y - pair.b.y;
    pair.minkowski.z = pair.a.z - pair.b.z;
    return pair;
}

static int UpeEpaFace_Init(UpeEpaFace *face, UpeConvexSupportPair *vertices, int a, int b, int c) {
    face->v[0] = a;
    face->v[1] = b;
    face->v[2] = c;
    face->removed = 0;

    UpeVec3 pa = vertices[a].minkowski;
    UpeVec3 pb = vertices[b].minkowski;
    UpeVec3 pc = vertices[c].minkowski;

    UpeVec3 ab;
    ab.x = pb.x - pa.x;
    ab.y = pb.y - pa.y;
    ab.z = pb.z - pa.z;
    UpeVec3 ac;
    ac.x = pc.x - pa.x;
    ac.y = pc.y - pa.y;
    ac.z = pc.z - pa.z;

    UpeVec3 normal = UpeVec3_Cross(ab, ac);
    float nlen = UpeVec3_Length(normal);
    if (nlen < UPE_EPA_TOLERANCE) {
        return 0;
    }

    normal = UpeVec3_Scale(normal, 1.0f / nlen);
    float distance = UpeVec3_Dot(pa, normal);
    if (distance < 0.0f) {
        normal = UpeVec3_Scale(normal, -1.0f);
        distance = -distance;
        int tmp = face->v[1];
        face->v[1] = face->v[2];
        face->v[2] = tmp;
    }

    face->normal = normal;
    face->distance = distance;
    return 1;
}

static int UpeEpaPoly_AddFace(UpeEpaPolytope *poly, int a, int b, int c) {
    if (poly->face_count >= UPE_EPA_MAX_FACES) {
        return -1;
    }
    if (!UpeEpaFace_Init(&poly->faces[poly->face_count], poly->vertices, a, b, c)) {
        return -1;
    }
    return (int)poly->face_count++;
}

static int UpeEpaPoly_FindClosestFace(UpeEpaPolytope *poly) {
    int closest = -1;
    float min_distance = FLT_MAX;
    for (uint32_t i = 0; i < poly->face_count; i++) {
        if (!poly->faces[i].removed && poly->faces[i].distance < min_distance) {
            min_distance = poly->faces[i].distance;
            closest = (int)i;
        }
    }
    return closest;
}

static void UpeEpaEdge_AddUnique(int (*edges)[2], uint32_t *edge_count, int a, int b) {
    for (uint32_t i = 0; i < *edge_count; i++) {
        if (edges[i][0] == b && edges[i][1] == a) {
            edges[i][0] = edges[*edge_count - 1][0];
            edges[i][1] = edges[*edge_count - 1][1];
            (*edge_count)--;
            return;
        }
    }
    edges[*edge_count][0] = a;
    edges[*edge_count][1] = b;
    (*edge_count)++;
}

UpeConvexHull* UpeConvexHull_Create(uint32_t max_vertices) {
    UpeConvexHull *hull = (UpeConvexHull*)malloc(sizeof(UpeConvexHull));
    if (!hull) return NULL;
    
    hull->vertices = (UpeVec3*)malloc(max_vertices * sizeof(UpeVec3));
    hull->vertex_count = 0;
    hull->centroid.x = hull->centroid.y = hull->centroid.z = 0.0f;
    hull->bounding_radius = 0.0f;
    
    return hull;
}

void UpeConvexHull_Destroy(UpeConvexHull *hull) {
    if (!hull) return;
    free(hull->vertices);
    free(hull);
}

void UpeConvexHull_AddVertex(UpeConvexHull *hull, UpeVec3 vertex) {
    if (!hull) return;
    hull->vertices[hull->vertex_count++] = vertex;
}

void UpeConvexHull_ComputeProperties(UpeConvexHull *hull) {
    if (!hull || hull->vertex_count == 0) return;
    
    // Compute centroid
    hull->centroid.x = hull->centroid.y = hull->centroid.z = 0.0f;
    for (uint32_t i = 0; i < hull->vertex_count; i++) {
        hull->centroid.x += hull->vertices[i].x;
        hull->centroid.y += hull->vertices[i].y;
        hull->centroid.z += hull->vertices[i].z;
    }
    float inv_count = 1.0f / hull->vertex_count;
    hull->centroid.x *= inv_count;
    hull->centroid.y *= inv_count;
    hull->centroid.z *= inv_count;
    
    // Compute bounding radius
    hull->bounding_radius = 0.0f;
    for (uint32_t i = 0; i < hull->vertex_count; i++) {
        float dx = hull->vertices[i].x - hull->centroid.x;
        float dy = hull->vertices[i].y - hull->centroid.y;
        float dz = hull->vertices[i].z - hull->centroid.z;
        float dist_sq = dx * dx + dy * dy + dz * dz;
        if (dist_sq > hull->bounding_radius * hull->bounding_radius) {
            hull->bounding_radius = sqrtf(dist_sq);
        }
    }
}

UpeVec3 UpeSupport_ConvexHull(const void *shape, UpeVec3 direction) {
    const UpeConvexHull *hull = (const UpeConvexHull*)shape;
    
    float max_dot = -FLT_MAX;
    UpeVec3 support_point = hull->vertices[0];
    
    for (uint32_t i = 0; i < hull->vertex_count; i++) {
        UpeVec3 v = hull->vertices[i];
        float dot = v.x * direction.x + v.y * direction.y + v.z * direction.z;
        if (dot > max_dot) {
            max_dot = dot;
            support_point = v;
        }
    }
    
    return support_point;
}

// Simple GJK implementation (distance/separation test)
UpeGjkResult UpeGjk_Test(
    const void *shape_a, UpeSupportFunction support_a, UpeTransform3 xform_a,
    const void *shape_b, UpeSupportFunction support_b, UpeTransform3 xform_b) {
    
    UpeGjkResult result;
    result.intersecting = 0;
    result.distance = 0.0f;
    result.iterations = 0;
    
    // Initial direction
    UpeVec3 direction;
    direction.x = xform_b.position.x - xform_a.position.x;
    direction.y = xform_b.position.y - xform_a.position.y;
    direction.z = xform_b.position.z - xform_a.position.z;
    direction = UpeConvex_NormalizeSafe(direction, {1.0f, 0.0f, 0.0f});
    
    // GJK simplex (up to 4 points)
    UpeVec3 simplex[4];
    uint32_t simplex_size = 0;
    UpeVec3 last_a = xform_a.position;
    UpeVec3 last_b = xform_b.position;
    
    for (uint32_t iter = 0; iter < UPE_GJK_MAX_ITERATIONS; iter++) {
        result.iterations = iter + 1;
        
        // Get support point in world-space Minkowski difference
        UpeConvexSupportPair support = UpeConvex_SupportWorld(
            shape_a, support_a, xform_a,
            shape_b, support_b, xform_b,
            direction);
        last_a = support.a;
        last_b = support.b;
        
        // Check if we passed the origin
        float dot = support.minkowski.x * direction.x + support.minkowski.y * direction.y + support.minkowski.z * direction.z;
        if (dot < 0.0f) {
            // Shapes are separated
            result.intersecting = 0;
            result.distance = -dot;
            result.normal = direction;
            result.closest_a = last_a;
            result.closest_b = last_b;
            return result;
        }
        
        // Add to simplex
        simplex[simplex_size++] = support.minkowski;
        
        // Simplified: if we have 4 points, assume intersection
        if (simplex_size >= 4) {
            result.intersecting = 1;
            result.closest_a = last_a;
            result.closest_b = last_b;
            return result;
        }
        
        // Update direction (simplified: toward origin)
        direction.x = -support.minkowski.x;
        direction.y = -support.minkowski.y;
        direction.z = -support.minkowski.z;
        float dir_len = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
        if (dir_len < 1e-6f) {
            result.intersecting = 1;
            result.closest_a = last_a;
            result.closest_b = last_b;
            return result;
        }
        direction.x /= dir_len;
        direction.y /= dir_len;
        direction.z /= dir_len;
    }
    
    result.intersecting = 0;
    result.closest_a = last_a;
    result.closest_b = last_b;
    return result;
}

UpeConvexCollisionResult UpeConvex_TestCollision(
    const void *shape_a, UpeSupportFunction support_a, UpeTransform3 xform_a,
    const void *shape_b, UpeSupportFunction support_b, UpeTransform3 xform_b) {
    
    UpeConvexCollisionResult result;
    memset(&result, 0, sizeof(result));
    
    UpeGjkResult gjk = UpeGjk_Test(shape_a, support_a, xform_a, shape_b, support_b, xform_b);
    
    result.colliding = gjk.intersecting;
    if (!gjk.intersecting) {
        result.normal = gjk.normal;
        result.depth = gjk.distance;
        result.contact_point = UpeVec3_Scale(UpeVec3_Add(gjk.closest_a, gjk.closest_b), 0.5f);
        return result;
    }

    UpeVec3 simplex_seed[4] = {};
    UpeEpaResult epa = UpeEpa_ComputePenetration(
        shape_a, support_a, xform_a,
        shape_b, support_b, xform_b,
        simplex_seed, 0);

    result.normal = epa.normal;
    result.depth = epa.depth;
    result.contact_point = epa.contact_point;
    
    return result;
}

UpeEpaResult UpeEpa_ComputePenetration(
    const void *shape_a, UpeSupportFunction support_a, UpeTransform3 xform_a,
    const void *shape_b, UpeSupportFunction support_b, UpeTransform3 xform_b,
    UpeVec3 *gjk_simplex, uint32_t simplex_count) {

    UpeEpaResult result = {};
    result.normal = {0.0f, 1.0f, 0.0f};
    result.contact_point = UpeVec3_Scale(UpeVec3_Add(xform_a.position, xform_b.position), 0.5f);
    result.depth = 0.0f;
    result.iterations = 0;

    UpeEpaPolytope poly = {};

    if (gjk_simplex && simplex_count >= 4) {
        uint32_t seed_count = simplex_count < 4 ? simplex_count : 4;
        for (uint32_t i = 0; i < seed_count; i++) {
            UpeVec3 fallback_dir = {1.0f, 0.0f, 0.0f};
            UpeVec3 dir = UpeConvex_NormalizeSafe(gjk_simplex[i], fallback_dir);
            poly.vertices[poly.vertex_count++] = UpeConvex_SupportWorld(
                shape_a, support_a, xform_a,
                shape_b, support_b, xform_b,
                dir);
        }
    }

    if (poly.vertex_count < 4) {
        UpeVec3 dirs[4] = {
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f},
            {-0.577f, -0.577f, -0.577f}
        };
        poly.vertex_count = 0;
        for (int i = 0; i < 4; i++) {
            poly.vertices[poly.vertex_count++] = UpeConvex_SupportWorld(
                shape_a, support_a, xform_a,
                shape_b, support_b, xform_b,
                dirs[i]);
        }
    }

    UpeEpaPoly_AddFace(&poly, 0, 1, 2);
    UpeEpaPoly_AddFace(&poly, 0, 3, 1);
    UpeEpaPoly_AddFace(&poly, 0, 2, 3);
    UpeEpaPoly_AddFace(&poly, 1, 3, 2);

    for (uint32_t iter = 0; iter < UPE_EPA_MAX_ITERATIONS; iter++) {
        result.iterations = iter + 1;
        int closest = UpeEpaPoly_FindClosestFace(&poly);
        if (closest < 0) {
            break;
        }

        UpeEpaFace *face = &poly.faces[closest];
        UpeConvexSupportPair support = UpeConvex_SupportWorld(
            shape_a, support_a, xform_a,
            shape_b, support_b, xform_b,
            face->normal);

        float support_dist = UpeVec3_Dot(support.minkowski, face->normal);
        if ((support_dist - face->distance) < UPE_EPA_TOLERANCE) {
            result.normal = face->normal;
            result.depth = face->distance;
            UpeVec3 pa = UpeConvex_SupportWorld(
                shape_a, support_a, xform_a,
                shape_b, support_b, xform_b,
                face->normal).a;
            UpeVec3 neg_normal = UpeVec3_Scale(face->normal, -1.0f);
            UpeVec3 pb = UpeConvex_SupportWorld(
                shape_a, support_a, xform_a,
                shape_b, support_b, xform_b,
                neg_normal).b;
            result.contact_point = UpeVec3_Scale(UpeVec3_Add(pa, pb), 0.5f);
            return result;
        }

        if (poly.vertex_count >= UPE_EPA_MAX_VERTICES) {
            result.normal = face->normal;
            result.depth = face->distance;
            return result;
        }

        int new_index = (int)poly.vertex_count++;
        poly.vertices[new_index] = support;

        int boundary[UPE_EPA_MAX_FACES * 3][2];
        uint32_t boundary_count = 0;

        for (uint32_t i = 0; i < poly.face_count; i++) {
            UpeEpaFace *candidate = &poly.faces[i];
            if (candidate->removed) {
                continue;
            }

            UpeVec3 to_new = UpeVec3_Sub(support.minkowski, poly.vertices[candidate->v[0]].minkowski);
            if (UpeVec3_Dot(candidate->normal, to_new) > UPE_EPA_TOLERANCE) {
                candidate->removed = 1;
                UpeEpaEdge_AddUnique(boundary, &boundary_count, candidate->v[0], candidate->v[1]);
                UpeEpaEdge_AddUnique(boundary, &boundary_count, candidate->v[1], candidate->v[2]);
                UpeEpaEdge_AddUnique(boundary, &boundary_count, candidate->v[2], candidate->v[0]);
            }
        }

        uint32_t write_face = 0;
        for (uint32_t i = 0; i < poly.face_count; i++) {
            if (!poly.faces[i].removed) {
                poly.faces[write_face++] = poly.faces[i];
            }
        }
        poly.face_count = write_face;

        for (uint32_t i = 0; i < boundary_count; i++) {
            UpeEpaPoly_AddFace(&poly, boundary[i][0], boundary[i][1], new_index);
        }
    }

    int closest = UpeEpaPoly_FindClosestFace(&poly);
    if (closest >= 0) {
        UpeEpaFace *face = &poly.faces[closest];
        result.normal = face->normal;
        result.depth = face->distance;
        UpeVec3 pa = UpeConvex_SupportWorld(
            shape_a, support_a, xform_a,
            shape_b, support_b, xform_b,
            face->normal).a;
        UpeVec3 neg_normal = UpeVec3_Scale(face->normal, -1.0f);
        UpeVec3 pb = UpeConvex_SupportWorld(
            shape_a, support_a, xform_a,
            shape_b, support_b, xform_b,
            neg_normal).b;
        result.contact_point = UpeVec3_Scale(UpeVec3_Add(pa, pb), 0.5f);
    }

    return result;
}
