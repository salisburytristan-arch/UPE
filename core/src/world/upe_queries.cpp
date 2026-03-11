#include "upe/upe_queries.h"
#include "upe/upe_api.h"
#include "upe/upe_shape.h"
#include "upe/upe_body.h"
#include "upe/upe_bvh.h"
#include "upe/upe_obb.h"
#include "upe/upe_convex.h"
#include "upe/upe_ccd.h"
#include "upe/upe_trimesh.h"
#include "../world/upe_world.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>

UpeQueryFilter UpeQueryFilter_Default() {
    UpeQueryFilter filter;
    filter.layer_mask = 0xFFFFFFFF;
    filter.ignore_mask = 0;
    filter.include_triggers = 0;
    return filter;
}

int UpeCollisionFilter_ShouldCollide(uint32_t body1_layer, uint32_t body1_mask, 
                                    uint32_t body2_layer, uint32_t body2_mask) {
    // Check if body1's mask includes body2's layer
    if (!(body1_mask & (1U << body2_layer))) return 0;
    // Check if body2's mask includes body1's layer
    if (!(body2_mask & (1U << body1_layer))) return 0;
    return 1;
}

static const UpeBody *UpeWorld_FindBodyById(const UpeWorld *world, uint32_t body_id) {
    if (!world) {
        return nullptr;
    }

    for (uint32_t i = 0; i < world->body_capacity; ++i) {
        if (!world->body_alive || world->body_alive[i] == 0) {
            continue;
        }
        if (world->bodies[i].id == body_id) {
            return &world->bodies[i];
        }
    }
    return nullptr;
}

static int UpeWorld_QueryFilterAllowsShape(const UpeWorld *world,
                                           const UpeShape *shape,
                                           const UpeQueryFilter *filter) {
    if (!filter) {
        return 1;
    }
    if (!shape) {
        return 0;
    }

    uint32_t layer = shape->collision_layer;
    if (layer == 0u) {
        const UpeBody *body = UpeWorld_FindBodyById(world, shape->body_id);
        if (!body) {
            return 0;
        }
        layer = body->collision_layer;
    }

    if (layer >= 32u) {
        return 0;
    }
    return (filter->layer_mask & (1u << layer)) != 0u;
}

static int UpeWorld_ComputeShapeAabb(const UpeWorld *world, const UpeShape *shape, UpeAabb3 *out) {
    if (!world || !shape || !out) {
        return 0;
    }

    const UpeBody *body = UpeWorld_FindBodyById(world, shape->body_id);
    if (!body) {
        return 0;
    }

    if (shape->type == UPE_SHAPE_SPHERE) {
        UpeSphere sphere = shape->sphere;
        sphere.center = UpeVec3_Add(sphere.center, body->position);
        float r = sphere.radius;
        out->min = {sphere.center.x - r, sphere.center.y - r, sphere.center.z - r};
        out->max = {sphere.center.x + r, sphere.center.y + r, sphere.center.z + r};
        return 1;
    }
    if (shape->type == UPE_SHAPE_CAPSULE) {
        UpeCapsule capsule = shape->capsule;
        capsule.a = UpeVec3_Add(capsule.a, body->position);
        capsule.b = UpeVec3_Add(capsule.b, body->position);
        float r = capsule.radius;
        float min_x = (capsule.a.x < capsule.b.x ? capsule.a.x : capsule.b.x) - r;
        float min_y = (capsule.a.y < capsule.b.y ? capsule.a.y : capsule.b.y) - r;
        float min_z = (capsule.a.z < capsule.b.z ? capsule.a.z : capsule.b.z) - r;
        float max_x = (capsule.a.x > capsule.b.x ? capsule.a.x : capsule.b.x) + r;
        float max_y = (capsule.a.y > capsule.b.y ? capsule.a.y : capsule.b.y) + r;
        float max_z = (capsule.a.z > capsule.b.z ? capsule.a.z : capsule.b.z) + r;
        out->min = {min_x, min_y, min_z};
        out->max = {max_x, max_y, max_z};
        return 1;
    }
    if (shape->type == UPE_SHAPE_BOX) {
        UpeAabb3 box = shape->box;
        out->min = UpeVec3_Add(box.min, body->position);
        out->max = UpeVec3_Add(box.max, body->position);
        return 1;
    }
    if (shape->type == UPE_SHAPE_OBB) {
        UpeOBB obb = shape->obb;
        obb.center = UpeVec3_Add(obb.center, body->position);
        UpeOBB_ToAABB(&obb, &out->min, &out->max);
        return 1;
    }
    if (shape->type == UPE_SHAPE_CONVEX && shape->convex) {
        UpeVec3 center = UpeVec3_Add(shape->convex->centroid, body->position);
        float r = shape->convex->bounding_radius;
        out->min = {center.x - r, center.y - r, center.z - r};
        out->max = {center.x + r, center.y + r, center.z + r};
        return 1;
    }
    return 0;
}

