#include "upe/upe_queries.h"
#include "upe/upe_shape.h"
#include "../world/upe_world.h"
#include <math.h>

// Forward declarations for helper functions from upe_queries.cpp
static int AabbAabbOverlap(UpeAabb3 aabb1, UpeAabb3 aabb2) {
    if (aabb1.min.x > aabb2.max.x || aabb1.max.x < aabb2.min.x) return 0;
    if (aabb1.min.y > aabb2.max.y || aabb1.max.y < aabb2.min.y) return 0;
    if (aabb1.min.z > aabb2.max.z || aabb1.max.z < aabb2.min.z) return 0;
    return 1;
}

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
        default:
            return shape->box;
    }
}

// Sweep queries: test collision along a swept path
int UpeWorld_SweepSphere(UpeWorld *world, UpeSphere sphere, UpeVec3 velocity, float max_distance,
                        const UpeQueryFilter *filter, UpeSweepResult *result) {
    if (!world || !result || max_distance < 1.0e-6f) return 0;
    
    UpeVec3 r_vec = {sphere.radius, sphere.radius, sphere.radius};
    UpeVec3 sweep_extent = UpeVec3_Scale(velocity, max_distance);
    
    UpeVec3 start_min = UpeVec3_Sub(sphere.center, r_vec);
    UpeVec3 start_max = UpeVec3_Add(sphere.center, r_vec);
    UpeVec3 end_min = UpeVec3_Sub(UpeVec3_Add(sphere.center, sweep_extent), r_vec);
    UpeVec3 end_max = UpeVec3_Add(UpeVec3_Add(sphere.center, sweep_extent), r_vec);
    
    UpeAabb3 swept_aabb;
    swept_aabb.min.x = (start_min.x < end_min.x ? start_min.x : end_min.x);
    swept_aabb.min.y = (start_min.y < end_min.y ? start_min.y : end_min.y);
    swept_aabb.min.z = (start_min.z < end_min.z ? start_min.z : end_min.z);
    swept_aabb.max.x = (start_max.x > end_max.x ? start_max.x : end_max.x);
    swept_aabb.max.y = (start_max.y > end_max.y ? start_max.y : end_max.y);
    swept_aabb.max.z = (start_max.z > end_max.z ? start_max.z : end_max.z);
    
    float closest_toi = 1.0f;
    int found = 0;
    
    for (uint32_t shape_idx = 0; shape_idx < world->shape_count; shape_idx++) {
        struct UpeShape *shape = world->shapes + shape_idx;
        UpeAabb3 shape_aabb = ShapeAabb(shape);
        
        if (!AabbAabbOverlap(swept_aabb, shape_aabb)) continue;
        
        UpeVec3 to_shape = UpeVec3_Sub(ShapeAabb(shape).min, sphere.center);
        float vel_len = UpeVec3_Length(velocity);
        if (vel_len < 1.0e-6f) continue;
        
        UpeVec3 vel_dir = UpeVec3_Scale(velocity, 1.0f / vel_len);
        float dist_to_shape = UpeVec3_Dot(to_shape, vel_dir);
        float toi = (dist_to_shape - sphere.radius) / vel_len;
        toi = toi < 0.0f ? 0.0f : (toi > 1.0f ? 1.0f : toi);
        
        if (toi < closest_toi) {
            closest_toi = toi;
            result->toi = toi;
            result->position = UpeVec3_Add(sphere.center, UpeVec3_Scale(velocity, toi * max_distance));
            result->normal = vel_dir;
            result->body_id = shape->body_id;
            result->hit = 1;
            found = 1;
        }
    }
    
    return found;
}

