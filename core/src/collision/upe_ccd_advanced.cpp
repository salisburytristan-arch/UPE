#include "upe/upe_ccd_advanced.h"
#include "upe/upe_gjk.h"
#include <math.h>

// Conservative advancement for box-box CCD
UpeCcdResult UpeCcd_BoxBox(const UpeAabb3 *a, UpeVec3 vel_a, const UpeAabb3 *b, UpeVec3 vel_b, float dt) {
    UpeCcdResult result;
    result.hit = 0;
    result.toi = dt;
    result.position.x = 0; result.position.y = 0; result.position.z = 0;
    result.normal.x = 0; result.normal.y = 0; result.normal.z = 0;
    
    if (!a || !b) {
        return result;
    }
    
    // Relative velocity
    UpeVec3 relative_vel;
    relative_vel.x = vel_a.x - vel_b.x;
    relative_vel.y = vel_a.y - vel_b.y;
    relative_vel.z = vel_a.z - vel_b.z;
    
    float rel_speed = sqrtf(relative_vel.x * relative_vel.x + 
                           relative_vel.y * relative_vel.y + 
                           relative_vel.z * relative_vel.z);
    
    if (rel_speed < 1e-6f) {
        // No relative motion
        return result;
    }
    
    // Conservative advancement - binary search for TOI
    float t_min = 0.0f;
    float t_max = dt;
    const int max_iterations = 20;
    const float tolerance = 0.001f;
    
    for (int iter = 0; iter < max_iterations; ++iter) {
        float t_mid = (t_min + t_max) * 0.5f;
        
        // Sweep box A to time t_mid
        UpeVec3 offset;
        offset.x = relative_vel.x * t_mid;
        offset.y = relative_vel.y * t_mid;
        offset.z = relative_vel.z * t_mid;
        
        UpeAabb3 swept_a = *a;
        swept_a.min.x += offset.x;
        swept_a.min.y += offset.y;
        swept_a.min.z += offset.z;
        swept_a.max.x += offset.x;
        swept_a.max.y += offset.y;
        swept_a.max.z += offset.z;
        
        // Check overlap
        int overlapping = (swept_a.max.x >= b->min.x && swept_a.min.x <= b->max.x &&
                          swept_a.max.y >= b->min.y && swept_a.min.y <= b->max.y &&
                          swept_a.max.z >= b->min.z && swept_a.min.z <= b->max.z);
        
        if (overlapping) {
            // Overlap found, search earlier
            t_max = t_mid;
            result.hit = 1;
            result.toi = t_mid;
        } else {
            // No overlap, search later
            t_min = t_mid;
        }
        
        if (t_max - t_min < tolerance) {
            break;
        }
    }
    
    if (result.hit) {
        // Compute contact normal (simplified - axis of least penetration)
        UpeVec3 offset;
        offset.x = relative_vel.x * result.toi;
        offset.y = relative_vel.y * result.toi;
        offset.z = relative_vel.z * result.toi;
        
        UpeVec3 center_a;
        center_a.x = (a->min.x + a->max.x) * 0.5f + offset.x;
        center_a.y = (a->min.y + a->max.y) * 0.5f + offset.y;
        center_a.z = (a->min.z + a->max.z) * 0.5f + offset.z;
        
        UpeVec3 center_b;
        center_b.x = (b->min.x + b->max.x) * 0.5f;
        center_b.y = (b->min.y + b->max.y) * 0.5f;
        center_b.z = (b->min.z + b->max.z) * 0.5f;
        
        UpeVec3 dir;
        dir.x = center_b.x - center_a.x;
        dir.y = center_b.y - center_a.y;
        dir.z = center_b.z - center_a.z;
        
        float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len > 1e-6f) {
            result.normal.x = dir.x / len;
            result.normal.y = dir.y / len;
            result.normal.z = dir.z / len;
        } else {
            result.normal.x = 0; result.normal.y = 1; result.normal.z = 0;
        }
        
        result.position = center_a;
    }
    
    return result;
}