static void UpeWorld_EnsureQueryBvh(UpeWorld *world) {
    if (!world || !world->query_bvh_dirty) {
        return;
    }

    if (world->shape_count == 0 || world->shape_capacity == 0) {
        world->query_node_count = 0;
        world->query_bound_count = 0;
        world->query_bvh_dirty = 0;
        return;
    }

    if (world->query_bound_capacity < world->shape_capacity) {
        uint32_t new_capacity = world->shape_capacity;
        UpeAabb3 *new_bounds = (UpeAabb3*)realloc(world->query_bounds, new_capacity * sizeof(UpeAabb3));
        if (!new_bounds) {
            return;
        }
        world->query_bounds = new_bounds;
        world->query_bound_capacity = new_capacity;
    }

    if (world->query_node_capacity < world->shape_count * 2u) {
        uint32_t new_capacity = world->shape_count * 2u;
        UpeBvhNode *new_nodes = (UpeBvhNode*)realloc(world->query_nodes, new_capacity * sizeof(UpeBvhNode));
        if (!new_nodes) {
            return;
        }
        world->query_nodes = new_nodes;
        world->query_node_capacity = new_capacity;
    }

    if (world->query_index_capacity < world->shape_count) {
        uint32_t new_capacity = world->shape_count;
        uint32_t *new_indices = (uint32_t*)realloc(world->query_indices, new_capacity * sizeof(uint32_t));
        if (!new_indices) {
            return;
        }
        world->query_indices = new_indices;
        world->query_index_capacity = new_capacity;
    }

    uint32_t count = 0;
    for (uint32_t i = 0; i < world->shape_capacity; ++i) {
        if (!world->shape_alive || world->shape_alive[i] == 0) {
            continue;
        }
        if (!UpeWorld_ComputeShapeAabb(world, &world->shapes[i], &world->query_bounds[i])) {
            continue;
        }
        world->query_indices[count++] = i;
    }

    world->query_bound_count = count;
    if (count > 0) {
        world->query_node_count = UpeBvh_Build(world->query_bounds, count, world->query_indices,
                                               world->query_nodes, world->query_node_capacity);
    } else {
        world->query_node_count = 0;
    }
    world->query_bvh_dirty = 0;
}

// Ray-sphere intersection
static int RaySphere(UpeVec3 origin, UpeVec3 dir, UpeSphere sphere, float max_t, float *out_t, UpeVec3 *out_normal) {
    UpeVec3 oc = UpeVec3_Sub(origin, sphere.center);
    float a = UpeVec3_Dot(dir, dir);
    float b = 2.0f * UpeVec3_Dot(oc, dir);
    float c = UpeVec3_Dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4.0f * a * c;
    
    if (discriminant < 0.0f) {
        return 0;
    }
    
    float sqrt_disc = sqrtf(discriminant);
    float t = (-b - sqrt_disc) / (2.0f * a);
    
    if (t < 0.0f || t > max_t) {
        t = (-b + sqrt_disc) / (2.0f * a);
        if (t < 0.0f || t > max_t) {
            return 0;
        }
    }
    
    *out_t = t;
    UpeVec3 hit_point = UpeVec3_Add(origin, UpeVec3_Scale(dir, t));
    UpeVec3 normal = UpeVec3_Sub(hit_point, sphere.center);
    float len = UpeVec3_Length(normal);
    *out_normal = len > 1.0e-6f ? UpeVec3_Scale(normal, 1.0f / len) : UpeVec3{1.0f, 0.0f, 0.0f};
    return 1;
}

// Ray-AABB intersection (for box shapes)
static int RayAabb(UpeVec3 origin, UpeVec3 dir, UpeAabb3 box, float max_t, float *out_t, UpeVec3 *out_normal) {
    // Slab method
    float tmin = 0.0f;
    float tmax = max_t;
    int normal_axis = 0;
    int normal_sign = 1;
    
    for (int i = 0; i < 3; ++i) {
        float d = ((float*)&dir)[i];
        float o = ((float*)&origin)[i];
        float min_val = ((float*)&box.min)[i];
        float max_val = ((float*)&box.max)[i];
        
        if (fabsf(d) < 1.0e-8f) {
            // Ray parallel to slab
            if (o < min_val || o > max_val) {
                return 0;
            }
        } else {
            float inv_d = 1.0f / d;
            float t1 = (min_val - o) * inv_d;
            float t2 = (max_val - o) * inv_d;
            
            if (t1 > t2) {
                float temp = t1;
                t1 = t2;
                t2 = temp;
            }
            
            if (t1 > tmin) {
                tmin = t1;
                normal_axis = i;
                normal_sign = (d > 0.0f) ? -1 : 1;
            }
            if (t2 < tmax) {
                tmax = t2;
            }
            
            if (tmin > tmax) {
                return 0;
            }
        }
    }
    
    if (tmin < 0.0f) {
        return 0;
    }
    
    *out_t = tmin;
    *out_normal = {0.0f, 0.0f, 0.0f};
    ((float*)out_normal)[normal_axis] = (float)normal_sign;
    return 1;
}

