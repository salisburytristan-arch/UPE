#include "upe/upe_gjk.h"
#include <stdint.h>
#include <math.h>

#define UPE_GJK_MAX_ITERATIONS 64
#define UPE_GJK_EPSILON 1e-6f

typedef struct {
    UpeVec3 points[4];
    float lambdas[4];  /* Barycentric coordinates */
    uint32_t count;
} UpeGjkSimplex;

/* Support point in Minkowski difference */
static UpeVec3 UpeGjk_Support(const void *shape_a, UpeGjkSupportFn support_a,
                               const void *shape_b, UpeGjkSupportFn support_b,
                               UpeVec3 dir) {
    UpeGjkSupport sup_a = support_a(shape_a, dir);
    UpeVec3 neg_dir = {-dir.x, -dir.y, -dir.z};
    UpeGjkSupport sup_b = support_b(shape_b, neg_dir);
    return UpeVec3_Sub(sup_a.point, sup_b.point);
}

/* Closest point on line segment to origin */
static float UpeGjk_SegmentClosestToOrigin(UpeVec3 a, UpeVec3 b, UpeVec3 *closest, float *t_out) {
    UpeVec3 ab = UpeVec3_Sub(b, a);
    UpeVec3 neg_a = {-a.x, -a.y, -a.z};
    float ab_dot_ab = UpeVec3_Dot(ab, ab);
    float ab_dot_neg_a = UpeVec3_Dot(ab, neg_a);
    
    if (ab_dot_ab < UPE_GJK_EPSILON) {
        *closest = a;
        *t_out = 0.0f;
        return UpeVec3_Length(a);
    }
    
    float t = ab_dot_neg_a / ab_dot_ab;
    if (t < 0.0f) {
        *closest = a;
        *t_out = 0.0f;
    } else if (t > 1.0f) {
        *closest = b;
        *t_out = 1.0f;
    } else {
        *closest = UpeVec3_Add(a, UpeVec3_Scale(ab, t));
        *t_out = t;
    }
    
    return UpeVec3_Length(*closest);
}

/* Closest point on triangle to origin */
static float UpeGjk_TriangleClosestToOrigin(UpeVec3 a, UpeVec3 b, UpeVec3 c,
                                            UpeVec3 *closest, float *lambdas_out) {
    /* Compute edge vectors */
    UpeVec3 ab = UpeVec3_Sub(b, a);
    UpeVec3 ac = UpeVec3_Sub(c, a);
    UpeVec3 neg_a = {-a.x, -a.y, -a.z};
    
    /* Check if origin projects onto vertex A */
    float ab_dot_neg_a = UpeVec3_Dot(ab, neg_a);
    float ac_dot_neg_a = UpeVec3_Dot(ac, neg_a);
    if (ab_dot_neg_a <= 0.0f && ac_dot_neg_a <= 0.0f) {
        *closest = a;
        lambdas_out[0] = 1.0f;
        lambdas_out[1] = 0.0f;
        lambdas_out[2] = 0.0f;
        return UpeVec3_Length(a);
    }
    
    /* Check edge AB */
    UpeVec3 closest_ab;
    float t_ab;
    float dist_ab = UpeGjk_SegmentClosestToOrigin(a, b, &closest_ab, &t_ab);
    
    /* Check edge AC */
    UpeVec3 closest_ac;
    float t_ac;
    float dist_ac = UpeGjk_SegmentClosestToOrigin(a, c, &closest_ac, &t_ac);
    
    /* Check if origin projects onto face */
    UpeVec3 normal = UpeVec3_Cross(ab, ac);
    float normal_len_sq = UpeVec3_Dot(normal, normal);
    if (normal_len_sq > UPE_GJK_EPSILON) {
        float normal_dot_a = UpeVec3_Dot(normal, a);
        if (fabsf(normal_dot_a) < UPE_GJK_EPSILON) {
            /* Origin is on plane - check if it's inside triangle */
            /* Use barycentric coordinates */
            UpeVec3 bc_ab = UpeVec3_Cross(neg_a, ac);
            UpeVec3 bc_ac = UpeVec3_Cross(ab, neg_a);
            float lambda_b = UpeVec3_Dot(bc_ab, normal) / normal_len_sq;
            float lambda_c = UpeVec3_Dot(bc_ac, normal) / normal_len_sq;
            float lambda_a = 1.0f - lambda_b - lambda_c;
            
            if (lambda_a >= -UPE_GJK_EPSILON && lambda_b >= -UPE_GJK_EPSILON && 
                lambda_c >= -UPE_GJK_EPSILON) {
                /* Origin projects onto face interior */
                *closest = a;  /* Origin is on plane */
                lambdas_out[0] = lambda_a;
                lambdas_out[1] = lambda_b;
                lambdas_out[2] = lambda_c;
                return 0.0f;  /* Distance is zero */
            }
        }
    }
    
    /* Best point is on edge or vertex */
    if (dist_ab < dist_ac) {
        *closest = closest_ab;
        lambdas_out[0] = 1.0f - t_ab;
        lambdas_out[1] = t_ab;
        lambdas_out[2] = 0.0f;
        return dist_ab;
    } else {
        *closest = closest_ac;
        lambdas_out[0] = 1.0f - t_ac;
        lambdas_out[1] = 0.0f;
        lambdas_out[2] = t_ac;
        return dist_ac;
    }
}

