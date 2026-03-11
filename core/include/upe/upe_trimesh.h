#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeTriangleMesh {
    UpeVec3 *vertices;
    uint32_t *indices;
    uint32_t vertex_count;
    uint32_t index_count;
} UpeTriangleMesh;

typedef struct UpeRaycastHit {
    float t;
    UpeVec3 position;
    UpeVec3 normal;
    uint32_t triangle_index;
} UpeRaycastHit;

void UpeTriangleMesh_Init(UpeTriangleMesh *mesh);
void UpeTriangleMesh_Destroy(UpeTriangleMesh *mesh);

int UpeTriangleMesh_Raycast(const UpeTriangleMesh *mesh, UpeVec3 origin, UpeVec3 dir,
                            float max_t, UpeRaycastHit *hit);

#ifdef __cplusplus
}
#endif