// Ray-capsule intersection
static int RayCapsule(UpeVec3 origin, UpeVec3 dir, UpeCapsule capsule, float max_t, float *out_t, UpeVec3 *out_normal) {
    // Simplified: treat as sphere at closest point on line segment
    UpeVec3 ab = UpeVec3_Sub(capsule.b, capsule.a);
    UpeVec3 ao = UpeVec3_Sub(origin, capsule.a);
    
    float ab_dot = UpeVec3_Dot(ab, ab);
    float t_line = ab_dot > 1.0e-6f ? UpeVec3_Dot(ao, ab) / ab_dot : 0.0f;
    t_line = t_line < 0.0f ? 0.0f : (t_line > 1.0f ? 1.0f : t_line);
    
    UpeVec3 closest = UpeVec3_Add(capsule.a, UpeVec3_Scale(ab, t_line));
    UpeSphere sphere = {closest, capsule.radius};
    return RaySphere(origin, dir, sphere, max_t, out_t, out_normal);
}

int UpeWorld_Raycast(const UpeWorld *world, UpeVec3 origin, UpeVec3 direction, float max_distance,
                     const UpeQueryFilter *filter, UpeRaycastResult *result) {
    if (!world || !result) {
        return 0;
    }
    
    // Normalize direction
    float dir_len = UpeVec3_Length(direction);
    if (dir_len < 1.0e-6f) {
        return 0;
    }
    UpeVec3 dir = UpeVec3_Scale(direction, 1.0f / dir_len);
    
    result->hit = 0;
    result->t = FLT_MAX;
    
    UpeWorld_EnsureQueryBvh((UpeWorld*)world);

    uint32_t stack_hits[256];
    uint32_t *hit_indices = stack_hits;
    uint32_t hit_capacity = 256;
    uint32_t hit_count = 0;

    if (world->query_node_count > 0 && world->query_bound_count > 0) {
        if (world->query_bound_count > hit_capacity) {
            hit_capacity = world->query_bound_count;
            hit_indices = (uint32_t*)malloc(hit_capacity * sizeof(uint32_t));
        }
        hit_count = UpeBvh_QueryRay(world->query_nodes, world->query_node_count,
                                    world->query_bounds, world->query_indices,
                                    origin, dir, max_distance, hit_indices, hit_capacity);
    } else {
        hit_count = world->shape_capacity;
        if (hit_count > hit_capacity) {
            hit_capacity = hit_count;
            hit_indices = (uint32_t*)malloc(hit_capacity * sizeof(uint32_t));
        }
        for (uint32_t i = 0; i < hit_count; ++i) {
            hit_indices[i] = i;
        }
    }

    for (uint32_t i = 0; i < hit_count; ++i) {
        uint32_t shape_index = hit_indices[i];
        if (shape_index >= world->shape_capacity) {
            continue;
        }
        if (!world->shape_alive || world->shape_alive[shape_index] == 0) {
            continue;
        }

        UpeShape *shape = &world->shapes[shape_index];
        if (!UpeWorld_QueryFilterAllowsShape(world, shape, filter)) {
            continue;
        }
        const UpeBody *body = UpeWorld_FindBodyById(world, shape->body_id);
        if (!body) {
            continue;
        }

        float t;
        UpeVec3 normal;
        int hit = 0;

        if (shape->type == UPE_SHAPE_SPHERE) {
            UpeSphere sphere = shape->sphere;
            sphere.center = UpeVec3_Add(sphere.center, body->position);
            hit = RaySphere(origin, dir, sphere, max_distance, &t, &normal);
        }
        else if (shape->type == UPE_SHAPE_BOX) {
            UpeAabb3 box = shape->box;
            box.min = UpeVec3_Add(box.min, body->position);
            box.max = UpeVec3_Add(box.max, body->position);
            hit = RayAabb(origin, dir, box, max_distance, &t, &normal);
        }
        else if (shape->type == UPE_SHAPE_CAPSULE) {
            UpeCapsule capsule = shape->capsule;
            capsule.a = UpeVec3_Add(capsule.a, body->position);
            capsule.b = UpeVec3_Add(capsule.b, body->position);
            hit = RayCapsule(origin, dir, capsule, max_distance, &t, &normal);
        }

        if (hit && t < result->t) {
            result->hit = 1;
            result->t = t;
            result->position = UpeVec3_Add(origin, UpeVec3_Scale(dir, t));
            result->normal = normal;
            result->body_id = shape->body_id;
        }
    }

    if (hit_indices != stack_hits) {
        free(hit_indices);
    }
    
    return result->hit;
}

