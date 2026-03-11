#include "upe/upe_shallow_water.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static inline uint32_t sw_idx(uint32_t x, uint32_t z, uint32_t width) {
    return z * width + x;
}

void UpeShallowWater_Init(UpeShallowWaterSystem *water, uint32_t grid_width, uint32_t grid_depth, float cell_size) {
    if (!water || grid_width == 0 || grid_depth == 0) {
        return;
    }

    uint32_t size = grid_width * grid_depth;

    memset(water, 0, sizeof(*water));

    water->grid.width = grid_width;
    water->grid.depth = grid_depth;
    water->grid.cell_size = cell_size;

    water->grid.height = (float *)calloc(size, sizeof(float));
    water->grid.velocity_x = (float *)calloc(size, sizeof(float));
    water->grid.velocity_z = (float *)calloc(size, sizeof(float));
    water->grid.terrain = (float *)calloc(size, sizeof(float));
    water->grid.boundary_type = (uint8_t *)calloc(size, sizeof(uint8_t));

    water->flux_x = (float *)calloc(size, sizeof(float));
    water->flux_z = (float *)calloc(size, sizeof(float));
    water->height_new = (float *)calloc(size, sizeof(float));
    water->vel_x_new = (float *)calloc(size, sizeof(float));
    water->vel_z_new = (float *)calloc(size, sizeof(float));

    water->gravity = 9.81f;
    water->manning_coefficient = 0.03f;
    water->min_height = 0.001f;
}

void UpeShallowWater_Destroy(UpeShallowWaterSystem *water) {
    if (!water) {
        return;
    }

    free(water->grid.height);
    free(water->grid.velocity_x);
    free(water->grid.velocity_z);
    free(water->grid.terrain);
    free(water->grid.boundary_type);

    free(water->flux_x);
    free(water->flux_z);
    free(water->height_new);
    free(water->vel_x_new);
    free(water->vel_z_new);

    memset(water, 0, sizeof(*water));
}

void UpeShallowWater_Step(UpeShallowWaterSystem *water, float dt) {
    if (!water || !water->grid.height || dt <= 0.0f) {
        return;
    }

    const uint32_t w = water->grid.width;
    const uint32_t d = water->grid.depth;
    const float dx = water->grid.cell_size;
    const float g = water->gravity;

    for (uint32_t z = 1; z + 1 < d; ++z) {
        for (uint32_t x = 1; x + 1 < w; ++x) {
            uint32_t c = sw_idx(x, z, w);
            uint32_t l = sw_idx(x - 1, z, w);
            uint32_t r = sw_idx(x + 1, z, w);
            uint32_t u = sw_idx(x, z - 1, w);
            uint32_t dn = sw_idx(x, z + 1, w);

            float h = water->grid.height[c];
            if (h < water->min_height) {
                water->height_new[c] = 0.0f;
                water->vel_x_new[c] = 0.0f;
                water->vel_z_new[c] = 0.0f;
                continue;
            }

            float dhdx = (water->grid.height[r] - water->grid.height[l]) / (2.0f * dx);
            float dhdz = (water->grid.height[dn] - water->grid.height[u]) / (2.0f * dx);

            float vx = water->grid.velocity_x[c] - g * dt * dhdx;
            float vz = water->grid.velocity_z[c] - g * dt * dhdz;

            float speed = sqrtf(vx * vx + vz * vz);
            float friction = 1.0f;
            if (h > water->min_height) {
                float n = water->manning_coefficient;
                float denom = powf(h, 4.0f / 3.0f);
                if (denom > 1e-6f) {
                    friction = 1.0f - dt * g * n * n * speed / denom;
                    if (friction < 0.0f) friction = 0.0f;
                }
            }

            vx *= friction;
            vz *= friction;

            float div = ((water->grid.velocity_x[r] - water->grid.velocity_x[l]) +
                         (water->grid.velocity_z[dn] - water->grid.velocity_z[u])) / (2.0f * dx);

            float h_new = h - dt * h * div;
            if (h_new < water->min_height) h_new = 0.0f;

            water->height_new[c] = h_new;
            water->vel_x_new[c] = vx;
            water->vel_z_new[c] = vz;
        }
    }

    for (uint32_t x = 0; x < w; ++x) {
        uint32_t t = sw_idx(x, 0, w);
        uint32_t b = sw_idx(x, d - 1, w);
        water->height_new[t] = water->grid.height[t];
        water->height_new[b] = water->grid.height[b];
        water->vel_x_new[t] = 0.0f;
        water->vel_z_new[t] = 0.0f;
        water->vel_x_new[b] = 0.0f;
        water->vel_z_new[b] = 0.0f;
    }
    for (uint32_t z = 0; z < d; ++z) {
        uint32_t l = sw_idx(0, z, w);
        uint32_t r = sw_idx(w - 1, z, w);
        water->height_new[l] = water->grid.height[l];
        water->height_new[r] = water->grid.height[r];
        water->vel_x_new[l] = 0.0f;
        water->vel_z_new[l] = 0.0f;
        water->vel_x_new[r] = 0.0f;
        water->vel_z_new[r] = 0.0f;
    }

    uint32_t size = w * d;
    memcpy(water->grid.height, water->height_new, size * sizeof(float));
    memcpy(water->grid.velocity_x, water->vel_x_new, size * sizeof(float));
    memcpy(water->grid.velocity_z, water->vel_z_new, size * sizeof(float));

    for (uint32_t i = 0; i < size; ++i) {
        if (water->grid.height[i] < 0.0f) {
            water->grid.height[i] = 0.0f;
        }
    }
}

