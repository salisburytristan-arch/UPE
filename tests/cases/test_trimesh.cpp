#include "test_framework.h"
#include "upe/upe_trimesh.h"

TEST(TestTriangleMeshRaycast) {
    UpeTriangleMesh mesh;
    UpeTriangleMesh_Init(&mesh);

    UpeVec3 vertices[3] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}
    };
    uint32_t indices[3] = {0, 1, 2};

    mesh.vertices = new UpeVec3[3];
    mesh.indices = new uint32_t[3];
    for (int i = 0; i < 3; ++i) {
        mesh.vertices[i] = vertices[i];
        mesh.indices[i] = indices[i];
    }
    mesh.vertex_count = 3;
    mesh.index_count = 3;

    UpeRaycastHit hit;
    UpeVec3 origin = {0.25f, 1.0f, 0.25f};
    UpeVec3 dir = {0.0f, -1.0f, 0.0f};
    REQUIRE(UpeTriangleMesh_Raycast(&mesh, origin, dir, 10.0f, &hit));
    REQUIRE(hit.t > 0.0f && hit.t < 2.0f);

    UpeTriangleMesh_Destroy(&mesh);
}
