#include "upe/upe_frame.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// REFERENCE FRAME SYSTEM
//
// Enables correct physics in rotating/accelerating frames
// - Inertial: fixed w.r.t. fixed stars
// - Rotating: spin around fixed axis
// - Local: moving origin (floating origin for space games)
// ============================================================================

typedef enum UpeFrameType {
    UPE_FRAME_INERTIAL,      // Non-accelerating, non-rotating
    UPE_FRAME_ROTATING,      // Spinning around fixed axis
    UPE_FRAME_COROTATING,    // Follows orbital body (co-rotating)
    UPE_FRAME_LOCAL          // Floating origin (e.g., camera-relative)
} UpeFrameType;

typedef struct UpeReferenceFrame {
    UpeFrameType type;
    
    // Position and orientation
    UpeVec3 origin;
    UpeVec3 axis_x;
    UpeVec3 axis_y;
    UpeVec3 axis_z;
    
    // Rotation state (for rotating frames)
    float angular_velocity_w;   // rad/s around principal axis
    UpeVec3 rotation_axis;      // Unit axis of rotation
    
    // Acceleration (for non-inertial frames)
    UpeVec3 acceleration;       // Frame acceleration
    
    // Coriolis/centrifugal parameters
    float centrifugal_enabled;  // 0.0 = off, 1.0 = on
    float coriolis_enabled;     // 0.0 = off, 1.0 = on
    
} UpeReferenceFrame;

// Create inertial frame (fixed reference)
UpeReferenceFrame UpeFrame_CreateInertial(UpeVec3 origin) {
    UpeReferenceFrame frame = {
        .type = UPE_FRAME_INERTIAL,
        .origin = origin,
        .axis_x = {1.0f, 0.0f, 0.0f},
        .axis_y = {0.0f, 1.0f, 0.0f},
        .axis_z = {0.0f, 0.0f, 1.0f},
        .angular_velocity_w = 0.0f,
        .rotation_axis = {0.0f, 1.0f, 0.0f},
        .acceleration = {0.0f, 0.0f, 0.0f},
        .centrifugal_enabled = 0.0f,
        .coriolis_enabled = 0.0f
    };
    return frame;
}

// Create rotating frame (spinning around axis)
UpeReferenceFrame UpeFrame_CreateRotating(UpeVec3 origin, UpeVec3 rotation_axis, float omega_rad_per_s) {
    // Normalize rotation axis
    float len = sqrtf(rotation_axis.x * rotation_axis.x + rotation_axis.y * rotation_axis.y + rotation_axis.z * rotation_axis.z);
    UpeVec3 axis_norm = {
        len > 1e-6f ? rotation_axis.x / len : 0.0f,
        len > 1e-6f ? rotation_axis.y / len : 1.0f,
        len > 1e-6f ? rotation_axis.z / len : 0.0f
    };
    
    UpeReferenceFrame frame = {
        .type = UPE_FRAME_ROTATING,
        .origin = origin,
        .axis_x = {1.0f, 0.0f, 0.0f},
        .axis_y = {0.0f, 1.0f, 0.0f},
        .axis_z = {0.0f, 0.0f, 1.0f},
        .angular_velocity_w = omega_rad_per_s,
        .rotation_axis = axis_norm,
        .acceleration = {0.0f, 0.0f, 0.0f},
        .centrifugal_enabled = 1.0f,
        .coriolis_enabled = 1.0f
    };
    return frame;
}

// Transform point FROM inertial TO frame coordinates
UpeVec3 UpeFrame_PointToFrame(const UpeReferenceFrame *frame, UpeVec3 inertial_point) {
    if (!frame) return inertial_point;
    
    // Translate
    UpeVec3 rel = {
        inertial_point.x - frame->origin.x,
        inertial_point.y - frame->origin.y,
        inertial_point.z - frame->origin.z
    };
    
    // Rotate to frame coordinates (frame axes as basis)
    UpeVec3 frame_point = {
        rel.x * frame->axis_x.x + rel.y * frame->axis_x.y + rel.z * frame->axis_x.z,
        rel.x * frame->axis_y.x + rel.y * frame->axis_y.y + rel.z * frame->axis_y.z,
        rel.x * frame->axis_z.x + rel.y * frame->axis_z.y + rel.z * frame->axis_z.z
    };
    
    return frame_point;
}

// Transform point FROM frame TO inertial coordinates
UpeVec3 UpeFrame_PointFromFrame(const UpeReferenceFrame *frame, UpeVec3 frame_point) {
    if (!frame) return frame_point;
    
    // Inverse rotate (frame axes form orthonormal basis)
    UpeVec3 rel = {
        frame_point.x * frame->axis_x.x + frame_point.y * frame->axis_y.x + frame_point.z * frame->axis_z.x,
        frame_point.x * frame->axis_x.y + frame_point.y * frame->axis_y.y + frame_point.z * frame->axis_z.y,
        frame_point.x * frame->axis_x.z + frame_point.y * frame->axis_y.z + frame_point.z * frame->axis_z.z
    };
    
    // Translate
    UpeVec3 inertial_point = {
        rel.x + frame->origin.x,
        rel.y + frame->origin.y,
        rel.z + frame->origin.z
    };
    
    return inertial_point;
}