int UpeWorld_RaycastAll(const UpeWorld *world, UpeVec3 origin, UpeVec3 direction, float max_distance,
                        const UpeQueryFilter *filter, UpeRaycastResult *results, uint32_t max_results) {
    if (!world || !results || max_results == 0) {
        return 0;
    }
    
    // Normalize direction
    float dir_len = UpeVec3_Length(direction);
    if (dir_len < 1.0e-6f) {
        return 0;
    }
    UpeVec3 dir = UpeVec3_Scale(direction, 1.0f / dir_len);
    
    uint32_t hit_count = 0;
    
    UpeWorld_EnsureQueryBvh((UpeWorld*)world);

    uint32_t stack_hits[256];
    uint32_t *hit_indices = stack_hits;
    uint32_t hit_capacity = 256;
    uint32_t candidate_count = 0;

    if (world->query_node_count > 0 && world->query_bound_count > 0) {
        if (world->query_bound_count > hit_capacity) {
            hit_capacity = world->query_bound_count;
            hit_indices = (uint32_t*)malloc(hit_capacity * sizeof(uint32_t));
        }
        candidate_count = UpeBvh_QueryRay(world->query_nodes, world->query_node_count,
                                          world->query_bounds, world->query_indices,
                                          origin, dir, max_distance, hit_indices, hit_capacity);
    } else {
        candidate_count = world->shape_capacity;
        if (candidate_count > hit_capacity) {
            hit_capacity = candidate_count;
            hit_indices = (uint32_t*)malloc(hit_capacity * sizeof(uint32_t));
        }
        for (uint32_t i = 0; i < candidate_count; ++i) {
            hit_indices[i] = i;
        }
    }

    // Test candidate shapes
    for (uint32_t i = 0; i < candidate_count; ++i) {
        if (hit_count >= max_results) {
            break;
        }
        uint32_t shape_index = hit_indices[i];
        if (shape_index >= world->shape_capacity) {
            continue;
        }
        if (!world->shape_alive || world->shape_alive[shape_index] == 0) {
            continue;
        }

        UpeShape *shape = &world->shapes[shape_index];
        if (!UpeWorld_QueryFilterAllowsShape(world, shape, filter)) {
            continue;
        }
        const UpeBody *body = UpeWorld_FindBodyById(world, shape->body_id);
        if (!body) {
            continue;
        }

        float t;
        UpeVec3 normal;
        int hit = 0;

        if (shape->type == UPE_SHAPE_SPHERE) {
            UpeSphere sphere = shape->sphere;
            sphere.center = UpeVec3_Add(sphere.center, body->position);
            hit = RaySphere(origin, dir, sphere, max_distance, &t, &normal);
        }
        else if (shape->type == UPE_SHAPE_BOX) {
            UpeAabb3 box = shape->box;
            box.min = UpeVec3_Add(box.min, body->position);
            box.max = UpeVec3_Add(box.max, body->position);
            hit = RayAabb(origin, dir, box, max_distance, &t, &normal);
        }
        else if (shape->type == UPE_SHAPE_CAPSULE) {
            UpeCapsule capsule = shape->capsule;
            capsule.a = UpeVec3_Add(capsule.a, body->position);
            capsule.b = UpeVec3_Add(capsule.b, body->position);
            hit = RayCapsule(origin, dir, capsule, max_distance, &t, &normal);
        }

        if (hit) {
            results[hit_count].hit = 1;
            results[hit_count].t = t;
            results[hit_count].position = UpeVec3_Add(origin, UpeVec3_Scale(dir, t));
            results[hit_count].normal = normal;
            results[hit_count].body_id = shape->body_id;
            hit_count++;
        }
    }

    if (hit_indices != stack_hits) {
        free(hit_indices);
    }
    
    return hit_count;
}

/*============================================================================
  Batch Raycast Query
  
  Process multiple rays efficiently in a single call. Useful for:
  - AI line-of-sight checks (multiple agents)
  - Particle tracers
  - Multi-sample rendering effects
  - Projectile spread patterns
============================================================================*/

uint32_t UpeWorld_RaycastBatch(const UpeWorld *world, 
                               const UpeVec3 *origins, 
                               const UpeVec3 *directions, 
                               const float *max_distances,
                               uint32_t ray_count,
                               const UpeQueryFilter *filter, 
                               UpeRaycastResult *results) {
    if (!world || !origins || !directions || !max_distances || !results || ray_count == 0) {
        return 0;
    }

    UpeWorld_EnsureQueryBvh((UpeWorld*)world);

    uint32_t stack_hits[256];
    uint32_t *hit_indices = stack_hits;
    uint32_t hit_capacity = 256;
    if (world->query_node_count > 0 && world->query_bound_count > hit_capacity) {
        hit_capacity = world->query_bound_count;
        hit_indices = (uint32_t*)malloc(hit_capacity * sizeof(uint32_t));
    }
    
    uint32_t total_hits = 0;
    
    /* Process each ray */
    for (uint32_t ray_idx = 0; ray_idx < ray_count; ++ray_idx) {
        UpeVec3 origin = origins[ray_idx];
        UpeVec3 direction = directions[ray_idx];
        float max_distance = max_distances[ray_idx];
        
        /* Normalize direction */
        float dir_len = UpeVec3_Length(direction);
        if (dir_len < 1.0e-6f) {
            results[ray_idx].hit = 0;
            continue;
        }
        UpeVec3 dir = UpeVec3_Scale(direction, 1.0f / dir_len);
        
        /* Initialize result */
        results[ray_idx].hit = 0;
        results[ray_idx].t = FLT_MAX;
        
        uint32_t candidate_count = 0;
        if (world->query_node_count > 0 && world->query_bound_count > 0) {
            candidate_count = UpeBvh_QueryRay(world->query_nodes, world->query_node_count,
                                              world->query_bounds, world->query_indices,
                                              origin, dir, max_distance, hit_indices, hit_capacity);
        } else {
            candidate_count = world->shape_capacity;
            if (candidate_count > hit_capacity) {
                hit_capacity = candidate_count;
                if (hit_indices != stack_hits) {
                    free(hit_indices);
                }
                hit_indices = (uint32_t*)malloc(hit_capacity * sizeof(uint32_t));
            }
            for (uint32_t i = 0; i < candidate_count; ++i) {
                hit_indices[i] = i;
            }
        }

        /* Test candidate shapes */
        for (uint32_t c = 0; c < candidate_count; ++c) {
            uint32_t shape_index = hit_indices[c];
            if (shape_index >= world->shape_capacity) {
                continue;
            }
            if (!world->shape_alive || world->shape_alive[shape_index] == 0) {
                continue;
            }
            
            UpeShape *shape = &world->shapes[shape_index];
            if (!UpeWorld_QueryFilterAllowsShape(world, shape, filter)) {
                continue;
            }
            const UpeBody *body = UpeWorld_FindBodyById(world, shape->body_id);
            if (!body) {
                continue;
            }
            
            float t;
            UpeVec3 normal;
            int hit = 0;
            
            if (shape->type == UPE_SHAPE_SPHERE) {
                UpeSphere sphere = shape->sphere;
                sphere.center = UpeVec3_Add(sphere.center, body->position);
                hit = RaySphere(origin, dir, sphere, max_distance, &t, &normal);
            }
            else if (shape->type == UPE_SHAPE_BOX) {
                UpeAabb3 box = shape->box;
                box.min = UpeVec3_Add(box.min, body->position);
                box.max = UpeVec3_Add(box.max, body->position);
                hit = RayAabb(origin, dir, box, max_distance, &t, &normal);
            }
            else if (shape->type == UPE_SHAPE_CAPSULE) {
                UpeCapsule capsule = shape->capsule;
                capsule.a = UpeVec3_Add(capsule.a, body->position);
                capsule.b = UpeVec3_Add(capsule.b, body->position);
                hit = RayCapsule(origin, dir, capsule, max_distance, &t, &normal);
            }
            
            if (hit && t < results[ray_idx].t) {
                results[ray_idx].hit = 1;
                results[ray_idx].t = t;
                results[ray_idx].position = UpeVec3_Add(origin, UpeVec3_Scale(dir, t));
                results[ray_idx].normal = normal;
                results[ray_idx].body_id = shape->body_id;
            }
        }
        
        if (results[ray_idx].hit) {
            total_hits++;
        }
    }

    if (hit_indices != stack_hits) {
        free(hit_indices);
    }
    
    return total_hits;
}

