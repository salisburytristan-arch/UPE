#include "upe/upe_destruction.h"
#include "upe/upe_fracture.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    uint32_t body_id;
    UpeDestructionMaterialDesc material;
    float accumulated_damage;
    int destroyed;
    uint8_t _padding[3];

    UpeDamageModel damage_model;
    UpeVec3 last_impact_point;
    UpeVec3 last_impact_normal;
    float last_impact_force;
} UpeDestructibleObject;

typedef struct {
    uint32_t body_id;
    UpeVec3 fracture_point;
    UpeVec3 impact_normal;
    float impact_force;
    uint8_t processed;
} UpePendingFracture;

struct UpeDestructionTracker {
    UpeDestructibleObject *objects;
    uint32_t object_count;
    uint32_t object_capacity;

    UpePendingFracture *pending_fractures;
    uint32_t pending_count;
    uint32_t pending_capacity;

    UpeFractureSystem fracture_system;

    UpeDestructionCallback destruction_callback;
    void *callback_user_data;

    float debris_lifetime;
    uint32_t max_debris;
};

static const UpeDestructionMaterialDesc g_material_defaults[] = {
    {UPE_MATERIAL_CONCRETE, 50.0f, 2400.0f, 0.8f, 1, 1},
    {UPE_MATERIAL_WOOD, 30.0f, 600.0f, 0.5f, 1, 0},
    {UPE_MATERIAL_GLASS, 40.0f, 2500.0f, 1.0f, 1, 1},
    {UPE_MATERIAL_METAL, 100.0f, 7850.0f, 0.1f, 0, 1},
    {UPE_MATERIAL_CERAMIC, 45.0f, 2300.0f, 0.9f, 1, 1},
};

static const UpeDestructionMaterialDesc *get_material_desc_or_default(const UpeDestructionMaterialDesc *material) {
    if (material) {
        return material;
    }
    return &g_material_defaults[0];
}

static UpeDestructibleObject *find_object(UpeDestructionTracker *tracker, uint32_t body_id) {
    if (!tracker) {
        return NULL;
    }

    for (uint32_t i = 0; i < tracker->object_count; ++i) {
        if (tracker->objects[i].body_id == body_id) {
            return &tracker->objects[i];
        }
    }
    return NULL;
}

static const UpeDestructibleObject *find_object_const(const UpeDestructionTracker *tracker, uint32_t body_id) {
    if (!tracker) {
        return NULL;
    }

    for (uint32_t i = 0; i < tracker->object_count; ++i) {
        if (tracker->objects[i].body_id == body_id) {
            return &tracker->objects[i];
        }
    }
    return NULL;
}

static void queue_fracture(UpeDestructionTracker *tracker,
                           uint32_t body_id,
                           UpeVec3 point,
                           UpeVec3 normal,
                           float force) {
    if (!tracker || tracker->pending_count >= tracker->pending_capacity) {
        return;
    }

    UpePendingFracture *evt = &tracker->pending_fractures[tracker->pending_count++];
    evt->body_id = body_id;
    evt->fracture_point = point;
    evt->impact_normal = normal;
    evt->impact_force = force;
    evt->processed = 0;
}

static uint32_t estimate_fragment_count(const UpeDestructibleObject *obj) {
    if (!obj) {
        return 4;
    }

    float brittle = obj->material.brittleness;
    if (brittle < 0.0f) brittle = 0.0f;
    if (brittle > 1.0f) brittle = 1.0f;

    float density_factor = obj->material.density / 2500.0f;
    if (density_factor < 0.3f) density_factor = 0.3f;
    if (density_factor > 2.0f) density_factor = 2.0f;

    uint32_t fragments = (uint32_t)(6.0f + 12.0f * brittle + 3.0f * density_factor);
    if (fragments < 2) fragments = 2;
    if (fragments > UPE_FRACTURE_MAX_VORONOI_SITES) fragments = UPE_FRACTURE_MAX_VORONOI_SITES;
    return fragments;
}

UpeDestructionTracker *UpeDestructionTracker_Create(uint32_t max_objects) {
    if (max_objects == 0) {
        max_objects = 1;
    }

    UpeDestructionTracker *tracker = (UpeDestructionTracker *)malloc(sizeof(UpeDestructionTracker));
    if (!tracker) {
        return NULL;
    }

    memset(tracker, 0, sizeof(*tracker));

    tracker->objects = (UpeDestructibleObject *)calloc(max_objects, sizeof(UpeDestructibleObject));
    tracker->pending_fractures = (UpePendingFracture *)calloc(max_objects, sizeof(UpePendingFracture));

    if (!tracker->objects || !tracker->pending_fractures) {
        free(tracker->objects);
        free(tracker->pending_fractures);
        free(tracker);
        return NULL;
    }

    tracker->object_count = 0;
    tracker->object_capacity = max_objects;
    tracker->pending_count = 0;
    tracker->pending_capacity = max_objects;

    tracker->debris_lifetime = 5.0f;
    tracker->max_debris = 1000;

    UpeFracture_Init(&tracker->fracture_system, max_objects * 16);

    return tracker;
}

