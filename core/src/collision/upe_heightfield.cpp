#include "upe/upe_heightfield.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void UpeHeightfield_Init(UpeHeightfield *hf) {
    if (!hf) {
        return;
    }
    hf->heights = NULL;
    hf->rows = 0;
    hf->cols = 0;
    hf->cell_size = 1.0f;
}

void UpeHeightfield_Destroy(UpeHeightfield *hf) {
    if (!hf) {
        return;
    }
    if (hf->heights) {
        free(hf->heights);
    }
    hf->heights = NULL;
    hf->rows = 0;
    hf->cols = 0;
}

int UpeHeightfield_Allocate(UpeHeightfield *hf, uint32_t rows, uint32_t cols, float cell_size) {
    if (!hf || rows == 0 || cols == 0) {
        return 0;
    }
    
    /* Free existing data */
    if (hf->heights) {
        free(hf->heights);
    }
    
    /* Allocate new height data */
    hf->heights = (float *)malloc(rows * cols * sizeof(float));
    if (!hf->heights) {
        hf->rows = 0;
        hf->cols = 0;
        return 0;
    }
    
    hf->rows = rows;
    hf->cols = cols;
    hf->cell_size = cell_size > 0.0f ? cell_size : 1.0f;
    
    /* Zero initialize */
    memset(hf->heights, 0, rows * cols * sizeof(float));
    return 1;
}

float UpeHeightfield_SampleHeight(const UpeHeightfield *hf, float x, float z) {
    if (!hf || !hf->heights || hf->rows == 0 || hf->cols == 0) {
        return 0.0f;
    }

    float col_f = x / hf->cell_size;
    float row_f = z / hf->cell_size;
    int col = (int)floorf(col_f);
    int row = (int)floorf(row_f);

    if (col < 0 || row < 0 || col >= (int)hf->cols - 1 || row >= (int)hf->rows - 1) {
        return 0.0f;
    }

    float fx = col_f - (float)col;
    float fz = row_f - (float)row;

    float h00 = hf->heights[row * hf->cols + col];
    float h10 = hf->heights[row * hf->cols + (col + 1)];
    float h01 = hf->heights[(row + 1) * hf->cols + col];
    float h11 = hf->heights[(row + 1) * hf->cols + (col + 1)];

    float h0 = h00 + (h10 - h00) * fx;
    float h1 = h01 + (h11 - h01) * fx;
    return h0 + (h1 - h0) * fz;
}

UpeVec3 UpeHeightfield_GetNormal(const UpeHeightfield *hf, float x, float z) {
    UpeVec3 normal = {0.0f, 1.0f, 0.0f};
    
    if (!hf || !hf->heights || hf->rows == 0 || hf->cols == 0) {
        return normal;
    }
    
    /* Sample corners to estimate normal via finite differences */
    float eps = hf->cell_size * 0.5f;
    float h_center = UpeHeightfield_SampleHeight(hf, x, z);
    float h_x = UpeHeightfield_SampleHeight(hf, x + eps, z) - h_center;
    float h_z = UpeHeightfield_SampleHeight(hf, x, z + eps) - h_center;
    
    /* Cross product to get normal (right-hand rule for tangent space) */
    UpeVec3 tangent_x = {eps * 2.0f, h_x, 0.0f};
    UpeVec3 tangent_z = {0.0f, h_z, eps * 2.0f};
    normal = UpeVec3_Cross(tangent_z, tangent_x);
    
    float len = UpeVec3_Length(normal);
    if (len > 1e-6f) {
        normal = UpeVec3_Scale(normal, 1.0f / len);
    } else {
        normal = {0.0f, 1.0f, 0.0f};
    }
    
    return normal;
}

int UpeHeightfield_RaycastClosestPoint(const UpeHeightfield *hf, UpeVec3 ray_origin, UpeVec3 ray_direction,
                                       float max_distance, float *out_t, UpeVec3 *out_point, UpeVec3 *out_normal) {
    if (!hf || !hf->heights || hf->rows == 0 || hf->cols == 0) {
        return 0;
    }
    
    /* Simple raycast: step along ray and check height field */
    float t_step = hf->cell_size * 0.25f;
    float best_t = max_distance + 1.0f;
    UpeVec3 best_point = ray_origin;
    UpeVec3 best_normal = {0.0f, 1.0f, 0.0f};
    
    for (float t = 0.0f; t <= max_distance; t += t_step) {
        /* Current ray position */
        UpeVec3 pos = {
            ray_origin.x + ray_direction.x * t,
            ray_origin.y + ray_direction.y * t,
            ray_origin.z + ray_direction.z * t
        };
        
        /* Sample heightfield */
        float height = UpeHeightfield_SampleHeight(hf, pos.x, pos.z);
        
        /* Check if ray is below terrain */
        if (pos.y < height && t < best_t) {
            best_t = t;
            best_point = pos;
            best_point.y = height;
            best_normal = UpeHeightfield_GetNormal(hf, pos.x, pos.z);
        }
    }
    
    if (best_t <= max_distance) {
        *out_t = best_t;
        *out_point = best_point;
        *out_normal = best_normal;
        return 1;
    }
    
    return 0;
}