// Overlap query helpers: test if two AABBs overlap
static int AabbAabbOverlap(UpeAabb3 aabb1, UpeAabb3 aabb2) {
    if (aabb1.min.x > aabb2.max.x || aabb1.max.x < aabb2.min.x) return 0;
    if (aabb1.min.y > aabb2.max.y || aabb1.max.y < aabb2.min.y) return 0;
    if (aabb1.min.z > aabb2.max.z || aabb1.max.z < aabb2.min.z) return 0;
    return 1;
}

// Helper to compute AABB from a shape
static UpeAabb3 ShapeAabb(const struct UpeShape *shape) {
    switch (shape->type) {
        case UPE_SHAPE_SPHERE: {
            UpeVec3 r = {shape->sphere.radius, shape->sphere.radius, shape->sphere.radius};
            UpeAabb3 aabb;
            aabb.min = UpeVec3_Sub(shape->sphere.center, r);
            aabb.max = UpeVec3_Add(shape->sphere.center, r);
            return aabb;
        }
        case UPE_SHAPE_CAPSULE: {
            UpeVec3 r = {shape->capsule.radius, shape->capsule.radius, shape->capsule.radius};
            UpeVec3 min_a = UpeVec3_Sub(shape->capsule.a, r);
            UpeVec3 max_a = UpeVec3_Add(shape->capsule.a, r);
            UpeVec3 min_b = UpeVec3_Sub(shape->capsule.b, r);
            UpeVec3 max_b = UpeVec3_Add(shape->capsule.b, r);
            UpeAabb3 aabb;
            aabb.min.x = (min_a.x < min_b.x ? min_a.x : min_b.x);
            aabb.min.y = (min_a.y < min_b.y ? min_a.y : min_b.y);
            aabb.min.z = (min_a.z < min_b.z ? min_a.z : min_b.z);
            aabb.max.x = (max_a.x > max_b.x ? max_a.x : max_b.x);
            aabb.max.y = (max_a.y > max_b.y ? max_a.y : max_b.y);
            aabb.max.z = (max_a.z > max_b.z ? max_a.z : max_b.z);
            return aabb;
        }
        case UPE_SHAPE_BOX:
            return shape->box;
        case UPE_SHAPE_CONVEX: {
            if (shape->convex) {
                UpeVec3 center = shape->convex->centroid;
                float r = shape->convex->bounding_radius;
                UpeAabb3 aabb;
                aabb.min = {center.x - r, center.y - r, center.z - r};
                aabb.max = {center.x + r, center.y + r, center.z + r};
                return aabb;
            }
            return shape->box;
        }
        case UPE_SHAPE_OBB: {
            UpeAabb3 aabb;
            UpeOBB_ToAABB(&shape->obb, &aabb.min, &aabb.max);
            return aabb;
        }
        case UPE_SHAPE_MESH:
            return shape->box;
        default:
            return shape->box;
    }
}