int UpeWorld_SweepBox(UpeWorld *world, UpeAabb3 box, UpeVec3 velocity, float max_distance,
                     const UpeQueryFilter *filter, UpeSweepResult *result) {
    if (!world || !result || max_distance < 1.0e-6f) return 0;
    
    UpeVec3 sweep_extent = UpeVec3_Scale(velocity, max_distance);
    UpeAabb3 swept_aabb = box;
    
    swept_aabb.min.x = (box.min.x < box.min.x + sweep_extent.x ? box.min.x : box.min.x + sweep_extent.x);
    swept_aabb.min.y = (box.min.y < box.min.y + sweep_extent.y ? box.min.y : box.min.y + sweep_extent.y);
    swept_aabb.min.z = (box.min.z < box.min.z + sweep_extent.z ? box.min.z : box.min.z + sweep_extent.z);
    swept_aabb.max.x = (box.max.x > box.max.x + sweep_extent.x ? box.max.x : box.max.x + sweep_extent.x);
    swept_aabb.max.y = (box.max.y > box.max.y + sweep_extent.y ? box.max.y : box.max.y + sweep_extent.y);
    swept_aabb.max.z = (box.max.z > box.max.z + sweep_extent.z ? box.max.z : box.max.z + sweep_extent.z);
    
    float closest_toi = 1.0f;
    int found = 0;
    
    for (uint32_t shape_idx = 0; shape_idx < world->shape_count; shape_idx++) {
        struct UpeShape *shape = world->shapes + shape_idx;
        UpeAabb3 shape_aabb = ShapeAabb(shape);
        
        if (!AabbAabbOverlap(swept_aabb, shape_aabb)) continue;
        
        UpeVec3 box_center = UpeVec3_Scale(UpeVec3_Add(box.min, box.max), 0.5f);
        UpeVec3 shape_center = UpeVec3_Scale(UpeVec3_Add(shape_aabb.min, shape_aabb.max), 0.5f);
        UpeVec3 to_shape = UpeVec3_Sub(shape_center, box_center);
        
        float vel_len = UpeVec3_Length(velocity);
        if (vel_len < 1.0e-6f) continue;
        
        UpeVec3 vel_dir = UpeVec3_Scale(velocity, 1.0f / vel_len);
        float dist_to_shape = UpeVec3_Dot(to_shape, vel_dir);
        float toi = dist_to_shape / vel_len;
        toi = toi < 0.0f ? 0.0f : (toi > 1.0f ? 1.0f : toi);
        
        if (toi < closest_toi) {
            closest_toi = toi;
            result->toi = toi;
            result->position = UpeVec3_Add(box_center, UpeVec3_Scale(velocity, toi * max_distance));
            result->normal = vel_dir;
            result->body_id = shape->body_id;
            result->hit = 1;
            found = 1;
        }
    }
    
    return found;
}

int UpeWorld_SweepCapsule(UpeWorld *world, UpeCapsule capsule, UpeVec3 velocity, float max_distance,
                         const UpeQueryFilter *filter, UpeSweepResult *result) {
    if (!world || !result || max_distance < 1.0e-6f) return 0;
    
    UpeVec3 radius_vec = {capsule.radius, capsule.radius, capsule.radius};
    UpeVec3 sweep_extent = UpeVec3_Scale(velocity, max_distance);
    
    UpeVec3 capsule_center = UpeVec3_Scale(UpeVec3_Add(capsule.a, capsule.b), 0.5f);
    UpeVec3 swept_center = UpeVec3_Add(capsule_center, sweep_extent);
    
    UpeAabb3 swept_aabb;
    swept_aabb.min = UpeVec3_Sub(capsule_center, radius_vec);
    swept_aabb.max = UpeVec3_Add(capsule_center, radius_vec);
    UpeVec3 swept_min = UpeVec3_Sub(swept_center, radius_vec);
    UpeVec3 swept_max = UpeVec3_Add(swept_center, radius_vec);
    
    swept_aabb.min.x = (swept_aabb.min.x < swept_min.x ? swept_aabb.min.x : swept_min.x);
    swept_aabb.min.y = (swept_aabb.min.y < swept_min.y ? swept_aabb.min.y : swept_min.y);
    swept_aabb.min.z = (swept_aabb.min.z < swept_min.z ? swept_aabb.min.z : swept_min.z);
    swept_aabb.max.x = (swept_aabb.max.x > swept_max.x ? swept_aabb.max.x : swept_max.x);
    swept_aabb.max.y = (swept_aabb.max.y > swept_max.y ? swept_aabb.max.y : swept_max.y);
    swept_aabb.max.z = (swept_aabb.max.z > swept_max.z ? swept_aabb.max.z : swept_max.z);
    
    float closest_toi = 1.0f;
    int found = 0;
    
    for (uint32_t shape_idx = 0; shape_idx < world->shape_count; shape_idx++) {
        struct UpeShape *shape = world->shapes + shape_idx;
        UpeAabb3 shape_aabb = ShapeAabb(shape);
        
        if (!AabbAabbOverlap(swept_aabb, shape_aabb)) continue;
        
        float vel_len = UpeVec3_Length(velocity);
        if (vel_len < 1.0e-6f) continue;
        
        UpeVec3 vel_dir = UpeVec3_Scale(velocity, 1.0f / vel_len);
        UpeVec3 to_shape = UpeVec3_Sub(shape_aabb.min, capsule_center);
        float dist_to_shape = UpeVec3_Dot(to_shape, vel_dir);
        float toi = (dist_to_shape - capsule.radius) / vel_len;
        toi = toi < 0.0f ? 0.0f : (toi > 1.0f ? 1.0f : toi);
        
        if (toi < closest_toi) {
            closest_toi = toi;
            result->toi = toi;
            result->position = UpeVec3_Add(capsule_center, UpeVec3_Scale(velocity, toi * max_distance));
            result->normal = vel_dir;
            result->body_id = shape->body_id;
            result->hit = 1;
            found = 1;
        }
    }
    
    return found;
}