// Conservative advancement for convex-convex CCD using GJK
UpeCcdResult UpeCcd_ConvexConvex(const void *a, UpeGjkSupportFn support_a, UpeVec3 vel_a,
                                 const void *b, UpeGjkSupportFn support_b, UpeVec3 vel_b, float dt) {
    UpeCcdResult result;
    result.hit = 0;
    result.toi = dt;
    result.position.x = 0; result.position.y = 0; result.position.z = 0;
    result.normal.x = 0; result.normal.y = 0; result.normal.z = 0;
    
    if (!a || !b || !support_a || !support_b) {
        return result;
    }
    
    // Relative velocity
    UpeVec3 relative_vel;
    relative_vel.x = vel_a.x - vel_b.x;
    relative_vel.y = vel_a.y - vel_b.y;
    relative_vel.z = vel_a.z - vel_b.z;
    
    float rel_speed = sqrtf(relative_vel.x * relative_vel.x + 
                           relative_vel.y * relative_vel.y + 
                           relative_vel.z * relative_vel.z);
    
    if (rel_speed < 1e-6f) {
        // No relative motion - check for current overlap
        UpeVec3 initial_dir;
        initial_dir.x = 1; initial_dir.y = 0; initial_dir.z = 0;
        UpeGjkResult gjk_result = UpeGjk_Distance(a, support_a, b, support_b, initial_dir);
        
        if (gjk_result.intersecting || gjk_result.distance < 1e-4f) {
            result.hit = 1;
            result.toi = 0.0f;
            result.position = gjk_result.witness_a;
            
            // Compute normal from witness points
            UpeVec3 diff;
            diff.x = gjk_result.witness_b.x - gjk_result.witness_a.x;
            diff.y = gjk_result.witness_b.y - gjk_result.witness_a.y;
            diff.z = gjk_result.witness_b.z - gjk_result.witness_a.z;
            
            float len = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
            if (len > 1e-6f) {
                result.normal.x = diff.x / len;
                result.normal.y = diff.y / len;
                result.normal.z = diff.z / len;
            } else {
                result.normal.x = 0; result.normal.y = 1; result.normal.z = 0;
            }
        }
        return result;
    }
    
    // Conservative advancement - binary search for TOI
    float t_min = 0.0f;
    float t_max = dt;
    const int max_iterations = 20;
    const float tolerance = 0.001f;
    const float contact_threshold = 0.01f;  // Distance considered as contact
    
    UpeVec3 initial_dir;
    initial_dir.x = -relative_vel.x;
    initial_dir.y = -relative_vel.y;
    initial_dir.z = -relative_vel.z;
    float dir_len = sqrtf(initial_dir.x * initial_dir.x + 
                         initial_dir.y * initial_dir.y + 
                         initial_dir.z * initial_dir.z);
    if (dir_len > 1e-6f) {
        initial_dir.x /= dir_len;
        initial_dir.y /= dir_len;
        initial_dir.z /= dir_len;
    }
    
    for (int iter = 0; iter < max_iterations; ++iter) {
        float t_mid = (t_min + t_max) * 0.5f;
        
        // Note: In a full implementation, we would need to transform the shapes
        // by their velocities. For now, we assume the support functions handle
        // shape positions internally, and we just query at different times.
        // This is a simplified version - a full implementation would need to
        // pass time-dependent transformations to the support functions.
        
        UpeGjkResult gjk_result = UpeGjk_Distance(a, support_a, b, support_b, initial_dir);
        
        // Estimate distance at t_mid by adjusting for relative motion
        float motion_separation = rel_speed * (dt - t_mid);
        float effective_distance = gjk_result.distance - motion_separation;
        
        if (effective_distance < contact_threshold) {
            // Shapes are close enough - potential contact
            t_max = t_mid;
            result.hit = 1;
            result.toi = t_mid;
            
            // Store witness points and normal
            result.position = gjk_result.witness_a;
            
            UpeVec3 diff;
            diff.x = gjk_result.witness_b.x - gjk_result.witness_a.x;
            diff.y = gjk_result.witness_b.y - gjk_result.witness_a.y;
            diff.z = gjk_result.witness_b.z - gjk_result.witness_a.z;
            
            float len = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
            if (len > 1e-6f) {
                result.normal.x = diff.x / len;
                result.normal.y = diff.y / len;
                result.normal.z = diff.z / len;
            } else {
                result.normal.x = 0; result.normal.y = 1; result.normal.z = 0;
            }
        } else {
            // No contact yet, search later
            t_min = t_mid;
        }
        
        if (t_max - t_min < tolerance) {
            break;
        }
    }
    
    return result;
}