uint32_t UpeWorld_OverlapSphere(UpeWorld *world, UpeSphere sphere, UpeOverlapResult *results, uint32_t max_count) {
    if (!world || !results || max_count == 0) return 0;
    
    uint32_t hit_count = 0;
    
    // Compute query AABB from sphere
    UpeVec3 radius_vec = {sphere.radius, sphere.radius, sphere.radius};
    UpeAabb3 query_aabb;
    query_aabb.min = UpeVec3_Sub(sphere.center, radius_vec);
    query_aabb.max = UpeVec3_Add(sphere.center, radius_vec);
    
    // Iterate through world shapes and test overlap
    for (uint32_t shape_idx = 0; shape_idx < world->shape_count; shape_idx++) {
        struct UpeShape *shape = world->shapes + shape_idx;
        
        // Compute shape AABB and test overlap
        UpeAabb3 shape_aabb = ShapeAabb(shape);
        if (AabbAabbOverlap(query_aabb, shape_aabb)) {
            if (hit_count < max_count) {
                results[hit_count].body_id = shape->body_id;
                hit_count++;
            } else {
                break;
            }
        }
    }
    
    return hit_count;
}

uint32_t UpeWorld_OverlapBox(UpeWorld *world, UpeAabb3 box, UpeOverlapResult *results, uint32_t max_count) {
    if (!world || !results || max_count == 0) return 0;
    
    uint32_t hit_count = 0;
    
    // Iterate through world shapes and test overlap
    for (uint32_t shape_idx = 0; shape_idx < world->shape_count; shape_idx++) {
        struct UpeShape *shape = world->shapes + shape_idx;
        
        // Compute shape AABB and test overlap with query box
        UpeAabb3 shape_aabb = ShapeAabb(shape);
        if (AabbAabbOverlap(box, shape_aabb)) {
            if (hit_count < max_count) {
                results[hit_count].body_id = shape->body_id;
                hit_count++;
            } else {
                break;
            }
        }
    }
    
    return hit_count;
}

uint32_t UpeWorld_OverlapCapsule(UpeWorld *world, UpeCapsule capsule, UpeOverlapResult *results, uint32_t max_count) {
    if (!world || !results || max_count == 0) return 0;
    
    uint32_t hit_count = 0;
    
    // Compute query AABB from capsule
    UpeVec3 radius_vec = {capsule.radius, capsule.radius, capsule.radius};
    UpeVec3 min_a = UpeVec3_Sub(capsule.a, radius_vec);
    UpeVec3 max_a = UpeVec3_Add(capsule.a, radius_vec);
    UpeVec3 min_b = UpeVec3_Sub(capsule.b, radius_vec);
    UpeVec3 max_b = UpeVec3_Add(capsule.b, radius_vec);
    
    UpeAabb3 query_aabb;
    query_aabb.min.x = (min_a.x < min_b.x ? min_a.x : min_b.x);
    query_aabb.min.y = (min_a.y < min_b.y ? min_a.y : min_b.y);
    query_aabb.min.z = (min_a.z < min_b.z ? min_a.z : min_b.z);
    query_aabb.max.x = (max_a.x > max_b.x ? max_a.x : max_b.x);
    query_aabb.max.y = (max_a.y > max_b.y ? max_a.y : max_b.y);
    query_aabb.max.z = (max_a.z > max_b.z ? max_a.z : max_b.z);
    
    // Iterate through world shapes and test overlap
    for (uint32_t shape_idx = 0; shape_idx < world->shape_count; shape_idx++) {
        struct UpeShape *shape = world->shapes + shape_idx;
        
        // Compute shape AABB and test overlap
        UpeAabb3 shape_aabb = ShapeAabb(shape);
        if (AabbAabbOverlap(query_aabb, shape_aabb)) {
            if (hit_count < max_count) {
                results[hit_count].body_id = shape->body_id;
                hit_count++;
            } else {
                break;
            }
        }
    }
    
    return hit_count;
}

// ==================== Sweep Queries ====================