void UpeDestructionTracker_Destroy(UpeDestructionTracker *tracker) {
    if (!tracker) {
        return;
    }

    UpeFracture_Destroy(&tracker->fracture_system);

    free(tracker->objects);
    free(tracker->pending_fractures);
    free(tracker);
}

void UpeDestructionTracker_RegisterBody(UpeDestructionTracker *tracker,
                                        uint32_t body_id,
                                        const UpeDestructionMaterialDesc *material) {
    if (!tracker || tracker->object_count >= tracker->object_capacity) {
        return;
    }

    UpeDestructibleObject *existing = find_object(tracker, body_id);
    if (existing) {
        const UpeDestructionMaterialDesc *desc_existing = get_material_desc_or_default(material);
        existing->material = *desc_existing;
        return;
    }

    const UpeDestructionMaterialDesc *desc = get_material_desc_or_default(material);

    UpeDestructibleObject *obj = &tracker->objects[tracker->object_count++];
    memset(obj, 0, sizeof(*obj));

    obj->body_id = body_id;
    obj->material = *desc;
    obj->accumulated_damage = 0.0f;
    obj->destroyed = 0;

    obj->damage_model.max_stress = desc->fracture_threshold;
    obj->damage_model.fracture_toughness = desc->fracture_threshold * 0.8f;
    obj->damage_model.accumulated_damage = 0.0f;
    obj->damage_model.damage_rate = 0.2f;
    obj->damage_model.healing_rate = 0.01f;
    obj->damage_model.brittleness = desc->brittleness;
    obj->damage_model.plastic_strain = 0.02f + (1.0f - desc->brittleness) * 0.08f;
}

void UpeDestructionTracker_ReportImpact(UpeDestructionTracker *tracker,
                                        uint32_t body_id,
                                        UpeVec3 impact_point,
                                        UpeVec3 impact_normal,
                                        float impact_force) {
    if (!tracker || impact_force <= 0.0f) {
        return;
    }

    UpeDestructibleObject *obj = find_object(tracker, body_id);
    if (!obj || obj->destroyed) {
        return;
    }

    obj->last_impact_point = impact_point;
    obj->last_impact_normal = impact_normal;
    obj->last_impact_force = impact_force;

    float pseudo_area = 0.03f + (obj->material.density / 10000.0f);
    UpeVec3 impact_force_vec;
    impact_force_vec.x = impact_normal.x * impact_force;
    impact_force_vec.y = impact_normal.y * impact_force;
    impact_force_vec.z = impact_normal.z * impact_force;
    float stress = UpeFracture_ComputeStress(
        impact_force_vec,
        pseudo_area,
        impact_normal);

    UpeDamage_ApplyImpact(&obj->damage_model, stress, obj->material.fracture_threshold);

    float damage_factor = impact_force / fmaxf(0.001f, obj->material.fracture_threshold);
    obj->accumulated_damage += damage_factor * (0.2f + 0.8f * obj->material.brittleness);

    if (obj->accumulated_damage > 1.0f) {
        obj->accumulated_damage = 1.0f;
    }

    if (UpeDamage_ShouldFracture(&obj->damage_model) || obj->accumulated_damage >= 1.0f) {
        obj->destroyed = 1;
        queue_fracture(tracker, body_id, impact_point, impact_normal, impact_force);

        if (tracker->destruction_callback) {
            tracker->destruction_callback(body_id, impact_point, tracker->callback_user_data);
        }
    }
}

void UpeDestructionTracker_ProcessFrame(UpeDestructionTracker *tracker,
                                        UpeDestructionFrameResult *out_result) {
    if (!tracker || !out_result) {
        return;
    }

    memset(out_result, 0, sizeof(UpeDestructionFrameResult));

    for (uint32_t i = 0; i < tracker->object_count; ++i) {
        UpeDestructibleObject *obj = &tracker->objects[i];

        if (obj->destroyed) {
            out_result->destroyed_object_count++;
        }

        out_result->total_stress_energy += obj->accumulated_damage * obj->material.fracture_threshold;
    }

    uint32_t generated_debris = 0;

    for (uint32_t i = 0; i < tracker->pending_count; ++i) {
        UpePendingFracture *evt = &tracker->pending_fractures[i];
        if (evt->processed) {
            continue;
        }

        UpeDestructibleObject *obj = find_object(tracker, evt->body_id);
        if (!obj) {
            evt->processed = 1;
            continue;
        }

        UpeFracturePattern pattern;
        memset(&pattern, 0, sizeof(pattern));
        pattern.impact_point = evt->fracture_point;
        pattern.impact_direction = evt->impact_normal;
        pattern.impact_force = evt->impact_force;

        uint32_t fragment_target = estimate_fragment_count(obj);

        if (obj->material.brittleness > 0.75f) {
            UpeFracture_GenerateRadial(&pattern, evt->fracture_point, evt->impact_normal, fragment_target);
        } else {
            UpeVec3 bmin = {evt->fracture_point.x - 0.5f, evt->fracture_point.y - 0.5f, evt->fracture_point.z - 0.5f};
            UpeVec3 bmax = {evt->fracture_point.x + 0.5f, evt->fracture_point.y + 0.5f, evt->fracture_point.z + 0.5f};
            UpeFracture_GenerateVoronoi(&pattern, bmin, bmax, fragment_target, evt->body_id * 2654435761u);
        }

        UpeFractureFragment stack_fragments[UPE_FRACTURE_MAX_VORONOI_SITES];
        uint32_t out_count = 0;
        UpeFracture_Execute(&tracker->fracture_system, evt->body_id, &pattern, stack_fragments, &out_count);

        if (obj->material.create_debris) {
            generated_debris += out_count;
        }

        for (uint32_t f = 0; f < out_count; ++f) {
            free(stack_fragments[f].vertices);
            free(stack_fragments[f].indices);
            stack_fragments[f].vertices = NULL;
            stack_fragments[f].indices = NULL;
        }

        free(pattern.voronoi_sites);
        pattern.voronoi_sites = NULL;

        evt->processed = 1;
    }

    if (generated_debris > tracker->max_debris) {
        generated_debris = tracker->max_debris;
    }
    out_result->generated_debris_count = generated_debris;

    tracker->pending_count = 0;
}