// Compute Coriolis acceleration: a_coriolis = -2 * ω × v
UpeVec3 UpeFrame_ComputeCoriolisAccel(const UpeReferenceFrame *frame, UpeVec3 velocity_in_frame) {
    if (!frame || frame->coriolis_enabled < 0.5f || frame->angular_velocity_w < 1e-6f) {
        return UpeVec3{0.0f, 0.0f, 0.0f};
    }
    
    // ω vector = ω_magnitude * rotation_axis
    UpeVec3 omega = {
        frame->angular_velocity_w * frame->rotation_axis.x,
        frame->angular_velocity_w * frame->rotation_axis.y,
        frame->angular_velocity_w * frame->rotation_axis.z
    };
    
    // -2 * (ω × v)
    UpeVec3 omega_cross_v = {
        omega.y * velocity_in_frame.z - omega.z * velocity_in_frame.y,
        omega.z * velocity_in_frame.x - omega.x * velocity_in_frame.z,
        omega.x * velocity_in_frame.y - omega.y * velocity_in_frame.x
    };
    
    return UpeVec3{
        -2.0f * omega_cross_v.x,
        -2.0f * omega_cross_v.y,
        -2.0f * omega_cross_v.z
    };
}

// Compute centrifugal force acceleration: a_centrifugal = -ω × (ω × r)
UpeVec3 UpeFrame_ComputeCentrifugalAccel(const UpeReferenceFrame *frame, UpeVec3 position_in_frame) {
    if (!frame || frame->centrifugal_enabled < 0.5f || frame->angular_velocity_w < 1e-6f) {
        return UpeVec3{0.0f, 0.0f, 0.0f};
    }
    
    // ω vector
    UpeVec3 omega = {
        frame->angular_velocity_w * frame->rotation_axis.x,
        frame->angular_velocity_w * frame->rotation_axis.y,
        frame->angular_velocity_w * frame->rotation_axis.z
    };
    
    // ω × r
    UpeVec3 omega_cross_r = {
        omega.y * position_in_frame.z - omega.z * position_in_frame.y,
        omega.z * position_in_frame.x - omega.x * position_in_frame.z,
        omega.x * position_in_frame.y - omega.y * position_in_frame.x
    };
    
    // -ω × (ω × r) = ω² * r_perp (centrifugal pushes outward)
    UpeVec3 neg_omega_cross_omega_cross_r = {
        omega.y * omega_cross_r.z - omega.z * omega_cross_r.y,
        omega.z * omega_cross_r.x - omega.x * omega_cross_r.z,
        omega.x * omega_cross_r.y - omega.y * omega_cross_r.x
    };
    
    return neg_omega_cross_omega_cross_r;
}

// Compute all fictitious forces in rotating frame
UpeVec3 UpeFrame_ComputeFictitious(const UpeReferenceFrame *frame, UpeVec3 pos_in_frame, UpeVec3 vel_in_frame) {
    UpeVec3 force = {0.0f, 0.0f, 0.0f};
    
    // Centrifugal
    UpeVec3 centrifugal = UpeFrame_ComputeCentrifugalAccel(frame, pos_in_frame);
    force.x += centrifugal.x;
    force.y += centrifugal.y;
    force.z += centrifugal.z;
    
    // Coriolis
    UpeVec3 coriolis = UpeFrame_ComputeCoriolisAccel(frame, vel_in_frame);
    force.x += coriolis.x;
    force.y += coriolis.y;
    force.z += coriolis.z;
    
    // Frame acceleration (pseudo-force)
    force.x -= frame->acceleration.x;
    force.y -= frame->acceleration.y;
    force.z -= frame->acceleration.z;
    
    return force;
}

