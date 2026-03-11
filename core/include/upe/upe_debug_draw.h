#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Unified Debug Visualization Layer (48.0.0.0)
  
  Draw contacts, impulses, constraint errors, fields, flows, temperature maps, EM lines.
  Must be engine-native and dependency-free.
  
  Outputs to simple line/point/text primitives that can be rendered by any graphics API.
*/

// Debug primitive types
typedef enum UpeDebugPrimitive {
    UPE_DEBUG_LINE = 0,
    UPE_DEBUG_ARROW = 1,
    UPE_DEBUG_POINT = 2,
    UPE_DEBUG_SPHERE = 3,
    UPE_DEBUG_BOX = 4,
    UPE_DEBUG_CAPSULE = 5,
    UPE_DEBUG_TEXT = 6,
    UPE_DEBUG_AXES = 7,          // XYZ axes at a position
    UPE_DEBUG_GRID = 8
} UpeDebugPrimitive;

// Debug draw item
typedef struct UpeDebugDrawItem {
    UpeDebugPrimitive type;
    
    // Geometry
    UpeVec3 points[8];           // Up to 8 points (for boxes, text positions, etc.)
    float params[4];             // Radii, thickness, etc.
    
    // Appearance
    float color[4];              // RGBA
    float duration;              // How long to display (0 = one frame)
    int depth_test;              // Should it occlude?
    
    // Text (if UPE_DEBUG_TEXT)
    char text[128];
    
} UpeDebugDrawItem;

// Debug draw system
typedef struct UpeDebugDrawSystem {
    UpeDebugDrawItem *items;
    uint32_t item_count;
    uint32_t item_capacity;
    
    // Persistent items (don't clear each frame)
    UpeDebugDrawItem *persistent_items;
    uint32_t persistent_count;
    uint32_t persistent_capacity;
    
    // Stats
    uint32_t frame_number;
    
} UpeDebugDrawSystem;

// Initialization
void UpeDebug_Init(UpeDebugDrawSystem *debug);
void UpeDebug_Destroy(UpeDebugDrawSystem *debug);

// Frame management
void UpeDebug_BeginFrame(UpeDebugDrawSystem *debug);
void UpeDebug_EndFrame(UpeDebugDrawSystem *debug);
void UpeDebug_Clear(UpeDebugDrawSystem *debug);

// Basic primitives
void UpeDebug_Line(UpeDebugDrawSystem *debug, UpeVec3 start, UpeVec3 end,
                   float r, float g, float b, float duration);
void UpeDebug_Arrow(UpeDebugDrawSystem *debug, UpeVec3 start, UpeVec3 end,
                    float r, float g, float b, float duration);
void UpeDebug_Point(UpeDebugDrawSystem *debug, UpeVec3 position, float size,
                    float r, float g, float b, float duration);
void UpeDebug_Sphere(UpeDebugDrawSystem *debug, UpeVec3 center, float radius,
                     float r, float g, float b, float duration);
void UpeDebug_Box(UpeDebugDrawSystem *debug, UpeVec3 center, UpeVec3 half_extents,
                  UpeQuat rotation, float r, float g, float b, float duration);
void UpeDebug_Capsule(UpeDebugDrawSystem *debug, UpeVec3 point_a, UpeVec3 point_b,
                      float radius, float r, float g, float b, float duration);
void UpeDebug_Text(UpeDebugDrawSystem *debug, UpeVec3 position, const char *text,
                   float r, float g, float b, float duration);
void UpeDebug_Axes(UpeDebugDrawSystem *debug, UpeVec3 position, UpeQuat rotation,
                   float scale, float duration);
void UpeDebug_Grid(UpeDebugDrawSystem *debug, UpeVec3 origin, UpeVec3 normal,
                   uint32_t cell_count, float cell_size, float r, float g, float b, float duration);

// Physics-specific visualizations

// Contact visualization
void UpeDebug_Contact(UpeDebugDrawSystem *debug, UpeVec3 position, UpeVec3 normal,
                      float penetration_depth, float impulse_magnitude);

// Constraint error visualization
void UpeDebug_ConstraintError(UpeDebugDrawSystem *debug, UpeVec3 position_a, UpeVec3 position_b,
                              float error_magnitude);

// Rigid body state
void UpeDebug_RigidBody(UpeDebugDrawSystem *debug, UpeVec3 position, UpeQuat rotation,
                        UpeVec3 velocity, UpeVec3 angular_velocity, int is_sleeping);

// Field visualization (scalar field as height/color map)
void UpeDebug_ScalarField(UpeDebugDrawSystem *debug, const float *values,
                          UpeVec3 origin, uint32_t width, uint32_t height,
                          float cell_size, float min_val, float max_val);