int UpeDestructionTracker_IsDestroyed(UpeDestructionTracker *tracker, uint32_t body_id) {
    UpeDestructibleObject *obj = find_object(tracker, body_id);
    return obj ? obj->destroyed : 0;
}

float UpeDestructionTracker_GetDamage(UpeDestructionTracker *tracker, uint32_t body_id) {
    UpeDestructibleObject *obj = find_object(tracker, body_id);
    return obj ? obj->accumulated_damage : 0.0f;
}

void UpeDestructionTracker_SetDestructionCallback(UpeDestructionTracker *tracker,
                                                  UpeDestructionCallback callback,
                                                  void *user_data) {
    if (!tracker) {
        return;
    }
    tracker->destruction_callback = callback;
    tracker->callback_user_data = user_data;
}

void UpeDestructionTracker_PropagateFracture(UpeDestructionTracker *tracker,
                                             uint32_t initiating_body,
                                             float propagation_radius) {
    if (!tracker) {
        return;
    }

    const UpeDestructibleObject *initiating = find_object_const(tracker, initiating_body);
    if (!initiating || !initiating->destroyed) {
        return;
    }

    if (propagation_radius < 0.1f) {
        propagation_radius = 0.1f;
    }

    for (uint32_t i = 0; i < tracker->object_count; ++i) {
        UpeDestructibleObject *obj = &tracker->objects[i];
        if (obj->body_id == initiating_body || obj->destroyed) {
            continue;
        }

        float attenuation = 1.0f / (1.0f + propagation_radius);
        float propagated_damage = initiating->accumulated_damage * 0.6f * attenuation;

        if (!obj->material.propagate_fracture) {
            propagated_damage *= 0.25f;
        }

        obj->accumulated_damage += propagated_damage;
        if (obj->accumulated_damage > 1.0f) {
            obj->accumulated_damage = 1.0f;
        }

        if (obj->accumulated_damage >= 1.0f) {
            obj->destroyed = 1;
            queue_fracture(tracker,
                           obj->body_id,
                           obj->last_impact_point,
                           obj->last_impact_normal,
                           fmaxf(0.1f, obj->last_impact_force));

            if (tracker->destruction_callback) {
                tracker->destruction_callback(obj->body_id, obj->last_impact_point, tracker->callback_user_data);
            }
        }
    }
}

float UpeDestruction_SpallingDepth(float impact_pressure_Pa, float material_tensile_Pa) {
    if (impact_pressure_Pa <= 0.0f || material_tensile_Pa <= 0.0f) return 0.0f;
    /* Approximate spall depth scales with pressure ratio */
    float ratio = impact_pressure_Pa / material_tensile_Pa;
    if (ratio < 1.0f) return 0.0f;
    return 0.001f * sqrtf(ratio);  /* meters */
}

float UpeDestruction_CrumpleForce(float axial_load_N, float yield_stress_Pa,
                                   float tube_thickness_m, float tube_radius_m) {
    if (yield_stress_Pa <= 0.0f || tube_thickness_m <= 0.0f || tube_radius_m <= 0.0f) return 0.0f;
    /* Alexander-style plastic collapse force estimate */
    float F_plastic = 2.0f * 3.14159265f * yield_stress_Pa * tube_thickness_m * tube_radius_m;
    return (axial_load_N < F_plastic) ? axial_load_N : F_plastic;
}

float UpeDestruction_CraterRadius(float projectile_mass, float impact_velocity,
                                   float target_density) {
    if (projectile_mass <= 0.0f || impact_velocity <= 0.0f || target_density <= 0.0f) return 0.0f;
    /* Energy scaling: R ~ (E / rho)^1/5 */
    float E = 0.5f * projectile_mass * impact_velocity * impact_velocity;
    return powf(E / target_density, 0.2f);
}