int UpeWorld_SweepSphere(UpeWorld *world, UpeSphere sphere, UpeVec3 velocity, float max_distance,
                        const UpeQueryFilter *filter, UpeSweepResult *result) {
    if (!world || !result) return 0;
    
    // Initialize result as no hit
    result->hit = 0;
    result->toi = 1.0f;
    result->body_id = 0xFFFFFFFF;
    
    // Early exit if velocity is zero
    float vel_mag = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
    if (vel_mag < 1e-6f) return 0;
    
    // Normalize dt to be 1.0 for sweep distance
    float dt = (max_distance > 0.0f) ? max_distance / vel_mag : 1.0f;
    
    // Iterate through all shapes and find earliest hit
    for (uint32_t shape_idx = 0; shape_idx < world->shape_count; shape_idx++) {
        struct UpeShape *shape = world->shapes + shape_idx;
        
        // Apply filter if provided
        if (!UpeWorld_QueryFilterAllowsShape(world, shape, filter)) {
            continue;
        }
        
        UpeCcdResult ccd;
        ccd.hit = 0;
        
        // Test CCD based on shape type
        switch (shape->type) {
            case UPE_SHAPE_SPHERE: {
                UpeVec3 zero_vel = {0, 0, 0};
                ccd = UpeCcd_SphereSphere(sphere, velocity, shape->sphere, zero_vel, dt);
                break;
            }
            case UPE_SHAPE_CAPSULE: {
                UpeVec3 zero_vel = {0, 0, 0};
                ccd = UpeCcd_SphereCapsule(sphere, velocity, shape->capsule, zero_vel, dt);
                break;
            }
            case UPE_SHAPE_BOX: {
                UpeVec3 zero_vel = {0, 0, 0};
                ccd = UpeCcd_SphereBox(sphere, velocity, shape->box, zero_vel, dt);
                break;
            }
            case UPE_SHAPE_OBB: {
                // For OBB, convert to AABB in local space (simplified for now)
                UpeVec3 zero_vel = {0, 0, 0};
                UpeAabb3 aabb;
                aabb.min = shape->obb.center;
                aabb.max = shape->obb.center;
                // Simple AABB expansion from half-extents
                aabb.min.x -= shape->obb.half_extents.x;
                aabb.min.y -= shape->obb.half_extents.y;
                aabb.min.z -= shape->obb.half_extents.z;
                aabb.max.x += shape->obb.half_extents.x;
                aabb.max.y += shape->obb.half_extents.y;
                aabb.max.z += shape->obb.half_extents.z;
                ccd = UpeCcd_SphereBox(sphere, velocity, aabb, zero_vel, dt);
                break;
            }
            case UPE_SHAPE_CONVEX: {
                if (shape->convex) {
                    UpeVec3 zero_vel = {0, 0, 0};
                    UpeSphere convex_bounds;
                    convex_bounds.center = shape->convex->centroid;
                    convex_bounds.radius = shape->convex->bounding_radius;
                    ccd = UpeCcd_SphereSphere(sphere, velocity, convex_bounds, zero_vel, dt);
                }
                break;
            }
            case UPE_SHAPE_MESH: {
                if (shape->mesh) {
                    ccd = UpeCcd_SphereTriMesh(sphere, velocity, shape->mesh, dt);
                }
                break;
            }
            default:
                break;
        }
        
        // Update result if this hit is earlier
        if (ccd.hit && ccd.toi < result->toi) {
            result->hit = 1;
            result->toi = ccd.toi;
            result->position = ccd.position;
            result->normal = ccd.normal;
            result->body_id = shape->body_id;
        }
    }
    
    return result->hit;
}

int UpeWorld_SweepBox(UpeWorld *world, UpeAabb3 box, UpeVec3 velocity, float max_distance,
                     const UpeQueryFilter *filter, UpeSweepResult *result) {
    if (!world || !result) return 0;
    
    // Initialize result as no hit
    result->hit = 0;
    result->toi = 1.0f;
    result->body_id = 0xFFFFFFFF;
    
    // Early exit if velocity is zero
    float vel_mag = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
    if (vel_mag < 1e-6f) return 0;
    
    // Normalize dt to be 1.0 for sweep distance
    float dt = (max_distance > 0.0f) ? max_distance / vel_mag : 1.0f;
    
    // Iterate through all shapes and find earliest hit
    for (uint32_t shape_idx = 0; shape_idx < world->shape_count; shape_idx++) {
        struct UpeShape *shape = world->shapes + shape_idx;
        
        // Apply filter if provided
        if (!UpeWorld_QueryFilterAllowsShape(world, shape, filter)) {
            continue;
        }
        
        UpeCcdResult ccd;
        ccd.hit = 0;
        
        // Test CCD based on shape type
        UpeVec3 zero_vel = {0, 0, 0};
        switch (shape->type) {
            case UPE_SHAPE_SPHERE: {
                // Swap arguments: box is moving, sphere is static
                ccd = UpeCcd_SphereBox(shape->sphere, zero_vel, box, velocity, dt);
                break;
            }
            case UPE_SHAPE_CAPSULE: {
                ccd = UpeCcd_CapsuleBox(shape->capsule, zero_vel, box, velocity, dt);
                break;
            }
            case UPE_SHAPE_BOX: {
                ccd = UpeCcd_BoxBox(box, velocity, shape->box, zero_vel, dt);
                break;
            }
            case UPE_SHAPE_OBB: {
                // For OBB, convert to AABB (simplified for now)
                UpeAabb3 aabb;
                aabb.min = shape->obb.center;
                aabb.max = shape->obb.center;
                aabb.min.x -= shape->obb.half_extents.x;
                aabb.min.y -= shape->obb.half_extents.y;
                aabb.min.z -= shape->obb.half_extents.z;
                aabb.max.x += shape->obb.half_extents.x;
                aabb.max.y += shape->obb.half_extents.y;
                aabb.max.z += shape->obb.half_extents.z;
                ccd = UpeCcd_BoxBox(box, velocity, aabb, zero_vel, dt);
                break;
            }
            case UPE_SHAPE_CONVEX: {
                if (shape->convex) {
                    UpeVec3 center = shape->convex->centroid;
                    float r = shape->convex->bounding_radius;
                    UpeAabb3 bounds;
                    bounds.min = {center.x - r, center.y - r, center.z - r};
                    bounds.max = {center.x + r, center.y + r, center.z + r};
                    ccd = UpeCcd_BoxBox(box, velocity, bounds, zero_vel, dt);
                }
                break;
            }
            case UPE_SHAPE_MESH: {
                if (shape->mesh && shape->mesh->vertex_count > 0) {
                    UpeVec3 min_v = shape->mesh->vertices[0];
                    UpeVec3 max_v = shape->mesh->vertices[0];
                    for (uint32_t vi = 1; vi < shape->mesh->vertex_count; ++vi) {
                        min_v = UpeVec3_Min(min_v, shape->mesh->vertices[vi]);
                        max_v = UpeVec3_Max(max_v, shape->mesh->vertices[vi]);
                    }

                    UpeVec3 box_center = UpeVec3_Scale(UpeVec3_Add(box.min, box.max), 0.5f);
                    UpeVec3 half_extents = UpeVec3_Scale(UpeVec3_Sub(box.max, box.min), 0.5f);
                    float box_radius = UpeVec3_Length(half_extents);
                    UpeSphere sweep_sphere;
                    sweep_sphere.center = box_center;
                    sweep_sphere.radius = box_radius;

                    UpeCcdResult mesh_hit = UpeCcd_SphereTriMesh(sweep_sphere, velocity, shape->mesh, dt);
                    if (mesh_hit.hit) {
                        ccd = mesh_hit;
                    } else {
                        UpeAabb3 mesh_bounds;
                        mesh_bounds.min = min_v;
                        mesh_bounds.max = max_v;
                        ccd = UpeCcd_BoxBox(box, velocity, mesh_bounds, zero_vel, dt);
                    }
                }
                break;
            }
            default:
                break;
        }
        
        // Update result if this hit is earlier
        if (ccd.hit && ccd.toi < result->toi) {
            result->hit = 1;
            result->toi = ccd.toi;
            result->position = ccd.position;
            result->normal = ccd.normal;
            result->body_id = shape->body_id;
        }
    }
    
    return result->hit;
}