// Vector field (arrows)
void UpeDebug_VectorField(UpeDebugDrawSystem *debug, const UpeVec3 *vectors,
                          UpeVec3 origin, uint32_t width, uint32_t height, uint32_t depth,
                          float cell_size, float scale);

// Temperature map (color-coded)
void UpeDebug_TemperatureField(UpeDebugDrawSystem *debug, const float *temperatures,
                               UpeVec3 origin, uint32_t width, uint32_t height,
                               float cell_size, float min_temp, float max_temp);

// Pressure map
void UpeDebug_PressureField(UpeDebugDrawSystem *debug, const float *pressures,
                            UpeVec3 origin, uint32_t width, uint32_t height,
                            float cell_size);

// Density visualization (particles or grid)
void UpeDebug_DensityField(UpeDebugDrawSystem *debug, const float *densities,
                           UpeVec3 origin, uint32_t width, uint32_t height, uint32_t depth,
                           float cell_size, float density_scale);

// EM field lines
void UpeDebug_ElectricFieldLines(UpeDebugDrawSystem *debug, UpeVec3 charge_pos,
                                 float charge, uint32_t line_count, float max_distance);
void UpeDebug_MagneticFieldLines(UpeDebugDrawSystem *debug, UpeVec3 dipole_pos,
                                 UpeVec3 dipole_moment, uint32_t line_count, float max_distance);

// Particle system visualization
void UpeDebug_Particles(UpeDebugDrawSystem *debug, const UpeVec3 *positions,
                        const float *radii, uint32_t count, float r, float g, float b);

// Cloth/soft body visualization
void UpeDebug_Cloth(UpeDebugDrawSystem *debug, const UpeVec3 *vertices,
                    const uint32_t *indices, uint32_t vertex_count, uint32_t triangle_count);

// Rope/cable visualization
void UpeDebug_Rope(UpeDebugDrawSystem *debug, const UpeVec3 *segment_positions,
                   uint32_t segment_count, float thickness);

// Fluid surface (marching cubes visualization hint)
void UpeDebug_FluidIsosurface(UpeDebugDrawSystem *debug, const float *density_field,
                              UpeVec3 origin, uint32_t width, uint32_t height, uint32_t depth,
                              float cell_size, float iso_value);

// Collision shape outlines
void UpeDebug_CollisionShape(UpeDebugDrawSystem *debug, uint32_t shape_type,
                             UpeVec3 position, UpeQuat rotation, const float *shape_params);

// Broadphase AABB visualization
void UpeDebug_AABB(UpeDebugDrawSystem *debug, UpeVec3 min_bounds, UpeVec3 max_bounds,
                   float r, float g, float b, float duration);

// Island coloring (for solver islands)
void UpeDebug_Island(UpeDebugDrawSystem *debug, uint32_t island_id, const uint32_t *body_ids,
                     uint32_t body_count, void *world);

// Fracture visualization (crack lines)
void UpeDebug_Fracture(UpeDebugDrawSystem *debug, UpeVec3 *crack_vertices, uint32_t vertex_count,
                       float damage_level);

// Wind flow visualization
void UpeDebug_WindField(UpeDebugDrawSystem *debug, const UpeVec3 *wind_vectors,
                        UpeVec3 origin, uint32_t width, uint32_t depth,
                        float cell_size, float arrow_scale);

// Stress/strain tensor visualization
void UpeDebug_StressTensor(UpeDebugDrawSystem *debug, UpeVec3 position, const float *stress_tensor,
                           float scale);

// Path visualization (for procedural locomotion, AI, etc.)
void UpeDebug_Path(UpeDebugDrawSystem *debug, const UpeVec3 *waypoints, uint32_t waypoint_count,
                   float r, float g, float b, float duration);

// Bounding volume hierarchy (BVH) visualization
void UpeDebug_BVH(UpeDebugDrawSystem *debug, void *bvh_root, uint32_t max_depth);

// Trigger volume visualization
void UpeDebug_Trigger(UpeDebugDrawSystem *debug, uint32_t trigger_id, void *trigger_system);

// Performance visualization (time per subsystem as bars)
void UpeDebug_PerformanceBars(UpeDebugDrawSystem *debug, const char **names, const float *times_ms,
                              uint32_t count, UpeVec3 screen_position);

// Export primitives (for external renderer)
const UpeDebugDrawItem* UpeDebug_GetItems(const UpeDebugDrawSystem *debug, uint32_t *out_count);

// Statistics
void UpeDebug_PrintStats(const UpeDebugDrawSystem *debug);

#ifdef __cplusplus
}
#endif