// Update rotating frame rotation (integrate angular velocity)
void UpeFrame_UpdateRotation(UpeReferenceFrame *frame, float dt) {
    if (!frame || frame->angular_velocity_w < 1e-6f) {
        return;
    }
    
    // Rotation angle per timestep
    float theta = frame->angular_velocity_w * dt;
    float cos_t = cosf(theta);
    float sin_t = sinf(theta);
    float one_minus_cos = 1.0f - cos_t;
    
    // Rodriguez rotation formula: R = I + sin(θ) * K + (1-cos(θ)) * K²
    // where K is the skew-symmetric matrix of rotation axis
    float kx = frame->rotation_axis.x;
    float ky = frame->rotation_axis.y;
    float kz = frame->rotation_axis.z;
    
    // Apply rotation to frame axes
    UpeVec3 new_x = {
        frame->axis_x.x * (cos_t + one_minus_cos * kx * kx) + 
            frame->axis_x.y * (one_minus_cos * kx * ky - sin_t * kz) +
            frame->axis_x.z * (one_minus_cos * kx * kz + sin_t * ky),
        frame->axis_x.x * (one_minus_cos * kx * ky + sin_t * kz) +
            frame->axis_x.y * (cos_t + one_minus_cos * ky * ky) +
            frame->axis_x.z * (one_minus_cos * ky * kz - sin_t * kx),
        frame->axis_x.x * (one_minus_cos * kx * kz - sin_t * ky) +
            frame->axis_x.y * (one_minus_cos * ky * kz + sin_t * kx) +
            frame->axis_x.z * (cos_t + one_minus_cos * kz * kz)
    };
    
    UpeVec3 new_y = {
        frame->axis_y.x * (cos_t + one_minus_cos * kx * kx) + 
            frame->axis_y.y * (one_minus_cos * kx * ky - sin_t * kz) +
            frame->axis_y.z * (one_minus_cos * kx * kz + sin_t * ky),
        frame->axis_y.x * (one_minus_cos * kx * ky + sin_t * kz) +
            frame->axis_y.y * (cos_t + one_minus_cos * ky * ky) +
            frame->axis_y.z * (one_minus_cos * ky * kz - sin_t * kx),
        frame->axis_y.x * (one_minus_cos * kx * kz - sin_t * ky) +
            frame->axis_y.y * (one_minus_cos * ky * kz + sin_t * kx) +
            frame->axis_y.z * (cos_t + one_minus_cos * kz * kz)
    };
    
    UpeVec3 new_z = {
        frame->axis_z.x * (cos_t + one_minus_cos * kx * kx) + 
            frame->axis_z.y * (one_minus_cos * kx * ky - sin_t * kz) +
            frame->axis_z.z * (one_minus_cos * kx * kz + sin_t * ky),
        frame->axis_z.x * (one_minus_cos * kx * ky + sin_t * kz) +
            frame->axis_z.y * (cos_t + one_minus_cos * ky * ky) +
            frame->axis_z.z * (one_minus_cos * ky * kz - sin_t * kx),
        frame->axis_z.x * (one_minus_cos * kx * kz - sin_t * ky) +
            frame->axis_z.y * (one_minus_cos * ky * kz + sin_t * kx) +
            frame->axis_z.z * (cos_t + one_minus_cos * kz * kz)
    };
    
    frame->axis_x = new_x;
    frame->axis_y = new_y;
    frame->axis_z = new_z;
}

// ============================================================================
// FLOATING ORIGIN SYSTEM
// 
// Prevents jitter and precision loss at large distances
// Maintains double precision positions internally, float for simulation
// ============================================================================

typedef struct UpeFloatingOrigin {
    // Stored in double precision
    double world_x;
    double world_y;
    double world_z;
    
    // Local positions (float, relative to origin)
    float local_radius;  // When exceeded, re-base
    
} UpeFloatingOrigin;

void UpeFloatingOrigin_Init(UpeFloatingOrigin *origin) {
    if (!origin) return;
    origin->world_x = 0.0;
    origin->world_y = 0.0;
    origin->world_z = 0.0;
    origin->local_radius = 1000.0f;  // Re-base when > 1km away
}

// Convert local (float) position to world (double) position
void UpeFloatingOrigin_LocalToWorld(const UpeFloatingOrigin *origin, 
                                    float local_x, float local_y, float local_z,
                                    double *out_x, double *out_y, double *out_z) {
    if (!origin || !out_x || !out_y || !out_z) return;
    
    *out_x = origin->world_x + (double)local_x;
    *out_y = origin->world_y + (double)local_y;
    *out_z = origin->world_z + (double)local_z;
}

// Convert world (double) position to local (float) position
UpeVec3 UpeFloatingOrigin_WorldToLocal(const UpeFloatingOrigin *origin, 
                                       double world_x, double world_y, double world_z) {
    if (!origin) return UpeVec3{0.0f, 0.0f, 0.0f};
    
    return UpeVec3{
        (float)(world_x - origin->world_x),
        (float)(world_y - origin->world_y),
        (float)(world_z - origin->world_z)
    };
}

// Re-base floating origin (all bodies' positions update)
void UpeFloatingOrigin_Rebase(UpeFloatingOrigin *origin, double delta_x, double delta_y, double delta_z) {
    if (!origin) return;
    
    origin->world_x += delta_x;
    origin->world_y += delta_y;
    origin->world_z += delta_z;
    
    // Note: All body positions in local space get updated externally
}

// Check if re-basing needed
int UpeFloatingOrigin_NeedsRebase(const UpeFloatingOrigin *origin, 
                                   float max_local_x, float max_local_y, float max_local_z) {
    if (!origin) return 0;
    
    float dist_sq = max_local_x * max_local_x + max_local_y * max_local_y + max_local_z * max_local_z;
    float threshold_sq = origin->local_radius * origin->local_radius;
    
    return dist_sq > threshold_sq ? 1 : 0;
}
