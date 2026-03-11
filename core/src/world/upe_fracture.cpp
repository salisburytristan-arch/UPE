#include "upe/upe_fracture.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static uint32_t fr_rand_next(uint32_t *state) {
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

static float fr_rand01(uint32_t *state) {
    return (float)(fr_rand_next(state) & 0x00FFFFFFu) / 16777215.0f;
}

static UpeVec3 fr_make_vec3(float x, float y, float z) {
    UpeVec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

void UpeFracture_Init(UpeFractureSystem *sys, uint32_t max_fragments) {
    if (!sys) {
        return;
    }

    memset(sys, 0, sizeof(*sys));

    sys->fragments = (UpeFractureFragment *)calloc(max_fragments, sizeof(UpeFractureFragment));
    sys->fragment_capacity = max_fragments;

    sys->fracture_queue = (uint32_t *)calloc(max_fragments, sizeof(uint32_t));
    sys->queue_capacity = max_fragments;
}

void UpeFracture_Destroy(UpeFractureSystem *sys) {
    if (!sys) {
        return;
    }

    if (sys->fragments) {
        for (uint32_t i = 0; i < sys->fragment_count; ++i) {
            free(sys->fragments[i].vertices);
            free(sys->fragments[i].indices);
        }
    }

    free(sys->fragments);
    free(sys->fracture_queue);
    memset(sys, 0, sizeof(*sys));
}

void UpeFracture_GenerateVoronoi(UpeFracturePattern *pattern,
                                 UpeVec3 bounds_min, UpeVec3 bounds_max,
                                 uint32_t num_cells, uint32_t seed) {
    if (!pattern) {
        return;
    }

    if (num_cells == 0) {
        num_cells = 1;
    }
    if (num_cells > UPE_FRACTURE_MAX_VORONOI_SITES) {
        num_cells = UPE_FRACTURE_MAX_VORONOI_SITES;
    }

    free(pattern->voronoi_sites);
    pattern->voronoi_sites = (UpeVec3 *)malloc(num_cells * sizeof(UpeVec3));
    if (!pattern->voronoi_sites) {
        pattern->site_count = 0;
        pattern->num_fragments = 0;
        return;
    }

    pattern->mode = UPE_FRACTURE_VORONOI;
    pattern->site_count = num_cells;
    pattern->num_fragments = num_cells;
    pattern->random_seed = seed;

    uint32_t rng = (seed == 0) ? 1u : seed;
    for (uint32_t i = 0; i < num_cells; ++i) {
        float rx = fr_rand01(&rng);
        float ry = fr_rand01(&rng);
        float rz = fr_rand01(&rng);

        pattern->voronoi_sites[i].x = bounds_min.x + (bounds_max.x - bounds_min.x) * rx;
        pattern->voronoi_sites[i].y = bounds_min.y + (bounds_max.y - bounds_min.y) * ry;
        pattern->voronoi_sites[i].z = bounds_min.z + (bounds_max.z - bounds_min.z) * rz;
    }
}

void UpeFracture_GenerateRadial(UpeFracturePattern *pattern,
                                UpeVec3 impact_point, UpeVec3 impact_dir,
                                uint32_t num_rays) {
    if (!pattern) {
        return;
    }

    if (num_rays == 0) {
        num_rays = 2;
    }
    if (num_rays > UPE_FRACTURE_MAX_VORONOI_SITES) {
        num_rays = UPE_FRACTURE_MAX_VORONOI_SITES;
    }

    free(pattern->voronoi_sites);
    pattern->voronoi_sites = (UpeVec3 *)malloc(num_rays * sizeof(UpeVec3));
    if (!pattern->voronoi_sites) {
        pattern->site_count = 0;
        pattern->num_fragments = 0;
        return;
    }

    pattern->mode = UPE_FRACTURE_RADIAL;
    pattern->site_count = num_rays;
    pattern->num_fragments = num_rays;
    pattern->impact_point = impact_point;
    pattern->impact_direction = impact_dir;

    const float two_pi = 6.28318530718f;
    for (uint32_t i = 0; i < num_rays; ++i) {
        float t = (float)i / (float)num_rays;
        float angle = t * two_pi;
        float radius = 0.25f + 0.75f * t;

        pattern->voronoi_sites[i].x = impact_point.x + cosf(angle) * radius;
        pattern->voronoi_sites[i].y = impact_point.y + impact_dir.y * 0.2f * radius;
        pattern->voronoi_sites[i].z = impact_point.z + sinf(angle) * radius;
    }
}

void UpeFracture_Execute(UpeFractureSystem *sys,
                         uint32_t body_id,
                         const UpeFracturePattern *pattern,
                         UpeFractureFragment *out_fragments,
                         uint32_t *out_count) {
    if (!sys || !pattern || !out_fragments || !out_count) {
        return;
    }

    uint32_t requested = pattern->site_count;
    if (requested == 0) {
        requested = pattern->num_fragments;
    }
    if (requested == 0) {
        requested = 1;
    }
    if (requested > UPE_FRACTURE_MAX_FRAGMENTS) {
        requested = UPE_FRACTURE_MAX_FRAGMENTS;
    }

    *out_count = requested;

    float base_mass = 1.0f;
    float mass_per = base_mass / (float)requested;

    for (uint32_t i = 0; i < requested; ++i) {
        UpeFractureFragment *f = &out_fragments[i];
        memset(f, 0, sizeof(*f));

        f->body_id = body_id;
        f->centroid = (pattern->voronoi_sites && i < pattern->site_count)
                          ? pattern->voronoi_sites[i]
                          : pattern->impact_point;
        f->volume = 1.0f / (float)requested;
        f->mass = mass_per;

        f->vertex_count = 4;
        f->vertices = (UpeVec3 *)malloc(f->vertex_count * sizeof(UpeVec3));
        f->index_count = 12;
        f->indices = (uint32_t *)malloc(f->index_count * sizeof(uint32_t));

        if (f->vertices) {
            float s = 0.05f + 0.01f * (float)(i % 5);
            f->vertices[0] = fr_make_vec3(f->centroid.x - s, f->centroid.y, f->centroid.z - s);
            f->vertices[1] = fr_make_vec3(f->centroid.x + s, f->centroid.y, f->centroid.z - s);
            f->vertices[2] = fr_make_vec3(f->centroid.x, f->centroid.y + s, f->centroid.z + s);
            f->vertices[3] = fr_make_vec3(f->centroid.x, f->centroid.y - s, f->centroid.z + s);
        }

        if (f->indices) {
            uint32_t tri[12] = {0, 1, 2, 0, 3, 1, 1, 3, 2, 0, 2, 3};
            memcpy(f->indices, tri, sizeof(tri));
        }

        f->is_debris = (f->mass < 0.08f) ? 1 : 0;

        if (sys->fragment_count < sys->fragment_capacity) {
            UpeFractureFragment *dst = &sys->fragments[sys->fragment_count++];
            *dst = *f;
            f->vertices = NULL;
            f->indices = NULL;
        }
    }
}

void UpeDamage_ApplyImpact(UpeDamageModel *damage, float impact_force, float threshold) {
    if (!damage) {
        return;
    }

    if (threshold <= 0.0f) {
        threshold = 1.0f;
    }

    float normalized = impact_force / threshold;
    if (normalized <= 1.0f) {
        damage->accumulated_damage -= damage->healing_rate * 0.1f;
    } else {
        float excess = normalized - 1.0f;
        float brittle_boost = 0.5f + damage->brittleness;
        damage->accumulated_damage += excess * damage->damage_rate * brittle_boost;
    }

    if (damage->accumulated_damage < 0.0f) {
        damage->accumulated_damage = 0.0f;
    }
    if (damage->accumulated_damage > 1.0f) {
        damage->accumulated_damage = 1.0f;
    }
}

int UpeDamage_ShouldFracture(const UpeDamageModel *damage) {
    if (!damage) {
        return 0;
    }

    if (damage->accumulated_damage >= 1.0f) {
        return 1;
    }

    if (damage->accumulated_damage > 0.85f && damage->brittleness > 0.8f) {
        return 1;
    }

    return 0;
}

float UpeFracture_ComputeStress(UpeVec3 force, float area, UpeVec3 normal) {
    if (area <= 1e-6f) {
        return 0.0f;
    }

    float n_len = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (n_len <= 1e-6f) {
        return 0.0f;
    }

    normal.x /= n_len;
    normal.y /= n_len;
    normal.z /= n_len;

    float normal_force = force.x * normal.x + force.y * normal.y + force.z * normal.z;
    if (normal_force < 0.0f) {
        normal_force = -normal_force;
    }

    return normal_force / area;
}
