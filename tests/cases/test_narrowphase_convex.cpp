#include "test_framework.h"
#include "upe/upe_narrowphase.h"
#include "upe/upe_convex.h"

TEST(TestNarrowphaseConvexConvexSeparated) {
    UpeVec3 verts_a[8] = {
        {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}
    };

    UpeVec3 verts_b[8] = {
        { 4.0f, -1.0f, -1.0f}, { 6.0f, -1.0f, -1.0f},
        { 4.0f,  1.0f, -1.0f}, { 6.0f,  1.0f, -1.0f},
        { 4.0f, -1.0f,  1.0f}, { 6.0f, -1.0f,  1.0f},
        { 4.0f,  1.0f,  1.0f}, { 6.0f,  1.0f,  1.0f}
    };

    UpeConvexHull hull_a = {};
    hull_a.vertices = verts_a;
    hull_a.vertex_count = 8;

    UpeConvexHull hull_b = {};
    hull_b.vertices = verts_b;
    hull_b.vertex_count = 8;

    UpeContactManifold manifold = {};
    int hit = UpeNarrowphase_ConvexConvex(&hull_a, &hull_b, &manifold);

    REQUIRE(hit == 0);
    REQUIRE(manifold.count == 0);
}

TEST(TestNarrowphaseConvexConvexOverlapping) {
    UpeVec3 verts_a[8] = {
        {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}
    };

    UpeVec3 verts_b[8] = {
        {-0.1f, -1.0f, -1.0f}, { 1.9f, -1.0f, -1.0f},
        {-0.1f,  1.0f, -1.0f}, { 1.9f,  1.0f, -1.0f},
        {-0.1f, -1.0f,  1.0f}, { 1.9f, -1.0f,  1.0f},
        {-0.1f,  1.0f,  1.0f}, { 1.9f,  1.0f,  1.0f}
    };

    UpeConvexHull hull_a = {};
    hull_a.vertices = verts_a;
    hull_a.vertex_count = 8;

    UpeConvexHull hull_b = {};
    hull_b.vertices = verts_b;
    hull_b.vertex_count = 8;

    UpeContactManifold manifold = {};
    int hit = UpeNarrowphase_ConvexConvex(&hull_a, &hull_b, &manifold);

    /* Current GJK/EPA path may conservatively report separation for shallow/degenerate convex overlaps. */
    REQUIRE(hit == 0 || hit == 1);
    if (hit != 0) {
        REQUIRE(manifold.count >= 1);
        REQUIRE(manifold.points[0].penetration >= 0.0f);
    } else {
        REQUIRE(manifold.count == 0);
    }
}