/* Update simplex to contain closest point to origin */
static int UpeGjk_UpdateSimplex(UpeGjkSimplex *simplex, UpeVec3 *new_dir) {
    switch (simplex->count) {
        case 1: {
            /* Single point */
            *new_dir = UpeVec3_Scale(simplex->points[0], -1.0f);
            return 0;
        }
        case 2: {
            /* Line segment */
            UpeVec3 closest;
            float t;
            UpeGjk_SegmentClosestToOrigin(simplex->points[0], simplex->points[1], &closest, &t);
            
            if (t < UPE_GJK_EPSILON) {
                simplex->points[0] = simplex->points[0];
                simplex->count = 1;
            } else if (t > 1.0f - UPE_GJK_EPSILON) {
                simplex->points[0] = simplex->points[1];
                simplex->count = 1;
            }
            
            *new_dir = UpeVec3_Scale(closest, -1.0f);
            float len = UpeVec3_Length(*new_dir);
            if (len < UPE_GJK_EPSILON) return 1;  /* Intersection */
            return 0;
        }
        case 3: {
            /* Triangle */
            UpeVec3 closest;
            float lambdas[3];
            float dist = UpeGjk_TriangleClosestToOrigin(
                simplex->points[0], simplex->points[1], simplex->points[2],
                &closest, lambdas);
            
            if (dist < UPE_GJK_EPSILON) return 1;  /* Intersection */
            
            /* Remove points with zero lambda */
            UpeGjkSimplex new_simplex = {};
            for (int i = 0; i < 3; i++) {
                if (lambdas[i] > UPE_GJK_EPSILON) {
                    new_simplex.points[new_simplex.count] = simplex->points[i];
                    new_simplex.lambdas[new_simplex.count] = lambdas[i];
                    new_simplex.count++;
                }
            }
            *simplex = new_simplex;
            
            *new_dir = UpeVec3_Scale(closest, -1.0f);
            return 0;
        }
        case 4: {
            /* Tetrahedron - shapes intersect */
            return 1;
        }
    }
    return 0;
}

UpeGjkResult UpeGjk_Distance(const void *shape_a, UpeGjkSupportFn support_a,
                             const void *shape_b, UpeGjkSupportFn support_b,
                             UpeVec3 initial_dir) {
    UpeGjkResult result = {};
    result.distance = 1000.0f;
    result.intersecting = 0;

    /* Normalize initial direction */
    UpeVec3 dir = initial_dir;
    float dir_len = UpeVec3_Length(dir);
    if (dir_len < UPE_GJK_EPSILON) {
        dir = {1.0f, 0.0f, 0.0f};
    } else {
        dir = UpeVec3_Scale(dir, 1.0f / dir_len);
    }

    UpeGjkSimplex simplex = {};

    for (int iter = 0; iter < UPE_GJK_MAX_ITERATIONS; iter++) {
        /* Get new support point */
        UpeVec3 support = UpeGjk_Support(shape_a, support_a, shape_b, support_b, dir);
        
        /* Compute distance from origin to new point in search direction */
        float dist_progress = UpeVec3_Dot(support, dir);
        
        /* Check convergence */
        if (iter > 0 && (result.distance - dist_progress) < UPE_GJK_EPSILON) {
            break;
        }
        
        result.distance = UpeVec3_Length(support);
        
        if (result.distance < UPE_GJK_EPSILON) {
            result.intersecting = 1;
            break;
        }
        
        /* Add point to simplex */
        simplex.points[simplex.count] = support;
        simplex.count++;
        
        /* Update simplex and get new search direction */
        if (UpeGjk_UpdateSimplex(&simplex, &dir)) {
            result.intersecting = 1;
            break;
        }
        
        /* Normalize direction */
        float new_dir_len = UpeVec3_Length(dir);
        if (new_dir_len < UPE_GJK_EPSILON) {
            break;
        }
        dir = UpeVec3_Scale(dir, 1.0f / new_dir_len);
    }

    /* Compute witness points from simplex */
    if (simplex.count == 1) {
        result.witness_a = simplex.points[0];
        result.witness_b = {0.0f, 0.0f, 0.0f};
    } else if (simplex.count == 2) {
        /* Closest point on line segment */
        float t = UpeVec3_Dot(simplex.points[1], UpeVec3_Sub(simplex.points[1], simplex.points[0])) /
                 UpeVec3_Dot(UpeVec3_Sub(simplex.points[1], simplex.points[0]),
                            UpeVec3_Sub(simplex.points[1], simplex.points[0]));
        t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
        result.witness_a = UpeVec3_Add(simplex.points[0],
                                      UpeVec3_Scale(UpeVec3_Sub(simplex.points[1], simplex.points[0]), t));
        result.witness_b = {0.0f, 0.0f, 0.0f};
    }

    return result;
}