int UpeWorld_SweepCapsule(UpeWorld *world, UpeCapsule capsule, UpeVec3 velocity, float max_distance,
                         const UpeQueryFilter *filter, UpeSweepResult *result) {
    if (!world || !result) return 0;
    
    // Initialize result as no hit
    result->hit = 0;
    result->toi = 1.0f;
    result->body_id = 0xFFFFFFFF;
    
    // Early exit if velocity is zero
    float vel_mag = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
    if (vel_mag < 1e-6f) return 0;
    
    // Normalize dt to be 1.0 for sweep distance
    float dt = (max_distance > 0.0f) ? max_distance / vel_mag : 1.0f;
    
    // Iterate through all shapes and find earliest hit
    for (uint32_t shape_idx = 0; shape_idx < world->shape_count; shape_idx++) {
        struct UpeShape *shape = world->shapes + shape_idx;
        
        // Apply filter if provided
        if (!UpeWorld_QueryFilterAllowsShape(world, shape, filter)) {
            continue;
        }
        
        UpeCcdResult ccd;
        ccd.hit = 0;
        
        // Test CCD based on shape type
        UpeVec3 zero_vel = {0, 0, 0};
        switch (shape->type) {
            case UPE_SHAPE_SPHERE: {
                // Swap to use sphere-capsule CCD
                ccd = UpeCcd_SphereCapsule(shape->sphere, zero_vel, capsule, velocity, dt);
                break;
            }
            case UPE_SHAPE_CAPSULE: {
                ccd = UpeCcd_CapsuleCapsule(capsule, velocity, shape->capsule, zero_vel, dt);
                break;
            }
            case UPE_SHAPE_BOX: {
                ccd = UpeCcd_CapsuleBox(capsule, velocity, shape->box, zero_vel, dt);
                break;
            }
            case UPE_SHAPE_OBB: {
                // For OBB, convert to AABB (simplified for now)
                UpeAabb3 aabb;
                aabb.min = shape->obb.center;
                aabb.max = shape->obb.center;
                aabb.min.x -= shape->obb.half_extents.x;
                aabb.min.y -= shape->obb.half_extents.y;
                aabb.min.z -= shape->obb.half_extents.z;
                aabb.max.x += shape->obb.half_extents.x;
                aabb.max.y += shape->obb.half_extents.y;
                aabb.max.z += shape->obb.half_extents.z;
                ccd = UpeCcd_CapsuleBox(capsule, velocity, aabb, zero_vel, dt);
                break;
            }
            case UPE_SHAPE_CONVEX: {
                if (shape->convex) {
                    UpeVec3 center = shape->convex->centroid;
                    float r = shape->convex->bounding_radius;
                    UpeAabb3 bounds;
                    bounds.min = {center.x - r, center.y - r, center.z - r};
                    bounds.max = {center.x + r, center.y + r, center.z + r};
                    ccd = UpeCcd_CapsuleBox(capsule, velocity, bounds, zero_vel, dt);
                }
                break;
            }
            case UPE_SHAPE_MESH: {
                if (shape->mesh) {
                    ccd = UpeCcd_CapsuleTriMesh(capsule, velocity, shape->mesh, dt);
                }
                break;
            }
            default:
                break;
        }
        
        // Update result if this hit is earlier
        if (ccd.hit && ccd.toi < result->toi) {
            result->hit = 1;
            result->toi = ccd.toi;
            result->position = ccd.position;
            result->normal = ccd.normal;
            result->body_id = shape->body_id;
        }
    }
    
    return result->hit;
}