void UpeShallowWater_SetTerrain(UpeShallowWaterSystem *water, uint32_t x, uint32_t z, float elevation) {
    if (!water || !water->grid.terrain || x >= water->grid.width || z >= water->grid.depth) {
        return;
    }
    water->grid.terrain[sw_idx(x, z, water->grid.width)] = elevation;
}

void UpeShallowWater_AddWater(UpeShallowWaterSystem *water, uint32_t x, uint32_t z, float amount) {
    if (!water || !water->grid.height || x >= water->grid.width || z >= water->grid.depth) {
        return;
    }
    uint32_t i = sw_idx(x, z, water->grid.width);
    water->grid.height[i] += amount;
    if (water->grid.height[i] < 0.0f) {
        water->grid.height[i] = 0.0f;
    }
}

float UpeShallowWater_GetHeight(const UpeShallowWaterSystem *water, uint32_t x, uint32_t z) {
    if (!water || !water->grid.height || x >= water->grid.width || z >= water->grid.depth) {
        return 0.0f;
    }
    return water->grid.height[sw_idx(x, z, water->grid.width)];
}

UpeVec2 UpeShallowWater_GetVelocity(const UpeShallowWaterSystem *water, uint32_t x, uint32_t z) {
    UpeVec2 v = {0.0f, 0.0f};
    if (!water || !water->grid.velocity_x || x >= water->grid.width || z >= water->grid.depth) {
        return v;
    }
    uint32_t i = sw_idx(x, z, water->grid.width);
    v.x = water->grid.velocity_x[i];
    v.y = water->grid.velocity_z[i];
    return v;
}

void UpeShallowWater_CreateDamBreak(UpeShallowWaterSystem *water, uint32_t dam_x) {
    if (!water || !water->grid.height || dam_x >= water->grid.width) {
        return;
    }
    for (uint32_t z = 0; z < water->grid.depth; ++z) {
        for (uint32_t x = 0; x < water->grid.width; ++x) {
            uint32_t i = sw_idx(x, z, water->grid.width);
            water->grid.height[i] = (x < dam_x) ? 2.0f : 0.2f;
            water->grid.velocity_x[i] = 0.0f;
            water->grid.velocity_z[i] = 0.0f;
        }
    }
}

void UpeShallowWater_CreateWave(UpeShallowWaterSystem *water, uint32_t source_x, uint32_t source_z, float amplitude) {
    if (!water || !water->grid.height || source_x >= water->grid.width || source_z >= water->grid.depth) {
        return;
    }

    const float radius = 8.0f;
    for (uint32_t z = 0; z < water->grid.depth; ++z) {
        for (uint32_t x = 0; x < water->grid.width; ++x) {
            float dx = (float)((int)x - (int)source_x);
            float dz = (float)((int)z - (int)source_z);
            float dist = sqrtf(dx * dx + dz * dz);
            if (dist <= radius) {
                float falloff = 1.0f - (dist / radius);
                water->grid.height[sw_idx(x, z, water->grid.width)] += amplitude * falloff;
            }
        }
    }
}
