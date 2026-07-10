#pragma once

#include <stdint.h>
#include "upe/upe_config.h"
#include "upe/upe_descriptor.h"
#include "upe/upe_frame_allocator.h"
#include "upe/upe_math.h"
#include "upe/upe_scalar.h"
#include "upe/upe_math_scalar.h"
#include "upe/upe_geometry_scalar.h"
#include "upe/upe_contact_scalar.h"
#include "upe/upe_narrowphase_scalar.h"
#include "upe/upe_broadphase_scalar.h"
#include "upe/upe_collision_pipeline_scalar.h"
#include "upe/upe_manifold_cache.h"
#include "upe/upe_warm_start.h"
#include "upe/upe_ccd_scalar.h"
#include "upe/upe_queries.h"
#include "upe/upe_softbody.h"
#include "upe/upe_rope.h"
#include "upe/upe_compound_shapes.h"
#include "upe/upe_grapple.h"
#include "upe/upe_emitter.h"
#include "upe/upe_particles.h"
#include "upe/upe_gravity_fields.h"
#include "upe/upe_orbital_mechanics.h"
#include "upe/upe_relativistic_physics.h"
#include "upe/upe_string_theory.h"
#include "upe/upe_science_catalog.h"
#include "upe/upe_science_topics.h"
#include "upe/upe_science_export.h"
#include "upe/upe_particle_collision.h"
#include "upe/upe_world_body_api.h"
#include "upe/upe_step_graph.h"
#include "upe/upe_step_graph_dump.h"
#include "upe/upe_world_metrics.h"
#include "upe/upe_fluid_sph.h"
#include "upe/upe_fluid_coupling.h"
#include "upe/upe_advanced_vehicle.h"
#include "upe/upe_2d_physics.h"
#include "upe/upe_animation.h"
#include "upe/upe_ik_solver.h"
#include "upe/upe_animation_state_machine.h"
#include "upe/upe_character_controller.h"
#include "upe/upe_advanced_movement.h"
#include "upe/upe_procedural_animation.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeWorld UpeWorld;
struct UpeReplayRecorder;

// Deterministic replay/state hashing is handled by tools helpers.

UpeWorld *UpeWorld_Create(const UpeWorldDesc *desc);
void UpeWorld_Destroy(UpeWorld *world);
void UpeWorld_Step(UpeWorld *world);
float UpeWorld_Time(const UpeWorld *world);

/* Adaptive runtime tuning */
void UpeWorld_SetAdaptiveRuntimeConfig(UpeWorld *world, const UpeAdaptiveRuntimeConfig *config);
void UpeWorld_GetAdaptiveRuntimeConfig(const UpeWorld *world, UpeAdaptiveRuntimeConfig *out_config);

/* Scientific runtime controls */
void UpeWorld_SetScientificRuntimeConfig(UpeWorld *world, const UpeScientificRuntimeConfig *config);
void UpeWorld_GetScientificRuntimeConfig(const UpeWorld *world, UpeScientificRuntimeConfig *out_config);

// Determinism/replay hooks
void UpeWorld_AttachReplayRecorder(UpeWorld *world, struct UpeReplayRecorder *replay, int enable_apply);
struct UpeReplayRecorder *UpeWorld_GetReplayRecorder(const UpeWorld *world);
uint64_t UpeWorld_GetFrameIndex(const UpeWorld *world);
uint64_t UpeWorld_GetLastStateHash(const UpeWorld *world);

// Raycast and queries
int UpeWorld_Raycast(const UpeWorld *world, UpeVec3 origin, UpeVec3 direction, float max_distance,
                     const UpeQueryFilter *filter, UpeRaycastResult *result);
int UpeWorld_RaycastAll(const UpeWorld *world, UpeVec3 origin, UpeVec3 direction, float max_distance,
                        const UpeQueryFilter *filter, UpeRaycastResult *results, uint32_t max_results);

#ifdef __cplusplus
}
#endif
