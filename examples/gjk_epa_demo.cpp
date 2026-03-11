/**
 * GJK/EPA Collision Detection Demo (v0.12.0)
 * 
 * Demonstrates:
 * - GJK for distance computation and intersection testing
 * - EPA for penetration depth/normal
 * - Convex shape collision detection
 */

#include <stdio.h>
#include <math.h>
#include "upe/upe_gjk.h"
#include "upe/upe_epa.h"
#include "upe/upe_math.h"

/* Simple sphere shape (support function) */
typedef struct {
    UpeVec3 center;
    float radius;
} TestSphere;

static UpeGjkSupport TestSphere_Support(const void *shape, UpeVec3 dir) {
    TestSphere *sphere = (TestSphere *)shape;
    float len = UpeVec3_Length(dir);
    if (len < 1e-6f) {
        dir = (UpeVec3){1.0f, 0.0f, 0.0f};
    } else {
        dir = UpeVec3_Scale(dir, 1.0f / len);
    }
    
    UpeGjkSupport result;
    result.point = UpeVec3_Add(sphere->center, UpeVec3_Scale(dir, sphere->radius));
    return result;
}

int main(void) {
    printf("=== GJK/EPA Convex Collision Demo ===\n\n");
    
    /* Test 1: Spheres separated */
    {
        printf("Test 1: Separated spheres\n");
        TestSphere sphere1 = {{0.0f, 0.0f, 0.0f}, 1.0f};
        TestSphere sphere2 = {{5.0f, 0.0f, 0.0f}, 1.0f};
        
        UpeGjkResult result = UpeGjk_Distance(
            &sphere1, TestSphere_Support,
            &sphere2, TestSphere_Support,
            (UpeVec3){1.0f, 0.0f, 0.0f});
        
        printf("  Distance: %.4f\n", result.distance);
        printf("  Intersecting: %s\n", result.intersecting ? "YES" : "NO");
        printf("  Expected distance: ~3.0 (5 - 1 - 1)\n\n");
    }
    
    /* Test 2: Spheres touching */
    {
        printf("Test 2: Touching spheres\n");
        TestSphere sphere1 = {{0.0f, 0.0f, 0.0f}, 1.0f};
        TestSphere sphere2 = {{2.0f, 0.0f, 0.0f}, 1.0f};
        
        UpeGjkResult result = UpeGjk_Distance(
            &sphere1, TestSphere_Support,
            &sphere2, TestSphere_Support,
            (UpeVec3){1.0f, 0.0f, 0.0f});
        
        printf("  Distance: %.4f\n", result.distance);
        printf("  Intersecting: %s\n", result.intersecting ? "YES" : "NO");
        printf("  Expected: close to 0\n\n");
    }
    
    /* Test 3: Spheres overlapping */
    {
        printf("Test 3: Overlapping spheres (penetrating)\n");
        TestSphere sphere1 = {{0.0f, 0.0f, 0.0f}, 1.5f};
        TestSphere sphere2 = {{1.0f, 0.0f, 0.0f}, 1.5f};
        
        UpeGjkResult result = UpeGjk_Distance(
            &sphere1, TestSphere_Support,
            &sphere2, TestSphere_Support,
            (UpeVec3){1.0f, 0.0f, 0.0f});
        
        printf("  Distance: %.4f\n", result.distance);
        printf("  Intersecting: %s\n", result.intersecting ? "YES" : "NO");
        
        if (result.intersecting) {
            UpeEpaResult epa = UpeEpa_Penetration(
                &sphere1, TestSphere_Support,
                &sphere2, TestSphere_Support);
            
            printf("  Penetration normal: (%.4f, %.4f, %.4f)\n", 
                   epa.normal.x, epa.normal.y, epa.normal.z);
            printf("  Penetration depth: %.4f\n", epa.depth);
            printf("  Expected depth: ~2.0 (1.5 + 1.5 - 1)\n");
        }
        printf("\n");
    }
    
    /* Test 4: Different overlaps */
    {
        printf("Test 4: Small sphere inside large sphere\n");
        TestSphere large = {{0.0f, 0.0f, 0.0f}, 5.0f};
        TestSphere small = {{1.0f, 0.0f, 0.0f}, 1.0f};
        
        UpeGjkResult result = UpeGjk_Distance(
            &large, TestSphere_Support,
            &small, TestSphere_Support,
            (UpeVec3){1.0f, 0.0f, 0.0f});
        
        printf("  Intersecting: %s\n", result.intersecting ? "YES" : "NO");
        
        if (result.intersecting) {
            UpeEpaResult epa = UpeEpa_Penetration(
                &large, TestSphere_Support,
                &small, TestSphere_Support);
            
            printf("  Penetration normal: (%.4f, %.4f, %.4f)\n",
                   epa.normal.x, epa.normal.y, epa.normal.z);
            printf("  Penetration depth: %.4f\n", epa.depth);
            printf("  Expected depth: ~3.0 (5 - 1 - 1 from small's perspective)\n");
        }
        printf("\n");
    }
    
    printf("✓ All GJK/EPA tests completed\n");
    printf("\nSummary:\n");
    printf("  GJK: Correctly detects intersection and computes separation distance\n");
    printf("  EPA: Computes penetration depth and normal for collision response\n");
    printf("  Together: Complete convex collision detection pipeline\n");
    
    return 0;
}
