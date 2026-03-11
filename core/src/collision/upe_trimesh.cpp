#include "upe/upe_trimesh.h"
#include <float.h>

static int UpeRayTriangle(UpeVec3 origin, UpeVec3 dir, UpeVec3 v0, UpeVec3 v1, UpeVec3 v2, float *t, float *u, float *v) {
    UpeVec3 e1 = UpeVec3_Sub(v1, v0);
    UpeVec3 e2 = UpeVec3_Sub(v2, v0);
    UpeVec3 h = UpeVec3_Cross(dir, e2);
    float a = UpeVec3_Dot(e1, h);
    if (a > -1.0e-6f && a < 1.0e-6f) {
        return 0;
    }
    float f = 1.0f / a;
    UpeVec3 s = UpeVec3_Sub(origin, v0);
    *u = f * UpeVec3_Dot(s, h);
    if (*u < 0.0f || *u > 1.0f) {
        return 0;
    }
    UpeVec3 q = UpeVec3_Cross(s, e1);
    *v = f * UpeVec3_Dot(dir, q);
    if (*v < 0.0f || *u + *v > 1.0f) {
        return 0;
    }
    *t = f * UpeVec3_Dot(e2, q);
    return *t > 1.0e-6f;
}

void UpeTriangleMesh_Init(UpeTriangleMesh *mesh) {
    if (!mesh) {
        return;
    }
    mesh->vertices = nullptr;
    mesh->indices = nullptr;
    mesh->vertex_count = 0;
    mesh->index_count = 0;
}

void UpeTriangleMesh_Destroy(UpeTriangleMesh *mesh) {
    if (!mesh) {
        return;
    }
    delete[] mesh->vertices;
    delete[] mesh->indices;
    mesh->vertices = nullptr;
    mesh->indices = nullptr;
    mesh->vertex_count = 0;
    mesh->index_count = 0;
}

int UpeTriangleMesh_Raycast(const UpeTriangleMesh *mesh, UpeVec3 origin, UpeVec3 dir,
                            float max_t, UpeRaycastHit *hit) {
    if (!mesh || !hit || mesh->index_count == 0) {
        return 0;
    }

    int found = 0;
    float closest_t = max_t;
    uint32_t closest_tri = 0;

    uint32_t tri_count = mesh->index_count / 3;
    for (uint32_t i = 0; i < tri_count; ++i) {
        uint32_t i0 = mesh->indices[i * 3 + 0];
        uint32_t i1 = mesh->indices[i * 3 + 1];
        uint32_t i2 = mesh->indices[i * 3 + 2];
        UpeVec3 v0 = mesh->vertices[i0];
        UpeVec3 v1 = mesh->vertices[i1];
        UpeVec3 v2 = mesh->vertices[i2];

        float t = 0.0f;
        float u = 0.0f;
        float v = 0.0f;
        if (UpeRayTriangle(origin, dir, v0, v1, v2, &t, &u, &v)) {
            if (t < closest_t) {
                closest_t = t;
                closest_tri = i;
                found = 1;
            }
        }
    }

    if (found) {
        uint32_t i0 = mesh->indices[closest_tri * 3 + 0];
        uint32_t i1 = mesh->indices[closest_tri * 3 + 1];
        uint32_t i2 = mesh->indices[closest_tri * 3 + 2];
        UpeVec3 v0 = mesh->vertices[i0];
        UpeVec3 v1 = mesh->vertices[i1];
        UpeVec3 v2 = mesh->vertices[i2];

        UpeVec3 e1 = UpeVec3_Sub(v1, v0);
        UpeVec3 e2 = UpeVec3_Sub(v2, v0);
        UpeVec3 normal = UpeVec3_Normalize(UpeVec3_Cross(e1, e2));

        hit->t = closest_t;
        hit->position = UpeVec3_Add(origin, UpeVec3_Scale(dir, closest_t));
        hit->normal = normal;
        hit->triangle_index = closest_tri;
    }

    return found;
}
