#include "upe_world.h"
#include "upe_world_sort.h"
#include "upe/upe_body.h"
#include "upe/upe_integrator.h"
#include "upe/upe_shape.h"
#include "upe/upe_contact.h"
#include "upe/upe_broadphase.h"
#include "upe/upe_bvh.h"
#include "upe/upe_narrowphase.h"
#include "upe/upe_solver.h"
#include "upe/upe_joint.h"
#include "upe/upe_islands.h"
#include "upe/upe_particles.h"
#include "upe/upe_memory_policy.h"
#include "upe/upe_ccd.h"
#include "upe/upe_profiler.h"
#include "upe/upe_convex.h"
#include "upe/upe_trimesh.h"
#include "upe/upe_determinism.h"
#include "upe/upe_threading.h"
#include "upe/upe_telemetry.h"
#include "upe/upe_world_ex.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

extern "C" void UpeWorldEx_OnWorldDestroyed(const struct UpeWorld *world);

static uint32_t g_world_owned_thread_pool_refs = 0;

typedef struct UpeIslandSolveTask {
    UpeBodyState *solver_bodies;
    uint32_t body_capacity;
    UpeSolverConstraint *constraints;
    uint32_t constraint_offset;
    uint32_t constraint_count;
    uint32_t allow_parallel;
} UpeIslandSolveTask;

typedef struct UpeIslandSolveBatchTask {
    UpeIslandSolveTask *island_tasks;
    uint32_t *island_task_indices;
    uint32_t start;
    uint32_t count;
} UpeIslandSolveBatchTask;

static void UpeWorld_SolveIslandConstraintsTask(void *task_data) {
    UpeIslandSolveTask *task = (UpeIslandSolveTask*)task_data;
    if (!task || !task->solver_bodies || !task->constraints || task->constraint_count == 0) {
        return;
    }
    UpeSolver_PgsIteration(task->solver_bodies,
                           task->body_capacity,
                           task->constraints + task->constraint_offset,
                           task->constraint_count);
}

static void UpeWorld_SolveIslandBatchTask(void *task_data) {
    UpeIslandSolveBatchTask *batch = (UpeIslandSolveBatchTask*)task_data;
    if (!batch || !batch->island_tasks || !batch->island_task_indices || batch->count == 0) {
        return;
    }

    uint32_t end = batch->start + batch->count;
    for (uint32_t i = batch->start; i < end; ++i) {
        uint32_t task_index = batch->island_task_indices[i];
        UpeWorld_SolveIslandConstraintsTask(&batch->island_tasks[task_index]);
    }
}

static float UpeWorld_GravityFalloff(float inv_dist, float dist_sq, float exponent) {
    if (exponent <= 1.01f) {
        return inv_dist;
    }
    if (exponent <= 2.01f) {
        return 1.0f / dist_sq;
    }
    return powf(inv_dist, exponent);
}

static void UpeWorld_ApplyGravityFields(UpeWorld *world, const uint32_t *dynamic_ids, uint32_t dynamic_count) {
    if (!world || !dynamic_ids || dynamic_count == 0 || world->gravity_field_count == 0) {
        return;
    }

    for (uint32_t di = 0; di < dynamic_count; ++di) {
        uint32_t body_index = dynamic_ids[di];
        if (body_index >= world->body_capacity || !world->body_alive || world->body_alive[body_index] == 0) {
            continue;
        }

        UpeBody *body = &world->bodies[body_index];
        if (body->type != UPE_BODY_DYNAMIC || body->mass <= 0.0f) {
            continue;
        }

        UpeVec3 total_force = {0.0f, 0.0f, 0.0f};
        for (uint32_t fi = 0; fi < world->gravity_field_count; ++fi) {
            const UpeGravityFieldState *field = &world->gravity_fields[fi];
            if (!field->enabled) {
                continue;
            }

            UpeVec3 delta = UpeVec3_Sub(field->position, body->position);
            float dist_sq = UpeVec3_Dot(delta, delta);

            float min_dist = (field->min_distance > 0.001f) ? field->min_distance : 0.001f;
            float min_dist_sq = min_dist * min_dist;
            if (dist_sq < min_dist_sq) {
                dist_sq = min_dist_sq;
            }

            if (field->max_distance > min_dist) {
                float max_dist_sq = field->max_distance * field->max_distance;
                if (dist_sq > max_dist_sq) {
                    continue;
                }
            }

            float inv_dist = 1.0f / sqrtf(dist_sq);
            UpeVec3 dir = UpeVec3_Scale(delta, inv_dist);
            if (field->type == UPE_GRAVITY_FIELD_POINT_REPEL) {
                dir = UpeVec3_Scale(dir, -1.0f);
            }

            float exponent = (field->falloff_exponent > 0.0f) ? field->falloff_exponent : 2.0f;
            float attenuation = UpeWorld_GravityFalloff(inv_dist, dist_sq, exponent);
            float accel_mag = field->strength * attenuation;

            UpeVec3 force = UpeVec3_Scale(dir, body->mass * accel_mag);
            total_force = UpeVec3_Add(total_force, force);
        }

        UpeBody_ApplyForce(body, total_force);
    }
}

static int UpeWorld_EnsureScratch(void **buffer, uint32_t *capacity, uint32_t required, size_t elem_size) {
    if (!buffer || !capacity || elem_size == 0) {
        return 0;
    }
    if (required == 0) {
        required = 1;
    }
    if (*capacity >= required && *buffer) {
        return 1;
    }

    uint64_t bytes64 = (uint64_t)required * (uint64_t)elem_size;
    if (bytes64 > 0xFFFFFFFFull) {
        return 0;
    }

    void *new_buffer = realloc(*buffer, (size_t)bytes64);
    if (!new_buffer) {
        return 0;
    }

    *buffer = new_buffer;
    *capacity = required;
    return 1;
}

static void UpeWorld_CompactDenseBodyLists(UpeWorld *world) {
    if (!world || !world->body_alive || !world->bodies) {
        return;
    }

    if (world->alive_body_ids && world->alive_body_count > 0) {
        uint32_t write_alive = 0;
        for (uint32_t i = 0; i < world->alive_body_count; ++i) {
            uint32_t id = world->alive_body_ids[i];
            if (id < world->body_capacity && world->body_alive[id] != 0) {
                world->alive_body_ids[write_alive++] = id;
            }
        }
        world->alive_body_count = write_alive;
    }

    if (world->dynamic_body_ids && world->dynamic_body_count > 0) {
        uint32_t write_dynamic = 0;
        for (uint32_t i = 0; i < world->dynamic_body_count; ++i) {
            uint32_t id = world->dynamic_body_ids[i];
            if (id < world->body_capacity &&
                world->body_alive[id] != 0 &&
                world->bodies[id].type == UPE_BODY_DYNAMIC) {
                world->dynamic_body_ids[write_dynamic++] = id;
            }
        }
        world->dynamic_body_count = write_dynamic;
    }

    if (world->awake_dynamic_ids && world->awake_dynamic_count > 0) {
        uint32_t write_awake = 0;
        for (uint32_t i = 0; i < world->awake_dynamic_count; ++i) {
            uint32_t id = world->awake_dynamic_ids[i];
            if (id < world->body_capacity &&
                world->body_alive[id] != 0 &&
                world->bodies[id].type == UPE_BODY_DYNAMIC) {
                world->awake_dynamic_ids[write_awake++] = id;
            }
        }
        world->awake_dynamic_count = write_awake;
    }
}

static float UpeWorld_BitCastU32ToF32(uint32_t value) {
    float out = 0.0f;
    memcpy(&out, &value, sizeof(uint32_t));
    return out;
}

static uint64_t UpeWorld_OrderedBodyPairKey(uint32_t a, uint32_t b) {
    uint32_t lo = (a < b) ? a : b;
    uint32_t hi = (a < b) ? b : a;
    return ((uint64_t)lo << 32) | (uint64_t)hi;
}

static uint64_t UpeWorld_HashPairKey(uint64_t key) {
    key ^= key >> 33;
    key *= 0xff51afd7ed558ccdULL;
    key ^= key >> 33;
    key *= 0xc4ceb9fe1a85ec53ULL;
    key ^= key >> 33;
    return key;
}

static uint64_t UpeWorld_StageMicros(const UpeWorld *world, UpeStepStage stage) {
    if (!world || stage >= UPE_STEP_STAGE_COUNT) {
        return 0;
    }
    return (uint64_t)world->last_step_stats[stage].duration_us;
}

static uint64_t UpeWorld_TotalStepMicros(const UpeWorld *world) {
    if (!world) {
        return 0;
    }
    uint64_t total_us = 0;
    for (uint32_t i = 0; i < UPE_STEP_STAGE_COUNT; ++i) {
        total_us += (uint64_t)world->last_step_stats[i].duration_us;
    }
    return total_us;
}

static void UpeWorld_RecordTelemetry(UpeWorld *world) {
    if (!world) {
        return;
    }

    UpeTelemetryBuffer *telemetry = UpeWorld_GetTelemetryBuffer(world);
    if (!telemetry) {
        return;
    }

    uint64_t frame = world->frame_index;
    uint64_t awake = world->awake_dynamic_count;
    uint64_t dynamic_total = world->dynamic_body_count;
    uint64_t sleeping = (dynamic_total > awake) ? (dynamic_total - awake) : 0;

    UpeTelemetry_Record(telemetry, frame, UPE_TELEMETRY_BROADPHASE_PAIRS, world->last_broadphase_pair_count);
    UpeTelemetry_Record(telemetry, frame, UPE_TELEMETRY_NARROWPHASE_CONTACTS, world->last_contact_count);
    UpeTelemetry_Record(telemetry, frame, UPE_TELEMETRY_SOLVER_ITERATIONS, world->last_solver_iterations);
    UpeTelemetry_Record(telemetry, frame, UPE_TELEMETRY_SOLVER_CONSTRAINTS, world->last_constraint_count);
    UpeTelemetry_Record(telemetry, frame, UPE_TELEMETRY_BODIES_AWAKE, awake);
    UpeTelemetry_Record(telemetry, frame, UPE_TELEMETRY_BODIES_SLEEPING, sleeping);
    UpeTelemetry_Record(telemetry, frame, UPE_TELEMETRY_FRAME_TIME_US, (uint64_t)(world->fixed_dt * 1000000.0f));
    UpeTelemetry_Record(telemetry, frame, UPE_TELEMETRY_STEP_TIME_US, UpeWorld_TotalStepMicros(world));
    UpeTelemetry_Record(telemetry, frame, UPE_TELEMETRY_BROADPHASE_TIME_US, UpeWorld_StageMicros(world, UPE_STEP_STAGE_BROADPHASE));
    UpeTelemetry_Record(telemetry, frame, UPE_TELEMETRY_NARROWPHASE_TIME_US, UpeWorld_StageMicros(world, UPE_STEP_STAGE_NARROWPHASE));
    UpeTelemetry_Record(telemetry, frame, UPE_TELEMETRY_SOLVE_TIME_US, UpeWorld_StageMicros(world, UPE_STEP_STAGE_SOLVER));
    UpeTelemetry_Record(telemetry, frame, UPE_TELEMETRY_INTEGRATE_TIME_US, UpeWorld_StageMicros(world, UPE_STEP_STAGE_INTEGRATION));
    UpeTelemetry_Record(telemetry, frame, UPE_TELEMETRY_ISLANDS_FOUND, world->last_island_count);
    UpeTelemetry_Record(telemetry, frame, UPE_TELEMETRY_CCD_TESTS, world->ccd_tested_pairs);
}

static int UpeWorld_ComputeBodyAabbAndPrimaryShape(const UpeWorld *world,
                                                   uint32_t body_index,
                                                   UpeAabb3 *out_aabb,
                                                   uint32_t *out_primary_shape_index) {
    if (!world || !out_aabb || body_index >= world->body_capacity || !world->bodies) {
        return 0;
    }

    const UpeBody *body = &world->bodies[body_index];
    UpeAabb3 body_aabb = {{1e10f, 1e10f, 1e10f}, {-1e10f, -1e10f, -1e10f}};
    uint32_t primary_shape = 0xFFFFFFFFu;
    int has_shape = 0;

    uint32_t shape_idx = body->first_shape_index;
    while (shape_idx != 0xFFFFFFFFu) {
        if (shape_idx >= world->shape_capacity || !world->shape_alive || world->shape_alive[shape_idx] == 0) {
            break;
        }

        if (primary_shape == 0xFFFFFFFFu) {
            primary_shape = shape_idx;
        }

        const UpeShape *shape = &world->shapes[shape_idx];
        UpeAabb3 shape_aabb = {};

        if (shape->type == UPE_SHAPE_SPHERE) {
            UpeSphere sphere = shape->sphere;
            sphere.center = UpeVec3_Add(sphere.center, body->position);
            float r = sphere.radius;
            shape_aabb = {
                {sphere.center.x - r, sphere.center.y - r, sphere.center.z - r},
                {sphere.center.x + r, sphere.center.y + r, sphere.center.z + r}
            };
            has_shape = 1;
        } else if (shape->type == UPE_SHAPE_CAPSULE) {
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
            shape_aabb = {{min_x, min_y, min_z}, {max_x, max_y, max_z}};
            has_shape = 1;
        } else if (shape->type == UPE_SHAPE_BOX) {
            UpeAabb3 box = shape->box;
            shape_aabb.min = UpeVec3_Add(box.min, body->position);
            shape_aabb.max = UpeVec3_Add(box.max, body->position);
            has_shape = 1;
        } else if (shape->type == UPE_SHAPE_OBB) {
            UpeOBB obb = shape->obb;
            UpeVec3 c = UpeVec3_Add(obb.center, body->position);
            UpeVec3 e0 = UpeVec3_Scale(obb.axes[0], obb.half_extents.x);
            UpeVec3 e1 = UpeVec3_Scale(obb.axes[1], obb.half_extents.y);
            UpeVec3 e2 = UpeVec3_Scale(obb.axes[2], obb.half_extents.z);
            UpeVec3 ext = {
                fabsf(e0.x) + fabsf(e1.x) + fabsf(e2.x),
                fabsf(e0.y) + fabsf(e1.y) + fabsf(e2.y),
                fabsf(e0.z) + fabsf(e1.z) + fabsf(e2.z)
            };
            shape_aabb.min = UpeVec3_Sub(c, ext);
            shape_aabb.max = UpeVec3_Add(c, ext);
            has_shape = 1;
        }

        if (has_shape) {
            body_aabb = UpeAabb3_Union(body_aabb, shape_aabb);
        }

        shape_idx = shape->next_shape_index;
    }

    if (!has_shape) {
        body_aabb.min = body->position;
        body_aabb.max = body->position;
    }

    *out_aabb = body_aabb;
    if (out_primary_shape_index) {
        *out_primary_shape_index = primary_shape;
    }
    return has_shape;
}

static void UpeWorld_DefaultMemoryBudget(UpeMemoryBudget *out_budget) {
    if (!out_budget) {
        return;
    }
    out_budget->global_max = 512ull * 1024ull * 1024ull;
    out_budget->broadphase_pairs_max = 32ull * 1024ull * sizeof(UpeBroadphasePair);
    out_budget->contacts_max = 64ull * 1024ull * sizeof(UpeContactPair);
    out_budget->constraints_max = 32ull * 1024ull * sizeof(UpeSolverConstraint);
    out_budget->particles_max = 64ull * 1024ull * sizeof(UpeParticle);
    out_budget->terrain_chunks_max = 64ull * 1024ull * 1024ull;
    out_budget->meshes_max = 128ull * 1024ull * 1024ull;
    out_budget->frame_scratch_max = 64ull * 1024ull * 1024ull;
    out_budget->debug_telemetry_max = 16ull * 1024ull * 1024ull;
}

static uint64_t UpeWorld_NowUs(void) {
    clock_t ticks = clock();
    if (ticks < 0) {
        return 0;
    }
    return ((uint64_t)ticks * 1000000ull) / (uint64_t)CLOCKS_PER_SEC;
}

static void UpeWorld_BeginStage(UpeWorld *world, UpeStepStage stage, uint32_t item_count_in) {
    if (!world) {
        return;
    }
    uint32_t idx = (uint32_t)stage;
    if (idx >= UPE_STEP_STAGE_COUNT) {
        return;
    }
    UpeStepStageStat *stat = &world->last_step_stats[idx];
    stat->frame_index = world->frame_index;
    stat->stage = idx;
    stat->item_count_in = item_count_in;
    stat->item_count_out = 0;
    stat->begin_us = UpeWorld_NowUs();
    stat->end_us = stat->begin_us;
    stat->duration_us = 0;
}

static void UpeWorld_EndStage(UpeWorld *world, UpeStepStage stage, uint32_t item_count_out) {
    if (!world) {
        return;
    }
    uint32_t idx = (uint32_t)stage;
    if (idx >= UPE_STEP_STAGE_COUNT) {
        return;
    }
    UpeStepStageStat *stat = &world->last_step_stats[idx];
    stat->item_count_out = item_count_out;
    stat->end_us = UpeWorld_NowUs();
    stat->duration_us = (stat->end_us >= stat->begin_us) ? (stat->end_us - stat->begin_us) : 0;
    if (world->step_stage_callback) {
        world->step_stage_callback(stat, world->step_stage_user_data);
    }
}

UpeWorld *UpeWorld_Create(const UpeWorldDesc *desc) {
    if (!desc) {
        return nullptr;
    }
    static int s_memory_initialized = 0;
    if (!s_memory_initialized) {
        UpeMemoryBudget budget = {};
        UpeWorld_DefaultMemoryBudget(&budget);
        UpeMemory_Initialize(&budget, 64);
        s_memory_initialized = 1;
    }
    UpeWorld *world = new UpeWorld();
    world->time = 0.0f;
    world->fixed_dt = (desc->fixed_dt > 0.0f) ? desc->fixed_dt : (1.0f / 60.0f);
    world->gravity = {desc->gravity[0], desc->gravity[1], desc->gravity[2]};
    world->frame_index = 0;
    world->last_state_hash = 0;
    world->replay = nullptr;
    world->replay_apply_enabled = 0;
    world->solver_iterations = (desc->solver_iterations > 0) ? desc->solver_iterations : 10;
    world->solver_friction_iterations = (desc->solver_friction_iterations > 0) ? desc->solver_friction_iterations : 1;
    world->solver_thread_count = 1;
    world->max_pairs = (desc->max_pairs > 0) ? desc->max_pairs : 16384;
    world->max_contacts = (desc->max_contacts > 0) ? desc->max_contacts : 4096;
    world->last_solver_parallel_islands = 0;
    world->last_solver_serial_islands = 0;
    world->enable_warm_starting = desc->enable_warm_starting ? 1 : 0;
    world->enable_sleeping = desc->enable_sleeping ? 1 : 0;
    world->ccd_enabled = (desc->enable_ccd || desc->ccd_enabled) ? 1 : 0;
    world->enable_parallel_island_solver = 0;
    world->owns_thread_pool = 0;
    {
        float ccd_threshold = (desc->ccd_threshold > 0.0f) ? desc->ccd_threshold : 10.0f;
        world->ccd_threshold_sq = ccd_threshold * ccd_threshold;
    }
    world->ccd_candidate_pairs = 0;
    world->ccd_tested_pairs = 0;
    world->ccd_contact_hits = 0;
    world->last_active_body_count = 0;
    world->last_broadphase_pair_count = 0;
    world->last_contact_count = 0;
    world->last_constraint_count = 0;
    world->last_island_count = 0;
    world->last_solver_iterations = world->solver_iterations;
    world->last_solver_island_task_count = 0;
    world->awake_dynamic_count = 0;
    world->step_stage_callback = nullptr;
    world->step_stage_user_data = nullptr;
    world->last_step_stats_count = UPE_STEP_STAGE_COUNT;
    for (uint32_t i = 0; i < UPE_STEP_STAGE_COUNT; ++i) {
        world->last_step_stats[i] = {};
        world->last_step_stats[i].stage = i;
    }
    world->memory = nullptr;
    {
        UpeMemoryBudget budget = {};
        UpeWorld_DefaultMemoryBudget(&budget);
        world->memory = UpeMemoryContext_Create(&budget, 64);
    }
    world->body_count = 0;
    world->body_capacity = 0;
    world->bodies = nullptr;
    world->body_generations = nullptr;
    world->body_alive = nullptr;
    world->alive_body_ids = nullptr;
    world->alive_body_count = 0;
    world->alive_body_capacity = 0;
    world->dynamic_body_ids = nullptr;
    world->dynamic_body_count = 0;
    world->dynamic_body_capacity = 0;
    world->awake_dynamic_ids = nullptr;
    world->awake_dynamic_count = 0;
    world->awake_dynamic_capacity = 0;
    world->gravity_fields = nullptr;
    world->gravity_field_count = 0;
    world->gravity_field_capacity = 0;
    world->gravity_field_next_id = 1;

    world->shapes = nullptr;
    world->shape_count = 0;
    world->shape_capacity = 0;
    world->shape_generations = nullptr;
    world->shape_alive = nullptr;

    world->contacts = nullptr;
    world->contact_count = 0;
    world->contact_capacity = 0; // Allocated per-frame from arena
    world->scratch_bounds = nullptr;
    world->scratch_bounds_capacity = 0;
    world->scratch_body_indices = nullptr;
    world->scratch_body_indices_capacity = 0;
    world->scratch_pairs = nullptr;
    world->scratch_pairs_capacity = 0;
    world->scratch_contacts = nullptr;
    world->scratch_contacts_capacity = 0;
    world->scratch_ccd_bounds = nullptr;
    world->scratch_ccd_capacity = 0;
    world->scratch_ccd_body_ids = nullptr;
    world->scratch_ccd_primary_shape_ids = nullptr;
    world->scratch_ccd_fast_flags = nullptr;
    world->scratch_solver_bodies = nullptr;
    world->scratch_solver_bodies_capacity = 0;
    world->scratch_island_tasks = nullptr;
    world->scratch_island_tasks_capacity = 0;
    world->scratch_constraints = nullptr;
    world->scratch_constraints_capacity = 0;
    world->scratch_constraint_island_ids = nullptr;
    world->scratch_constraint_island_ids_capacity = 0;
    world->scratch_constraint_cursor = nullptr;
    world->scratch_constraint_cursor_capacity = 0;
    world->scratch_reordered_constraints = nullptr;
    world->scratch_reordered_constraints_capacity = 0;
    world->scratch_bucket_heads = nullptr;
    world->scratch_bucket_heads_capacity = 0;
    world->scratch_bucket_next = nullptr;
    world->scratch_bucket_next_capacity = 0;
    world->scratch_root_to_island = nullptr;
    world->scratch_root_to_island_capacity = 0;
    world->scratch_thread_work_items = nullptr;
    world->scratch_thread_work_items_capacity = 0;
    world->scratch_parallel_task_indices = nullptr;
    world->scratch_parallel_task_indices_capacity = 0;
    world->scratch_island_batch_tasks = nullptr;
    world->scratch_island_batch_tasks_capacity = 0;

    world->prev_constraints = nullptr;
    world->prev_constraint_count = 0;
    world->prev_constraint_capacity = 256 * 4;
    world->prev_constraints = (UpeSolverConstraint*)malloc(
        world->prev_constraint_capacity * sizeof(UpeSolverConstraint));
    
    world->joints = nullptr;
    world->joint_count = 0;
    world->joint_capacity = 0;
    
    world->islands = nullptr;
    world->island_count = 0;
    world->island_capacity = 0;
    world->body_island_ids = nullptr;
    world->island_constraint_offsets = nullptr;
    world->island_constraint_counts = nullptr;

    world->total_kinetic_energy = 0.0;
    world->total_potential_energy = 0.0;
    world->energy_dissipated_this_frame = 0.0;

    world->query_bounds = nullptr;
    world->query_bound_count = 0;
    world->query_bound_capacity = 0;
    world->query_indices = nullptr;
    world->query_index_capacity = 0;
    world->query_nodes = nullptr;
    world->query_node_count = 0;
    world->query_node_capacity = 0;
    world->query_bvh_dirty = 1;

    uint32_t requested_threads = 0;
    if (desc->thread_pool_size > 1.0f) {
        requested_threads = (uint32_t)(desc->thread_pool_size + 0.5f);
    }
    if (requested_threads > 1) {
        uint32_t existing_threads = UpeThreading_GetThreadCount();
        if (existing_threads == 0) {
            UpeThreading_Init(requested_threads);
            existing_threads = UpeThreading_GetThreadCount();
            if (existing_threads > 0) {
                world->owns_thread_pool = 1;
                g_world_owned_thread_pool_refs += 1;
            }
        }
        if (existing_threads > 0) {
            world->solver_thread_count = existing_threads;
            world->enable_parallel_island_solver = (existing_threads > 1) ? 1 : 0;
        }
    }

    world->profiler = nullptr;

    return world;
}

void UpeWorld_Destroy(UpeWorld *world) {
    if (!world) {
        return;
    }
    UpeWorldEx_OnWorldDestroyed(world);
    free(world->bodies);
    free(world->body_generations);
    free(world->body_alive);
    free(world->alive_body_ids);
    free(world->dynamic_body_ids);
    free(world->awake_dynamic_ids);
    free(world->gravity_fields);
    free(world->shapes);
    free(world->shape_generations);
    free(world->shape_alive);
    free(world->scratch_bounds);
    free(world->scratch_body_indices);
    free(world->scratch_pairs);
    free(world->scratch_contacts);
    free(world->scratch_ccd_bounds);
    free(world->scratch_ccd_body_ids);
    free(world->scratch_ccd_primary_shape_ids);
    free(world->scratch_ccd_fast_flags);
    free(world->scratch_solver_bodies);
    free(world->scratch_island_tasks);
    free(world->scratch_constraints);
    free(world->scratch_constraint_island_ids);
    free(world->scratch_constraint_cursor);
    free(world->scratch_reordered_constraints);
    free(world->scratch_bucket_heads);
    free(world->scratch_bucket_next);
    free(world->scratch_root_to_island);
    free(world->scratch_thread_work_items);
    free(world->scratch_parallel_task_indices);
    free(world->scratch_island_batch_tasks);
    // contacts allocated from frame arena, no free needed
    free(world->prev_constraints);
    free(world->joints);
    free(world->query_bounds);
    free(world->query_indices);
    free(world->query_nodes);
    if (world->islands) {
        for (uint32_t i = 0; i < world->island_count; ++i) {
            UpeIsland_Destroy(&world->islands[i]);
        }
        free(world->islands);
    }
    free(world->body_island_ids);
    free(world->island_constraint_offsets);
    free(world->island_constraint_counts);
    if (world->owns_thread_pool && g_world_owned_thread_pool_refs > 0) {
        g_world_owned_thread_pool_refs -= 1;
        if (g_world_owned_thread_pool_refs == 0) {
            UpeThreading_Shutdown();
        }
    }
    if (world->memory) {
        UpeMemoryContext_Destroy(world->memory);
        world->memory = nullptr;
    }
    delete world;
}

void UpeWorld_Step(UpeWorld *world) {
    if (!world) {
        return;
    }

    world->last_step_stats_count = UPE_STEP_STAGE_COUNT;
    UpeWorld_BeginStage(world, UPE_STEP_STAGE_PRE_STEP, world->body_count);

    if (world->replay && world->replay_apply_enabled) {
        UpeReplay_ApplyToWorld(world, world->replay, world->frame_index);
    }

    const int using_frame_arena = (world->memory && world->memory->frame_arena) ? 1 : 0;

    if (world->profiler) {
        UpeProfiler_BeginFrame(world->profiler);
    }
    if (world->memory) {
        UpeMemoryContext_PrepareFrame(world->memory);
    }

    // Clear previous frame contacts
    world->contact_count = 0;
    world->ccd_candidate_pairs = 0;
    world->ccd_tested_pairs = 0;
    world->ccd_contact_hits = 0;
    world->last_active_body_count = 0;
    world->last_broadphase_pair_count = 0;
    world->last_contact_count = 0;
    world->last_constraint_count = 0;
    world->last_island_count = 0;
    world->last_solver_iterations = (world->solver_iterations > 0) ? world->solver_iterations : 1;
    world->last_solver_island_task_count = 0;

    UpeWorld_CompactDenseBodyLists(world);

    // 1. Collect body AABBs for broadphase
    UpeAabb3 *bounds = nullptr;
    uint32_t *body_indices = nullptr;
    uint32_t active_body_count = 0;

    // Count active dynamic bodies
    active_body_count = world->dynamic_body_count;
    world->last_active_body_count = active_body_count;

    UpeWorld_ApplyGravityFields(world, world->dynamic_body_ids, active_body_count);

    if (active_body_count < 2) {
        UpeWorld_EndStage(world, UPE_STEP_STAGE_PRE_STEP, active_body_count);
        UpeWorld_BeginStage(world, UPE_STEP_STAGE_INTEGRATION, active_body_count);
        // No collisions possible, just integrate
        if (world->profiler) {
            UpeProfiler_BeginZone(world->profiler, "integration");
        }
        for (uint32_t dyn = 0; dyn < active_body_count; ++dyn) {
            uint32_t i = world->dynamic_body_ids[dyn];
            UpeIntegrator_SemiImplicit(&world->bodies[i], world->fixed_dt, world->gravity);
            UpeBody_ClearForces(&world->bodies[i]);
        }
        if (world->profiler) {
            UpeProfiler_EndZone(world->profiler);
        }
        UpeWorld_EndStage(world, UPE_STEP_STAGE_INTEGRATION, active_body_count);
        UpeWorld_BeginStage(world, UPE_STEP_STAGE_POST_STEP, active_body_count);
        if (world->memory) {
            UpeMemoryContext_FinalizeFrame(world->memory, world->fixed_dt * 1000.0f);
        }
        world->time += world->fixed_dt;
        world->frame_index += 1u;
        {
            UpeStateHash state_hash = UpeDeterminism_HashState(world);
            world->last_state_hash = state_hash.state;
            if (world->replay && world->replay->recording) {
                UpeEvent evt = {};
                evt.frame = world->frame_index;
                evt.type = UPE_EVENT_CUSTOM;
                evt.entity_id = 0xFFFFFFFFu;
                evt.data[0] = UpeWorld_BitCastU32ToF32((uint32_t)(world->last_state_hash & 0xFFFFFFFFu));
                evt.data[1] = UpeWorld_BitCastU32ToF32((uint32_t)(world->last_state_hash >> 32));
                evt.data[2] = UpeWorld_BitCastU32ToF32((uint32_t)(world->frame_index & 0xFFFFFFFFu));
                evt.data[3] = UpeWorld_BitCastU32ToF32((uint32_t)(world->frame_index >> 32));
                UpeReplay_RecordEvent(world->replay, &evt);
            }
        }
        world->query_bvh_dirty = 1;
        UpeWorld_RecordTelemetry(world);
        UpeWorld_EndStage(world, UPE_STEP_STAGE_POST_STEP, active_body_count);
        if (world->profiler) {
            UpeProfiler_EndFrame(world->profiler);
        }
        return;
    }

    if (world->memory && world->memory->frame_arena) {
        bounds = (UpeAabb3*)UpeFrameArena_Alloc(world->memory->frame_arena,
                                               active_body_count * sizeof(UpeAabb3),
                                               16);
        body_indices = (uint32_t*)UpeFrameArena_Alloc(world->memory->frame_arena,
                                                     active_body_count * sizeof(uint32_t),
                                                     16);
    } else {
        if (!UpeWorld_EnsureScratch((void**)&world->scratch_bounds,
                                    &world->scratch_bounds_capacity,
                                    active_body_count,
                                    sizeof(UpeAabb3)) ||
            !UpeWorld_EnsureScratch((void**)&world->scratch_body_indices,
                                    &world->scratch_body_indices_capacity,
                                    active_body_count,
                                    sizeof(uint32_t))) {
            if (world->memory) {
                UpeMemoryContext_FinalizeFrame(world->memory, world->fixed_dt * 1000.0f);
            }
            world->query_bvh_dirty = 1;
            if (world->profiler) {
                UpeProfiler_EndFrame(world->profiler);
            }
            return;
        }
        bounds = world->scratch_bounds;
        body_indices = world->scratch_body_indices;
    }

    uint32_t idx = 0;
    for (uint32_t dyn = 0; dyn < active_body_count; ++dyn) {
        uint32_t i = world->dynamic_body_ids[dyn];
        UpeBody *body = &world->bodies[i];
        body_indices[idx] = i;

        // Find shapes attached to this body and compute AABB (via linked list)
        UpeAabb3 body_aabb = {{1e10f, 1e10f, 1e10f}, {-1e10f, -1e10f, -1e10f}};
        int has_shape = 0;
        uint32_t shape_idx = body->first_shape_index;
        while (shape_idx != 0xFFFFFFFF) {
            if (shape_idx >= world->shape_capacity || !world->shape_alive || world->shape_alive[shape_idx] == 0) {
                break;
            }
            has_shape = 1;
            UpeShape *shape = &world->shapes[shape_idx];
            UpeAabb3 shape_aabb = {};
            
            if (shape->type == UPE_SHAPE_SPHERE) {
                UpeSphere sphere = shape->sphere;
                // Transform sphere center by body position
                sphere.center = UpeVec3_Add(sphere.center, body->position);
                float r = sphere.radius;
                shape_aabb = {
                    {sphere.center.x - r, sphere.center.y - r, sphere.center.z - r},
                    {sphere.center.x + r, sphere.center.y + r, sphere.center.z + r}
                };
            }
            else if (shape->type == UPE_SHAPE_CAPSULE) {
                UpeCapsule capsule = shape->capsule;
                capsule.a = UpeVec3_Add(capsule.a, body->position);
                capsule.b = UpeVec3_Add(capsule.b, body->position);
                float r = capsule.radius;
                // AABB that encompasses both endpoints plus radius
                float min_x = (capsule.a.x < capsule.b.x ? capsule.a.x : capsule.b.x) - r;
                float min_y = (capsule.a.y < capsule.b.y ? capsule.a.y : capsule.b.y) - r;
                float min_z = (capsule.a.z < capsule.b.z ? capsule.a.z : capsule.b.z) - r;
                float max_x = (capsule.a.x > capsule.b.x ? capsule.a.x : capsule.b.x) + r;
                float max_y = (capsule.a.y > capsule.b.y ? capsule.a.y : capsule.b.y) + r;
                float max_z = (capsule.a.z > capsule.b.z ? capsule.a.z : capsule.b.z) + r;
                shape_aabb = {{min_x, min_y, min_z}, {max_x, max_y, max_z}};
            }
            else if (shape->type == UPE_SHAPE_BOX) {
                UpeAabb3 box = shape->box;
                shape_aabb.min = UpeVec3_Add(box.min, body->position);
                shape_aabb.max = UpeVec3_Add(box.max, body->position);
            }
            
            body_aabb = UpeAabb3_Union(body_aabb, shape_aabb);
            shape_idx = shape->next_shape_index;
        }

        if (!has_shape) {
            // Fallback: point AABB at body position
            body_aabb.min = body->position;
            body_aabb.max = body->position;
        }

        bounds[idx] = body_aabb;
        idx++;
    }

    UpeWorld_EndStage(world, UPE_STEP_STAGE_PRE_STEP, active_body_count);
    UpeWorld_BeginStage(world, UPE_STEP_STAGE_BROADPHASE, active_body_count);

    // 2. Broadphase: find overlapping pairs (BVH O(N log N) when memory available, SAP fallback)
    if (world->profiler) {
        UpeProfiler_BeginZone(world->profiler, "broadphase");
    }
    UpeBroadphasePair *pairs = nullptr;
    uint32_t pair_count = 0;
    
    if (world->memory && world->memory->frame_arena) {
        /* Allocate BVH scratch space from frame arena */
        uint32_t indices_size = active_body_count * sizeof(uint32_t);
        uint32_t nodes_size = active_body_count * 2 * sizeof(UpeBvhNode);
        uint32_t query_temp_size = active_body_count * sizeof(uint32_t);
        uint32_t bvh_scratch_size = indices_size + nodes_size + query_temp_size;
        void *bvh_scratch = UpeFrameArena_Alloc(world->memory->frame_arena, bvh_scratch_size, 16);
        
        /* Allocate pair output buffer with hard cap to avoid O(N²) blowups */
        uint64_t worst_case_pairs64 = (uint64_t)active_body_count * (uint64_t)active_body_count;
        uint32_t worst_case_pairs = (worst_case_pairs64 > 0xFFFFFFFFull) ? 0xFFFFFFFFu : (uint32_t)worst_case_pairs64;
        uint32_t max_pairs = (world->max_pairs < worst_case_pairs) ? world->max_pairs : worst_case_pairs;
        pairs = (UpeBroadphasePair*)UpeFrameArena_Alloc(world->memory->frame_arena,
                                                       max_pairs * sizeof(UpeBroadphasePair),
                                                       16);
        
        if (bvh_scratch && pairs) {
            pair_count = UpeBroadphase_BvhPairs(bounds, active_body_count, pairs, max_pairs,
                                                bvh_scratch, bvh_scratch_size);
        }
    } else {
        /* Fallback to SAP for compatibility */
        uint64_t worst_case_pairs64 = (uint64_t)active_body_count * (uint64_t)active_body_count;
        uint32_t worst_case_pairs = (worst_case_pairs64 > 0xFFFFFFFFull) ? 0xFFFFFFFFu : (uint32_t)worst_case_pairs64;
        uint32_t max_pairs = (world->max_pairs < worst_case_pairs) ? world->max_pairs : worst_case_pairs;
        if (!UpeWorld_EnsureScratch(&world->scratch_pairs,
                                    &world->scratch_pairs_capacity,
                                    max_pairs,
                                    sizeof(UpeBroadphasePair))) {
            if (world->memory) {
                UpeMemoryContext_FinalizeFrame(world->memory, world->fixed_dt * 1000.0f);
            }
            world->query_bvh_dirty = 1;
            if (world->profiler) {
                UpeProfiler_EndZone(world->profiler);
                UpeProfiler_EndFrame(world->profiler);
            }
            return;
        }
        pairs = (UpeBroadphasePair*)world->scratch_pairs;
        pair_count = UpeBroadphase_SapPairs(bounds, active_body_count, pairs, max_pairs);
    }

    UpeWorldSort_BroadphasePairs(pairs, pair_count, body_indices, world->memory);
    world->last_broadphase_pair_count = pair_count;
    UpeWorld_EndStage(world, UPE_STEP_STAGE_BROADPHASE, pair_count);
    UpeWorld_BeginStage(world, UPE_STEP_STAGE_NARROWPHASE, pair_count);

    if (world->profiler) {
        UpeProfiler_EndZone(world->profiler);
        UpeProfiler_BeginZone(world->profiler, "narrowphase");
    }

    // Allocate contact storage for this frame (estimate: all pairs could generate contacts)
    uint32_t contact_budget = (world->max_contacts > 0) ? world->max_contacts : pair_count;
    world->contact_capacity = pair_count > 0 ? pair_count : 1;
    if (world->contact_capacity > contact_budget) {
        world->contact_capacity = contact_budget;
    }
    if (world->contact_capacity == 0) {
        world->contact_capacity = 1;
    }
    if (world->memory && world->memory->frame_arena) {
        world->contacts = (UpeContactPair*)UpeFrameArena_Alloc(world->memory->frame_arena,
                                                               world->contact_capacity * sizeof(UpeContactPair),
                                                               16);
    } else {
        if (!UpeWorld_EnsureScratch((void**)&world->scratch_contacts,
                                    &world->scratch_contacts_capacity,
                                    world->contact_capacity,
                                    sizeof(UpeContactPair))) {
            if (world->memory) {
                UpeMemoryContext_FinalizeFrame(world->memory, world->fixed_dt * 1000.0f);
            }
            world->query_bvh_dirty = 1;
            if (world->profiler) {
                UpeProfiler_EndZone(world->profiler);
                UpeProfiler_EndFrame(world->profiler);
            }
            return;
        }
        world->contacts = world->scratch_contacts;
    }

    // 3. Narrowphase: generate contacts for each pair
    for (uint32_t p = 0; p < pair_count; ++p) {
        uint32_t a_idx = body_indices[pairs[p].a];
        uint32_t b_idx = body_indices[pairs[p].b];
        UpeBody *body_a = &world->bodies[a_idx];
        UpeBody *body_b = &world->bodies[b_idx];

        uint32_t shape_a_idx = body_a->first_shape_index;
        while (shape_a_idx != 0xFFFFFFFF) {
            if (shape_a_idx >= world->shape_capacity || !world->shape_alive) {
                break;
            }
            if (!world->shape_alive[shape_a_idx]) {
                shape_a_idx = world->shapes[shape_a_idx].next_shape_index;
                continue;
            }
            UpeShape *shape_a = &world->shapes[shape_a_idx];

            uint32_t shape_b_idx = body_b->first_shape_index;
            while (shape_b_idx != 0xFFFFFFFF) {
                if (shape_b_idx >= world->shape_capacity || !world->shape_alive) {
                    break;
                }
                if (!world->shape_alive[shape_b_idx]) {
                    shape_b_idx = world->shapes[shape_b_idx].next_shape_index;
                    continue;
                }
                UpeShape *shape_b = &world->shapes[shape_b_idx];

        // Check collision filtering
        uint32_t layer_a = shape_a->collision_layer > 0 ? shape_a->collision_layer : body_a->collision_layer;
        uint32_t mask_a = shape_a->collision_mask > 0 ? shape_a->collision_mask : body_a->collision_mask;
        uint32_t layer_b = shape_b->collision_layer > 0 ? shape_b->collision_layer : body_b->collision_layer;
        uint32_t mask_b = shape_b->collision_mask > 0 ? shape_b->collision_mask : body_b->collision_mask;
        
        extern int UpeCollisionFilter_ShouldCollide(uint32_t body1_layer, uint32_t body1_mask, 
                                                    uint32_t body2_layer, uint32_t body2_mask);
        if (!UpeCollisionFilter_ShouldCollide(layer_a, mask_a, layer_b, mask_b)) {
            continue; // Collision disabled by filtering
        }

        // Generate contact manifold for various shape type pairs
        UpeContactManifold manifold = {};
        int has_contact = 0;
        
        if (shape_a->type == UPE_SHAPE_SPHERE && shape_b->type == UPE_SHAPE_SPHERE) {
            UpeSphere sphere_a = shape_a->sphere;
            UpeSphere sphere_b = shape_b->sphere;
            // Transform to world space
            sphere_a.center = UpeVec3_Add(sphere_a.center, body_a->position);
            sphere_b.center = UpeVec3_Add(sphere_b.center, body_b->position);
            has_contact = UpeNarrowphase_SphereSphere(sphere_a, sphere_b, &manifold);
        }
        else if (shape_a->type == UPE_SHAPE_SPHERE && shape_b->type == UPE_SHAPE_CAPSULE) {
            UpeSphere sphere = shape_a->sphere;
            UpeCapsule capsule = shape_b->capsule;
            sphere.center = UpeVec3_Add(sphere.center, body_a->position);
            capsule.a = UpeVec3_Add(capsule.a, body_b->position);
            capsule.b = UpeVec3_Add(capsule.b, body_b->position);
            has_contact = UpeNarrowphase_SphereCapsule(sphere, capsule, &manifold);
        }
        else if (shape_a->type == UPE_SHAPE_CAPSULE && shape_b->type == UPE_SHAPE_SPHERE) {
            UpeCapsule capsule = shape_a->capsule;
            UpeSphere sphere = shape_b->sphere;
            capsule.a = UpeVec3_Add(capsule.a, body_a->position);
            capsule.b = UpeVec3_Add(capsule.b, body_a->position);
            sphere.center = UpeVec3_Add(sphere.center, body_b->position);
            has_contact = UpeNarrowphase_SphereCapsule(sphere, capsule, &manifold);
            // Flip normal since we called SphereCapsule with swapped arguments
            if (has_contact && manifold.count > 0) {
                manifold.points[0].normal = UpeVec3_Scale(manifold.points[0].normal, -1.0f);
            }
        }
        else if (shape_a->type == UPE_SHAPE_CAPSULE && shape_b->type == UPE_SHAPE_CAPSULE) {
            UpeCapsule capsule_a = shape_a->capsule;
            UpeCapsule capsule_b = shape_b->capsule;
            capsule_a.a = UpeVec3_Add(capsule_a.a, body_a->position);
            capsule_a.b = UpeVec3_Add(capsule_a.b, body_a->position);
            capsule_b.a = UpeVec3_Add(capsule_b.a, body_b->position);
            capsule_b.b = UpeVec3_Add(capsule_b.b, body_b->position);
            has_contact = UpeNarrowphase_CapsuleCapsule(capsule_a, capsule_b, &manifold);
        }
        else if (shape_a->type == UPE_SHAPE_BOX && shape_b->type == UPE_SHAPE_BOX) {
            UpeAabb3 box_a = shape_a->box;
            UpeAabb3 box_b = shape_b->box;
            // Transform to world space
            box_a.min = UpeVec3_Add(box_a.min, body_a->position);
            box_a.max = UpeVec3_Add(box_a.max, body_a->position);
            box_b.min = UpeVec3_Add(box_b.min, body_b->position);
            box_b.max = UpeVec3_Add(box_b.max, body_b->position);
            has_contact = UpeNarrowphase_BoxBox(box_a, box_b, &manifold);
        }
        else if (shape_a->type == UPE_SHAPE_SPHERE && shape_b->type == UPE_SHAPE_BOX) {
            UpeSphere sphere = shape_a->sphere;
            UpeAabb3 box = shape_b->box;
            sphere.center = UpeVec3_Add(sphere.center, body_a->position);
            box.min = UpeVec3_Add(box.min, body_b->position);
            box.max = UpeVec3_Add(box.max, body_b->position);
            has_contact = UpeNarrowphase_SphereAabb(sphere, box, &manifold);
        }
        else if (shape_a->type == UPE_SHAPE_BOX && shape_b->type == UPE_SHAPE_SPHERE) {
            UpeAabb3 box = shape_a->box;
            UpeSphere sphere = shape_b->sphere;
            box.min = UpeVec3_Add(box.min, body_a->position);
            box.max = UpeVec3_Add(box.max, body_a->position);
            sphere.center = UpeVec3_Add(sphere.center, body_b->position);
            has_contact = UpeNarrowphase_SphereAabb(sphere, box, &manifold);
            // Flip normal since we called SphereAabb with swapped arguments
            if (has_contact && manifold.count > 0) {
                manifold.points[0].normal = UpeVec3_Scale(manifold.points[0].normal, -1.0f);
            }
        }
        else if (shape_a->type == UPE_SHAPE_CAPSULE && shape_b->type == UPE_SHAPE_BOX) {
            UpeCapsule capsule = shape_a->capsule;
            UpeAabb3 box = shape_b->box;
            capsule.a = UpeVec3_Add(capsule.a, body_a->position);
            capsule.b = UpeVec3_Add(capsule.b, body_a->position);
            box.min = UpeVec3_Add(box.min, body_b->position);
            box.max = UpeVec3_Add(box.max, body_b->position);
            has_contact = UpeNarrowphase_CapsuleAabb(capsule, box, &manifold);
        }
        else if (shape_a->type == UPE_SHAPE_BOX && shape_b->type == UPE_SHAPE_CAPSULE) {
            UpeAabb3 box = shape_a->box;
            UpeCapsule capsule = shape_b->capsule;
            box.min = UpeVec3_Add(box.min, body_a->position);
            box.max = UpeVec3_Add(box.max, body_a->position);
            capsule.a = UpeVec3_Add(capsule.a, body_b->position);
            capsule.b = UpeVec3_Add(capsule.b, body_b->position);
            has_contact = UpeNarrowphase_CapsuleAabb(capsule, box, &manifold);
            // Flip normal since we called CapsuleAabb with swapped arguments
            if (has_contact && manifold.count > 0) {
                manifold.points[0].normal = UpeVec3_Scale(manifold.points[0].normal, -1.0f);
            }
        }
        else if (shape_a->type == UPE_SHAPE_OBB && shape_b->type == UPE_SHAPE_OBB) {
            /* OBB vs OBB collision via SAT */
            UpeOBB obb_a = shape_a->obb;
            UpeOBB obb_b = shape_b->obb;
            obb_a.center = UpeVec3_Add(obb_a.center, body_a->position);
            obb_b.center = UpeVec3_Add(obb_b.center, body_b->position);
            
            UpeOBBContact obb_contact = {};
            if (UpeOBB_TestOBB(&obb_a, &obb_b, &obb_contact) && obb_contact.contact_count > 0) {
                has_contact = 1;
                manifold = {};
                manifold.count = obb_contact.contact_count;
                for (uint32_t pc = 0; pc < manifold.count && pc < 4; ++pc) {
                    manifold.points[pc].position = obb_contact.contact_points[pc];
                    manifold.points[pc].normal = obb_contact.normal;
                    manifold.points[pc].penetration = obb_contact.penetration_depth;
                }
            }
        }
        else if (shape_a->type == UPE_SHAPE_OBB || shape_b->type == UPE_SHAPE_OBB) {
            /* OBB vs other shapes (GJK-based when convex involved) */
            if (shape_a->type == UPE_SHAPE_OBB && shape_b->type == UPE_SHAPE_CONVEX) {
                /* OBB vs Convex (GJK-based) */
                if (shape_b->convex) {
                    UpeNarrowphaseObbShape obb_shape = {
                        shape_a->obb.center,
                        shape_a->obb.half_extents,
                        {shape_a->obb.axes[0], shape_a->obb.axes[1], shape_a->obb.axes[2]}
                    };
                    has_contact = UpeNarrowphase_ObbConvex(&obb_shape, shape_b->convex, &manifold);
                }
            }
            else if (shape_a->type == UPE_SHAPE_CONVEX && shape_b->type == UPE_SHAPE_OBB) {
                /* Convex vs OBB (GJK-based, normal reversed) */
                if (shape_a->convex) {
                    UpeNarrowphaseObbShape obb_shape = {
                        shape_b->obb.center,
                        shape_b->obb.half_extents,
                        {shape_b->obb.axes[0], shape_b->obb.axes[1], shape_b->obb.axes[2]}
                    };
                    has_contact = UpeNarrowphase_ObbConvex(&obb_shape, shape_a->convex, &manifold);
                    if (has_contact) {
                        /* Reverse the normal for Convex vs OBB */
                        for (uint32_t i = 0; i < manifold.count; ++i) {
                            manifold.points[i].normal = UpeVec3_Scale(manifold.points[i].normal, -1.0f);
                        }
                    }
                }
            }
            else {
                /* OBB vs other non-convex shapes */
                has_contact = 0;
            }
        }
        else if (shape_a->type == UPE_SHAPE_CONVEX || shape_b->type == UPE_SHAPE_CONVEX) {
            /* Convex collision via GJK/EPA (v0.12.0) */
            if (shape_a->type == UPE_SHAPE_CONVEX && shape_b->type == UPE_SHAPE_CONVEX) {
                /* Convex vs Convex */
                if (shape_a->convex && shape_b->convex) {
                    has_contact = UpeNarrowphase_ConvexConvex(shape_a->convex, shape_b->convex, &manifold);
                }
            }
            else if (shape_a->type == UPE_SHAPE_SPHERE && shape_b->type == UPE_SHAPE_CONVEX) {
                /* Sphere vs Convex */
                if (shape_b->convex) {
                    UpeNarrowphaseSphereShape sphere_shape = {shape_a->sphere.center, shape_a->sphere.radius};
                    UpeNarrowphaseConvexShape convex_shape = {shape_b->convex->vertices, shape_b->convex->vertex_count};
                    has_contact = UpeNarrowphase_SphereConvex(&sphere_shape, &convex_shape, &manifold);
                }
            }
            else if (shape_a->type == UPE_SHAPE_CONVEX && shape_b->type == UPE_SHAPE_SPHERE) {
                /* Convex vs Sphere */
                if (shape_a->convex) {
                    UpeNarrowphaseConvexShape convex_shape = {shape_a->convex->vertices, shape_a->convex->vertex_count};
                    UpeNarrowphaseSphereShape sphere_shape = {shape_b->sphere.center, shape_b->sphere.radius};
                    has_contact = UpeNarrowphase_SphereConvex(&sphere_shape, &convex_shape, &manifold);
                    if (has_contact) {
                        /* Reverse the normal for Convex vs Sphere */
                        for (uint32_t i = 0; i < manifold.count; ++i) {
                            manifold.points[i].normal = UpeVec3_Scale(manifold.points[i].normal, -1.0f);
                        }
                    }
                }
            }
            else if (shape_a->type == UPE_SHAPE_CAPSULE && shape_b->type == UPE_SHAPE_CONVEX) {
                /* Capsule vs Convex */
                if (shape_b->convex) {
                    UpeNarrowphaseCapsuleShape capsule_shape = {shape_a->capsule.a, shape_a->capsule.b, shape_a->capsule.radius};
                    UpeNarrowphaseConvexShape convex_shape = {shape_b->convex->vertices, shape_b->convex->vertex_count};
                    has_contact = UpeNarrowphase_CapsuleConvex(&capsule_shape, &convex_shape, &manifold);
                }
            }
            else if (shape_a->type == UPE_SHAPE_CONVEX && shape_b->type == UPE_SHAPE_CAPSULE) {
                /* Convex vs Capsule */
                if (shape_a->convex) {
                    UpeNarrowphaseConvexShape convex_shape = {shape_a->convex->vertices, shape_a->convex->vertex_count};
                    UpeNarrowphaseCapsuleShape capsule_shape = {shape_b->capsule.a, shape_b->capsule.b, shape_b->capsule.radius};
                    has_contact = UpeNarrowphase_CapsuleConvex(&capsule_shape, &convex_shape, &manifold);
                    if (has_contact) {
                        /* Reverse the normal for Convex vs Capsule */
                        for (uint32_t i = 0; i < manifold.count; ++i) {
                            manifold.points[i].normal = UpeVec3_Scale(manifold.points[i].normal, -1.0f);
                        }
                    }
                }
            }
            else if (shape_a->type == UPE_SHAPE_BOX && shape_b->type == UPE_SHAPE_CONVEX) {
                /* Box vs Convex (GJK-based) */
                if (shape_b->convex) {
                    has_contact = UpeNarrowphase_BoxConvex(&shape_a->box, shape_b->convex, &manifold);
                }
            }
            else if (shape_a->type == UPE_SHAPE_CONVEX && shape_b->type == UPE_SHAPE_BOX) {
                /* Convex vs Box (GJK-based, normal reversed) */
                if (shape_a->convex) {
                    has_contact = UpeNarrowphase_BoxConvex(&shape_b->box, shape_a->convex, &manifold);
                    if (has_contact) {
                        /* Reverse the normal for Convex vs Box */
                        for (uint32_t i = 0; i < manifold.count; ++i) {
                            manifold.points[i].normal = UpeVec3_Scale(manifold.points[i].normal, -1.0f);
                        }
                    }
                }
            }
            else if (shape_a->type == UPE_SHAPE_MESH && shape_b->type == UPE_SHAPE_CONVEX) {
                /* Mesh vs Convex (GJK-based) */
                if (shape_b->convex && shape_a->mesh) {
                    /* Compute mesh center from vertices */
                    UpeVec3 mesh_center = {0.0f, 0.0f, 0.0f};
                    if (shape_a->mesh->vertex_count > 0) {
                        for (uint32_t i = 0; i < shape_a->mesh->vertex_count; ++i) {
                            mesh_center = UpeVec3_Add(mesh_center, shape_a->mesh->vertices[i]);
                        }
                        mesh_center = UpeVec3_Scale(mesh_center, 1.0f / (float)shape_a->mesh->vertex_count);
                    }
                    
                    UpeNarrowphaseMeshShape mesh_shape = {
                        shape_a->mesh->vertices,
                        shape_a->mesh->indices,
                        shape_a->mesh->vertex_count,
                        shape_a->mesh->index_count / 3,  /* Convert from index count to triangle count */
                        mesh_center
                    };
                    has_contact = UpeNarrowphase_MeshConvex(&mesh_shape, shape_b->convex, &manifold);
                }
            }
            else if (shape_a->type == UPE_SHAPE_CONVEX && shape_b->type == UPE_SHAPE_MESH) {
                /* Convex vs Mesh (GJK-based, normal reversed) */
                if (shape_a->convex && shape_b->mesh) {
                    /* Compute mesh center from vertices */
                    UpeVec3 mesh_center = {0.0f, 0.0f, 0.0f};
                    if (shape_b->mesh->vertex_count > 0) {
                        for (uint32_t i = 0; i < shape_b->mesh->vertex_count; ++i) {
                            mesh_center = UpeVec3_Add(mesh_center, shape_b->mesh->vertices[i]);
                        }
                        mesh_center = UpeVec3_Scale(mesh_center, 1.0f / (float)shape_b->mesh->vertex_count);
                    }
                    
                    UpeNarrowphaseMeshShape mesh_shape = {
                        shape_b->mesh->vertices,
                        shape_b->mesh->indices,
                        shape_b->mesh->vertex_count,
                        shape_b->mesh->index_count / 3,  /* Convert from index count to triangle count */
                        mesh_center
                    };
                    has_contact = UpeNarrowphase_MeshConvex(&mesh_shape, shape_a->convex, &manifold);
                    if (has_contact) {
                        /* Reverse the normal for Convex vs Mesh */
                        for (uint32_t i = 0; i < manifold.count; ++i) {
                            manifold.points[i].normal = UpeVec3_Scale(manifold.points[i].normal, -1.0f);
                        }
                    }
                }
            }
            else {
                has_contact = 0;
            }
        }
        
                if (has_contact && manifold.count > 0) {
                    if (world->contact_count < world->contact_capacity) {
                        UpeContactPair contact = {};
                        contact.body_a = a_idx;
                        contact.body_b = b_idx;
                        contact.shape_a = shape_a;
                        contact.shape_b = shape_b;
                        contact.manifold = manifold;
                        world->contacts[world->contact_count++] = contact;
                    }
                }

                shape_b_idx = shape_b->next_shape_index;
                if (world->contact_count >= world->contact_capacity) {
                    break;
                }
            }

            shape_a_idx = shape_a->next_shape_index;
            if (world->contact_count >= world->contact_capacity) {
                break;
            }
        }
    }

    // 3.5. CCD Pass: Detect fast-moving bodies and generate TOI contacts
    // This prevents tunneling for high-velocity objects
    if (world->ccd_enabled) {
        UpeAabb3 *ccd_bounds = nullptr;
        uint32_t *ccd_body_ids = nullptr;
        uint32_t *ccd_primary_shape_ids = nullptr;
        uint8_t *ccd_fast_flags = nullptr;
        uint32_t ccd_body_count = 0;

        if (world->memory && world->memory->frame_arena) {
            ccd_bounds = (UpeAabb3*)UpeFrameArena_Alloc(world->memory->frame_arena, world->body_capacity * sizeof(UpeAabb3), 16);
            ccd_body_ids = (uint32_t*)UpeFrameArena_Alloc(world->memory->frame_arena, world->body_capacity * sizeof(uint32_t), 16);
            ccd_primary_shape_ids = (uint32_t*)UpeFrameArena_Alloc(world->memory->frame_arena, world->body_capacity * sizeof(uint32_t), 16);
            ccd_fast_flags = (uint8_t*)UpeFrameArena_Alloc(world->memory->frame_arena, world->body_capacity * sizeof(uint8_t), 16);
        } else {
            if (!UpeWorld_EnsureScratch((void**)&world->scratch_ccd_bounds,
                                        &world->scratch_ccd_capacity,
                                        world->body_capacity,
                                        sizeof(UpeAabb3)) ||
                !UpeWorld_EnsureScratch((void**)&world->scratch_ccd_body_ids,
                                        &world->scratch_ccd_capacity,
                                        world->body_capacity,
                                        sizeof(uint32_t)) ||
                !UpeWorld_EnsureScratch((void**)&world->scratch_ccd_primary_shape_ids,
                                        &world->scratch_ccd_capacity,
                                        world->body_capacity,
                                        sizeof(uint32_t)) ||
                !UpeWorld_EnsureScratch((void**)&world->scratch_ccd_fast_flags,
                                        &world->scratch_ccd_capacity,
                                        world->body_capacity,
                                        sizeof(uint8_t))) {
                ccd_bounds = nullptr;
                ccd_body_ids = nullptr;
                ccd_primary_shape_ids = nullptr;
                ccd_fast_flags = nullptr;
            } else {
                ccd_bounds = world->scratch_ccd_bounds;
                ccd_body_ids = world->scratch_ccd_body_ids;
                ccd_primary_shape_ids = world->scratch_ccd_primary_shape_ids;
                ccd_fast_flags = world->scratch_ccd_fast_flags;
            }
        }

        if (ccd_bounds && ccd_body_ids && ccd_primary_shape_ids && ccd_fast_flags) {
            for (uint32_t alive_idx = 0; alive_idx < world->alive_body_count; ++alive_idx) {
                uint32_t i = world->alive_body_ids[alive_idx];

                UpeAabb3 body_aabb = {};
                uint32_t primary_shape = 0xFFFFFFFFu;
                if (!UpeWorld_ComputeBodyAabbAndPrimaryShape(world, i, &body_aabb, &primary_shape)) {
                    continue;
                }

                UpeBody *body = &world->bodies[i];
                float speed_sq = UpeVec3_Dot(body->velocity, body->velocity);
                int is_fast = (body->type == UPE_BODY_DYNAMIC && speed_sq >= world->ccd_threshold_sq) ? 1 : 0;

                if (is_fast) {
                    UpeVec3 sweep = UpeVec3_Scale(body->velocity, world->fixed_dt);
                    UpeAabb3 moved = {
                        UpeVec3_Add(body_aabb.min, sweep),
                        UpeVec3_Add(body_aabb.max, sweep)
                    };
                    body_aabb = UpeAabb3_Union(body_aabb, moved);
                }

                ccd_bounds[ccd_body_count] = body_aabb;
                ccd_body_ids[ccd_body_count] = i;
                ccd_primary_shape_ids[ccd_body_count] = primary_shape;
                ccd_fast_flags[ccd_body_count] = (uint8_t)is_fast;
                ccd_body_count++;
            }

            if (ccd_body_count >= 2) {
                UpeBroadphasePair *ccd_pairs = nullptr;
                uint32_t ccd_pair_count = 0;
                uint64_t ccd_worst_pairs64 = (uint64_t)ccd_body_count * (uint64_t)ccd_body_count;
                uint32_t ccd_worst_pairs = (ccd_worst_pairs64 > 0xFFFFFFFFull) ? 0xFFFFFFFFu : (uint32_t)ccd_worst_pairs64;
                uint32_t ccd_max_pairs = (world->max_pairs < ccd_worst_pairs) ? world->max_pairs : ccd_worst_pairs;

                if (world->memory && world->memory->frame_arena) {
                    uint32_t ccd_indices_size = ccd_body_count * sizeof(uint32_t);
                    uint32_t ccd_nodes_size = ccd_body_count * 2 * sizeof(UpeBvhNode);
                    uint32_t ccd_query_size = ccd_body_count * sizeof(uint32_t);
                    uint32_t ccd_scratch_size = ccd_indices_size + ccd_nodes_size + ccd_query_size;
                    void *ccd_scratch = UpeFrameArena_Alloc(world->memory->frame_arena, ccd_scratch_size, 16);
                    ccd_pairs = (UpeBroadphasePair*)UpeFrameArena_Alloc(world->memory->frame_arena,
                                                                        ccd_max_pairs * sizeof(UpeBroadphasePair),
                                                                        16);
                    if (ccd_scratch && ccd_pairs) {
                        ccd_pair_count = UpeBroadphase_BvhPairs(ccd_bounds,
                                                                ccd_body_count,
                                                                ccd_pairs,
                                                                ccd_max_pairs,
                                                                ccd_scratch,
                                                                ccd_scratch_size);
                    }
                } else {
                    if (UpeWorld_EnsureScratch(&world->scratch_pairs,
                                               &world->scratch_pairs_capacity,
                                               ccd_max_pairs,
                                               sizeof(UpeBroadphasePair))) {
                        ccd_pairs = (UpeBroadphasePair*)world->scratch_pairs;
                        ccd_pair_count = UpeBroadphase_SapPairs(ccd_bounds, ccd_body_count, ccd_pairs, ccd_max_pairs);
                    }
                }

                world->ccd_candidate_pairs = ccd_pair_count;

                if (ccd_pairs) {
                    for (uint32_t p = 0; p < ccd_pair_count; ++p) {
                        uint32_t a_local = ccd_pairs[p].a;
                        uint32_t b_local = ccd_pairs[p].b;
                        if (a_local >= ccd_body_count || b_local >= ccd_body_count || a_local == b_local) {
                            continue;
                        }
                        if (!ccd_fast_flags[a_local] && !ccd_fast_flags[b_local]) {
                            continue;
                        }

                        uint32_t body_a_id = ccd_body_ids[a_local];
                        uint32_t body_b_id = ccd_body_ids[b_local];
                        UpeBody *body_a = &world->bodies[body_a_id];
                        UpeBody *body_b = &world->bodies[body_b_id];

                        uint32_t shape_a_id = ccd_primary_shape_ids[a_local];
                        uint32_t shape_b_id = ccd_primary_shape_ids[b_local];
                        if (shape_a_id == 0xFFFFFFFFu || shape_b_id == 0xFFFFFFFFu ||
                            shape_a_id >= world->shape_capacity || shape_b_id >= world->shape_capacity) {
                            continue;
                        }

                        UpeShape *shape_a = &world->shapes[shape_a_id];
                        UpeShape *shape_b = &world->shapes[shape_b_id];

                        uint32_t layer_a = shape_a->collision_layer > 0 ? shape_a->collision_layer : body_a->collision_layer;
                        uint32_t mask_a = shape_a->collision_mask > 0 ? shape_a->collision_mask : body_a->collision_mask;
                        uint32_t layer_b = shape_b->collision_layer > 0 ? shape_b->collision_layer : body_b->collision_layer;
                        uint32_t mask_b = shape_b->collision_mask > 0 ? shape_b->collision_mask : body_b->collision_mask;

                        extern int UpeCollisionFilter_ShouldCollide(uint32_t body1_layer, uint32_t body1_mask,
                                                                   uint32_t body2_layer, uint32_t body2_mask);
                        if (!UpeCollisionFilter_ShouldCollide(layer_a, mask_a, layer_b, mask_b)) {
                            continue;
                        }

                        world->ccd_tested_pairs++;

                        UpeCcdResult ccd_result = {};
                        ccd_result.hit = 0;

                        if (shape_a->type == UPE_SHAPE_SPHERE && shape_b->type == UPE_SHAPE_SPHERE) {
                            UpeSphere sphere_a = shape_a->sphere;
                            UpeSphere sphere_b = shape_b->sphere;
                            sphere_a.center = UpeVec3_Add(sphere_a.center, body_a->position);
                            sphere_b.center = UpeVec3_Add(sphere_b.center, body_b->position);

                            UpeVec3 vel_b = (body_b->type == UPE_BODY_DYNAMIC) ? body_b->velocity : UpeVec3{0, 0, 0};
                            ccd_result = UpeCcd_SphereSphere(sphere_a, body_a->velocity, sphere_b, vel_b, world->fixed_dt);
                        }
                        else if (shape_a->type == UPE_SHAPE_CAPSULE && shape_b->type == UPE_SHAPE_CAPSULE) {
                            UpeCapsule capsule_a = shape_a->capsule;
                            UpeCapsule capsule_b = shape_b->capsule;
                            capsule_a.a = UpeVec3_Add(capsule_a.a, body_a->position);
                            capsule_a.b = UpeVec3_Add(capsule_a.b, body_a->position);
                            capsule_b.a = UpeVec3_Add(capsule_b.a, body_b->position);
                            capsule_b.b = UpeVec3_Add(capsule_b.b, body_b->position);

                            UpeVec3 vel_b = (body_b->type == UPE_BODY_DYNAMIC) ? body_b->velocity : UpeVec3{0, 0, 0};
                            ccd_result = UpeCcd_CapsuleCapsule(capsule_a, body_a->velocity, capsule_b, vel_b, world->fixed_dt);
                        }
                        else if (shape_a->type == UPE_SHAPE_SPHERE && shape_b->type == UPE_SHAPE_CAPSULE) {
                            UpeSphere sphere = shape_a->sphere;
                            UpeCapsule capsule = shape_b->capsule;
                            sphere.center = UpeVec3_Add(sphere.center, body_a->position);
                            capsule.a = UpeVec3_Add(capsule.a, body_b->position);
                            capsule.b = UpeVec3_Add(capsule.b, body_b->position);

                            UpeVec3 vel_b = (body_b->type == UPE_BODY_DYNAMIC) ? body_b->velocity : UpeVec3{0, 0, 0};
                            ccd_result = UpeCcd_SphereCapsule(sphere, body_a->velocity, capsule, vel_b, world->fixed_dt);
                        }
                        else if (shape_a->type == UPE_SHAPE_SPHERE && shape_b->type == UPE_SHAPE_BOX) {
                            UpeSphere sphere = shape_a->sphere;
                            UpeAabb3 box = shape_b->box;
                            sphere.center = UpeVec3_Add(sphere.center, body_a->position);
                            box.min = UpeVec3_Add(box.min, body_b->position);
                            box.max = UpeVec3_Add(box.max, body_b->position);

                            UpeVec3 vel_b = (body_b->type == UPE_BODY_DYNAMIC) ? body_b->velocity : UpeVec3{0, 0, 0};
                            ccd_result = UpeCcd_SphereBox(sphere, body_a->velocity, box, vel_b, world->fixed_dt);
                        }
                        else if (shape_a->type == UPE_SHAPE_BOX && shape_b->type == UPE_SHAPE_SPHERE) {
                            UpeAabb3 box = shape_a->box;
                            UpeSphere sphere = shape_b->sphere;
                            box.min = UpeVec3_Add(box.min, body_a->position);
                            box.max = UpeVec3_Add(box.max, body_a->position);
                            sphere.center = UpeVec3_Add(sphere.center, body_b->position);

                            UpeVec3 vel_b = (body_b->type == UPE_BODY_DYNAMIC) ? body_b->velocity : UpeVec3{0, 0, 0};
                            ccd_result = UpeCcd_SphereBox(sphere, vel_b, box, body_a->velocity, world->fixed_dt);
                        }
                        else if (shape_a->type == UPE_SHAPE_BOX && shape_b->type == UPE_SHAPE_BOX) {
                            UpeAabb3 box_a = shape_a->box;
                            UpeAabb3 box_b = shape_b->box;
                            box_a.min = UpeVec3_Add(box_a.min, body_a->position);
                            box_a.max = UpeVec3_Add(box_a.max, body_a->position);
                            box_b.min = UpeVec3_Add(box_b.min, body_b->position);
                            box_b.max = UpeVec3_Add(box_b.max, body_b->position);

                            UpeVec3 vel_b = (body_b->type == UPE_BODY_DYNAMIC) ? body_b->velocity : UpeVec3{0, 0, 0};
                            ccd_result = UpeCcd_BoxBox(box_a, body_a->velocity, box_b, vel_b, world->fixed_dt);
                        }
                        else if (shape_a->type == UPE_SHAPE_CAPSULE && shape_b->type == UPE_SHAPE_BOX) {
                            UpeCapsule capsule = shape_a->capsule;
                            UpeAabb3 box = shape_b->box;
                            capsule.a = UpeVec3_Add(capsule.a, body_a->position);
                            capsule.b = UpeVec3_Add(capsule.b, body_a->position);
                            box.min = UpeVec3_Add(box.min, body_b->position);
                            box.max = UpeVec3_Add(box.max, body_b->position);

                            UpeVec3 vel_b = (body_b->type == UPE_BODY_DYNAMIC) ? body_b->velocity : UpeVec3{0, 0, 0};
                            ccd_result = UpeCcd_CapsuleBox(capsule, body_a->velocity, box, vel_b, world->fixed_dt);
                        }
                        else if (shape_a->type == UPE_SHAPE_BOX && shape_b->type == UPE_SHAPE_CAPSULE) {
                            UpeAabb3 box = shape_a->box;
                            UpeCapsule capsule = shape_b->capsule;
                            box.min = UpeVec3_Add(box.min, body_a->position);
                            box.max = UpeVec3_Add(box.max, body_a->position);
                            capsule.a = UpeVec3_Add(capsule.a, body_b->position);
                            capsule.b = UpeVec3_Add(capsule.b, body_b->position);

                            UpeVec3 vel_b = (body_b->type == UPE_BODY_DYNAMIC) ? body_b->velocity : UpeVec3{0, 0, 0};
                            ccd_result = UpeCcd_CapsuleBox(capsule, vel_b, box, body_a->velocity, world->fixed_dt);
                        }

                        if (ccd_result.hit && ccd_result.toi < world->fixed_dt && world->contact_count < world->contact_capacity) {
                            UpeContactPair contact = {};
                            contact.body_a = body_a_id;
                            contact.body_b = body_b_id;
                            contact.shape_a = shape_a;
                            contact.shape_b = shape_b;
                            contact.manifold.count = 1;
                            contact.manifold.points[0].position = ccd_result.position;
                            contact.manifold.points[0].normal = ccd_result.normal;
                            contact.manifold.points[0].penetration = 0.01f;
                            world->contacts[world->contact_count++] = contact;
                            world->ccd_contact_hits++;
                        }
                    }
                }
            }
        }
    }

    UpeWorldSort_Contacts(world, world->contacts, world->contact_count, world->memory);
    world->last_contact_count = world->contact_count;
    UpeWorld_EndStage(world, UPE_STEP_STAGE_NARROWPHASE, world->contact_count);
    UpeWorld_BeginStage(world, UPE_STEP_STAGE_ISLANDS, world->contact_count);

    if (world->profiler) {
        UpeProfiler_EndZone(world->profiler);
        UpeProfiler_BeginZone(world->profiler, "islands");
    }

    // 4. Build islands and handle sleeping
    // Union-find parent array for grouping connected bodies
    if (!world->body_island_ids || world->island_capacity < world->body_capacity) {
        if (world->memory) {
            UpeMemoryContext_FinalizeFrame(world->memory, world->fixed_dt * 1000.0f);
        }
        world->query_bvh_dirty = 1;
        if (world->profiler) {
            UpeProfiler_EndZone(world->profiler);
            UpeProfiler_EndFrame(world->profiler);
        }
        return;
    }
    
    // Initialize union-find: each body is its own parent
    for (uint32_t i = 0; i < world->body_capacity; ++i) {
        world->body_island_ids[i] = i;
    }
    
    // Union-find helper functions
    auto find_root = [&](uint32_t id) -> uint32_t {
        while (world->body_island_ids[id] != id) {
            // Path compression
            uint32_t parent = world->body_island_ids[id];
            world->body_island_ids[id] = world->body_island_ids[parent];
            id = parent;
        }
        return id;
    };
    
    auto union_bodies = [&](uint32_t a, uint32_t b) {
        uint32_t root_a = find_root(a);
        uint32_t root_b = find_root(b);
        if (root_a != root_b) {
            world->body_island_ids[root_b] = root_a;
        }
    };
    
    // Connect bodies via contacts
    for (uint32_t c = 0; c < world->contact_count; ++c) {
        uint32_t body_a = world->contacts[c].body_a;
        uint32_t body_b = world->contacts[c].body_b;
        if (body_a >= world->body_capacity || body_b >= world->body_capacity) {
            continue;
        }
        if (world->bodies[body_a].type != UPE_BODY_DYNAMIC || world->bodies[body_b].type != UPE_BODY_DYNAMIC) {
            continue;
        }
        union_bodies(body_a, body_b);
    }
    
    // Connect bodies via joints
    for (uint32_t j = 0; j < world->joint_count; ++j) {
        uint32_t body_a = world->joints[j].body_a;
        uint32_t body_b = world->joints[j].body_b;
        if (body_a >= world->body_capacity || body_b >= world->body_capacity) {
            continue;
        }
        if (world->bodies[body_a].type != UPE_BODY_DYNAMIC || world->bodies[body_b].type != UPE_BODY_DYNAMIC) {
            continue;
        }
        union_bodies(body_a, body_b);
    }
    
    // Build island structures from roots
    uint32_t *root_to_island = nullptr;
    if (world->memory && world->memory->frame_arena) {
        root_to_island = (uint32_t*)UpeFrameArena_Alloc(
            world->memory->frame_arena,
            world->body_capacity * sizeof(uint32_t),
            16);
    } else if (UpeWorld_EnsureScratch((void**)&world->scratch_root_to_island,
                                      &world->scratch_root_to_island_capacity,
                                      world->body_capacity,
                                      sizeof(uint32_t))) {
        root_to_island = world->scratch_root_to_island;
    }

    if (!root_to_island) {
        if (world->memory) {
            UpeMemoryContext_FinalizeFrame(world->memory, world->fixed_dt * 1000.0f);
        }
        world->query_bvh_dirty = 1;
        if (world->profiler) {
            UpeProfiler_EndZone(world->profiler);
            UpeProfiler_EndFrame(world->profiler);
        }
        return;
    }

    for (uint32_t i = 0; i < world->body_capacity; ++i) {
        root_to_island[i] = 0xFFFFFFFFu;
    }

    // First pass: assign compact island IDs to distinct roots
    uint32_t island_count = 0;
    for (uint32_t dyn = 0; dyn < active_body_count; ++dyn) {
        uint32_t i = body_indices[dyn];
        uint32_t root = find_root(i);
        if (root < world->body_capacity && root_to_island[root] == 0xFFFFFFFFu) {
            root_to_island[root] = island_count++;
        }
    }

    // Second pass: write island IDs to dynamic bodies
    for (uint32_t dyn = 0; dyn < active_body_count; ++dyn) {
        uint32_t i = body_indices[dyn];
        uint32_t root = find_root(i);
        if (root < world->body_capacity && root_to_island[root] != 0xFFFFFFFFu) {
            world->body_island_ids[i] = root_to_island[root];
        } else {
            world->body_island_ids[i] = 0;
        }
    }
    
    // Resize island array if needed
    if (world->island_capacity < island_count) {
        if (world->memory) {
            UpeMemoryContext_FinalizeFrame(world->memory, world->fixed_dt * 1000.0f);
        }
        world->query_bvh_dirty = 1;
        if (world->profiler) {
            UpeProfiler_EndZone(world->profiler);
            UpeProfiler_EndFrame(world->profiler);
        }
        return;
    }
    world->island_count = island_count;
    world->last_island_count = island_count;
    
    // Clear island body lists
    for (uint32_t i = 0; i < world->island_count; ++i) {
        world->islands[i].body_count = 0;
    }
    
    // Second pass: populate islands with body indices
    for (uint32_t dyn = 0; dyn < active_body_count; ++dyn) {
        uint32_t i = body_indices[dyn];
        uint32_t island_id = world->body_island_ids[i];
        if (island_id < world->island_count) {
            UpeIsland_AddBody(&world->islands[island_id], i);
        }
    }
    
    // Compute kinetic energy and update sleep state for each island
    const float sleep_threshold = 0.01f;  // Kinetic energy threshold
    const float sleep_time_required = 0.5f;  // Seconds below threshold before sleeping
    
    for (uint32_t i = 0; i < world->island_count; ++i) {
        UpeIsland *island = &world->islands[i];
        if (!world->enable_sleeping) {
            island->awake = 1;
            island->sleep_time = 0.0f;
        }
        if (!island->awake) {
            continue;  // Already sleeping
        }
        
        // Compute total kinetic energy
        float ke = 0.0f;
        for (uint32_t b = 0; b < island->body_count; ++b) {
            uint32_t body_idx = island->bodies[b];
            UpeBody *body = &world->bodies[body_idx];
            
            float v_sq = UpeVec3_Dot(body->velocity, body->velocity);
            float w_sq = UpeVec3_Dot(body->angular_velocity, body->angular_velocity);
            ke += 0.5f * body->mass * v_sq;
            ke += 0.5f * w_sq;  // Simplified angular KE
        }
        
        island->kinetic_energy = ke;
        
        // Update sleep timer
        if (world->enable_sleeping && ke < sleep_threshold) {
            island->sleep_time += world->fixed_dt;
            if (island->sleep_time >= sleep_time_required) {
                UpeIsland_Sleep(island);
                // Zero out velocities for sleeping bodies
                for (uint32_t b = 0; b < island->body_count; ++b) {
                    uint32_t body_idx = island->bodies[b];
                    world->bodies[body_idx].velocity = {0.0f, 0.0f, 0.0f};
                    world->bodies[body_idx].angular_velocity = {0.0f, 0.0f, 0.0f};
                }
            }
        } else {
            island->sleep_time = 0.0f;
        }
    }

    // 5. Build solver constraints from contacts (only for awake islands)
    UpeSolverConstraint *constraints = nullptr;
    uint32_t constraint_count = 0;
    if (world->contact_count > 0) {
        if (world->memory && world->memory->frame_arena) {
            constraints = (UpeSolverConstraint*)UpeFrameArena_Alloc(
                world->memory->frame_arena,
                world->contact_count * 4 * sizeof(UpeSolverConstraint),
                16);
        } else {
            uint32_t required_constraints = world->contact_count * 4;
            if (UpeWorld_EnsureScratch(&world->scratch_constraints,
                                       &world->scratch_constraints_capacity,
                                       required_constraints,
                                       sizeof(UpeSolverConstraint))) {
                constraints = (UpeSolverConstraint*)world->scratch_constraints;
            }
        }
        for (uint32_t c = 0; c < world->contact_count; ++c) {
            UpeContactPair *pair = &world->contacts[c];
            UpeBody *body_a = &world->bodies[pair->body_a];
            UpeBody *body_b = &world->bodies[pair->body_b];
            UpeShape *shape_a = pair->shape_a;
            UpeShape *shape_b = pair->shape_b;
            
            // Skip if both bodies are in sleeping islands
            int a_sleeping = 0, b_sleeping = 0;
            if (body_a->type == UPE_BODY_DYNAMIC) {
                uint32_t island_a = world->body_island_ids[pair->body_a];
                if (world->enable_sleeping && island_a < world->island_count && !world->islands[island_a].awake) {
                    a_sleeping = 1;
                }
            }
            if (body_b->type == UPE_BODY_DYNAMIC) {
                uint32_t island_b = world->body_island_ids[pair->body_b];
                if (world->enable_sleeping && island_b < world->island_count && !world->islands[island_b].awake) {
                    b_sleeping = 1;
                }
            }
            if (a_sleeping && b_sleeping) {
                continue;  // Skip this contact
            }
            
            // Wake up islands if one body is awake and touching
            if (!a_sleeping && b_sleeping) {
                uint32_t island_b = world->body_island_ids[pair->body_b];
                if (island_b < world->island_count) {
                    UpeIsland_Wake(&world->islands[island_b]);
                }
            }
            if (a_sleeping && !b_sleeping) {
                uint32_t island_a = world->body_island_ids[pair->body_a];
                if (island_a < world->island_count) {
                    UpeIsland_Wake(&world->islands[island_a]);
                }
            }
            
            for (uint32_t pt = 0; pt < pair->manifold.count; ++pt) {
                UpeContactPoint *point = &pair->manifold.points[pt];
                UpeSolverConstraint constraint = {};
                constraint.body_a = pair->body_a;
                constraint.body_b = pair->body_b;
                constraint.normal = point->normal;
                constraint.contact_point = point->position;
                constraint.penetration = point->penetration;
                constraint.lambda = 0.0f;
                constraint.lambda_t1 = 0.0f;
                constraint.lambda_t2 = 0.0f;
                
                // Material properties: average per-shape values
                constraint.friction = 0.5f;
                constraint.restitution = 0.3f;
                if (shape_a && shape_b) {
                    constraint.friction = 0.5f * (shape_a->friction + shape_b->friction);
                    constraint.restitution = 0.5f * (shape_a->restitution + shape_b->restitution);
                }
                
                // Compute contact point relative to body centers
                constraint.r_a = UpeVec3_Sub(point->position, body_a->position);
                constraint.r_b = UpeVec3_Sub(point->position, body_b->position);
                
                // Compute friction tangent vectors (perpendicular to normal)
                UpeVec3 tangent_ref = {1.0f, 0.0f, 0.0f};
                if (fabsf(UpeVec3_Dot(point->normal, tangent_ref)) > 0.9f) {
                    tangent_ref = {0.0f, 1.0f, 0.0f};
                }
                constraint.tangent1 = UpeVec3_Normalize(UpeVec3_Cross(point->normal, tangent_ref));
                constraint.tangent2 = UpeVec3_Cross(point->normal, constraint.tangent1);
                
                constraints[constraint_count++] = constraint;
            }
        }
    }

    UpeWorldSort_Constraints(constraints, constraint_count, world->memory);
    world->last_constraint_count = constraint_count;

    if (world->island_constraint_offsets && world->island_constraint_counts) {
        memset(world->island_constraint_offsets, 0, world->island_capacity * sizeof(uint32_t));
        memset(world->island_constraint_counts, 0, world->island_capacity * sizeof(uint32_t));
    }

    if (constraint_count > 0 && world->island_count > 0 &&
        world->island_constraint_offsets && world->island_constraint_counts) {
        uint32_t *constraint_island_ids = nullptr;
        uint32_t *cursor = nullptr;
        UpeSolverConstraint *reordered_constraints = nullptr;

        if (world->memory && world->memory->frame_arena) {
            constraint_island_ids = (uint32_t*)UpeFrameArena_Alloc(world->memory->frame_arena,
                constraint_count * sizeof(uint32_t), 16);
            cursor = (uint32_t*)UpeFrameArena_Alloc(world->memory->frame_arena,
                world->island_count * sizeof(uint32_t), 16);
            reordered_constraints = (UpeSolverConstraint*)UpeFrameArena_Alloc(world->memory->frame_arena,
                constraint_count * sizeof(UpeSolverConstraint), 16);
        } else {
            if (UpeWorld_EnsureScratch((void**)&world->scratch_constraint_island_ids,
                                       &world->scratch_constraint_island_ids_capacity,
                                       constraint_count,
                                       sizeof(uint32_t)) &&
                UpeWorld_EnsureScratch((void**)&world->scratch_constraint_cursor,
                                       &world->scratch_constraint_cursor_capacity,
                                       world->island_count,
                                       sizeof(uint32_t)) &&
                UpeWorld_EnsureScratch(&world->scratch_reordered_constraints,
                                       &world->scratch_reordered_constraints_capacity,
                                       constraint_count,
                                       sizeof(UpeSolverConstraint))) {
                constraint_island_ids = world->scratch_constraint_island_ids;
                cursor = world->scratch_constraint_cursor;
                reordered_constraints = (UpeSolverConstraint*)world->scratch_reordered_constraints;
            }
        }

        if (constraint_island_ids && cursor && reordered_constraints) {
            for (uint32_t i = 0; i < constraint_count; ++i) {
                UpeSolverConstraint *c = &constraints[i];
                uint32_t island_id = 0;

                if (c->body_a < world->body_capacity && world->bodies[c->body_a].type == UPE_BODY_DYNAMIC) {
                    island_id = world->body_island_ids[c->body_a];
                } else if (c->body_b < world->body_capacity && world->bodies[c->body_b].type == UPE_BODY_DYNAMIC) {
                    island_id = world->body_island_ids[c->body_b];
                }

                if (island_id >= world->island_count) {
                    island_id = world->island_count - 1;
                }

                constraint_island_ids[i] = island_id;
                world->island_constraint_counts[island_id] += 1;
            }

            uint32_t running = 0;
            for (uint32_t island = 0; island < world->island_count; ++island) {
                world->island_constraint_offsets[island] = running;
                cursor[island] = running;
                running += world->island_constraint_counts[island];
            }

            for (uint32_t i = 0; i < constraint_count; ++i) {
                uint32_t island_id = constraint_island_ids[i];
                uint32_t dst = cursor[island_id]++;
                reordered_constraints[dst] = constraints[i];
            }

            memcpy(constraints, reordered_constraints, constraint_count * sizeof(UpeSolverConstraint));
        }
    }

    // Warm-starting: match current constraints with previous frame and copy lambda values
    // Uses a body-pair hash table to avoid O(n^2) scans.
    if (world->enable_warm_starting && world->prev_constraints && world->prev_constraint_count > 0) {
        uint32_t bucket_count = 1;
        while (bucket_count < (world->prev_constraint_count * 2u)) {
            bucket_count <<= 1;
        }
        int32_t *bucket_heads = nullptr;
        uint32_t *bucket_next = nullptr;
        if (world->memory && world->memory->frame_arena) {
            bucket_heads = (int32_t*)UpeFrameArena_Alloc(world->memory->frame_arena, bucket_count * sizeof(int32_t), 16);
            bucket_next = (uint32_t*)UpeFrameArena_Alloc(world->memory->frame_arena, world->prev_constraint_count * sizeof(uint32_t), 16);
        } else {
            if (UpeWorld_EnsureScratch((void**)&world->scratch_bucket_heads,
                                       &world->scratch_bucket_heads_capacity,
                                       bucket_count,
                                       sizeof(int32_t)) &&
                UpeWorld_EnsureScratch((void**)&world->scratch_bucket_next,
                                       &world->scratch_bucket_next_capacity,
                                       world->prev_constraint_count,
                                       sizeof(uint32_t))) {
                bucket_heads = world->scratch_bucket_heads;
                bucket_next = world->scratch_bucket_next;
            }
        }

        if (bucket_heads && bucket_next) {
            for (uint32_t b = 0; b < bucket_count; ++b) {
                bucket_heads[b] = -1;
            }

            uint32_t mask = bucket_count - 1u;
            for (uint32_t j = 0; j < world->prev_constraint_count; ++j) {
                const UpeSolverConstraint *prev = &world->prev_constraints[j];
                uint64_t pair_key = UpeWorld_OrderedBodyPairKey(prev->body_a, prev->body_b);
                uint32_t bucket = (uint32_t)(UpeWorld_HashPairKey(pair_key) & mask);
                bucket_next[j] = (bucket_heads[bucket] >= 0) ? (uint32_t)bucket_heads[bucket] : 0xFFFFFFFFu;
                bucket_heads[bucket] = (int32_t)j;
            }

            for (uint32_t i = 0; i < constraint_count; ++i) {
                UpeSolverConstraint *curr = &constraints[i];
                uint64_t pair_key = UpeWorld_OrderedBodyPairKey(curr->body_a, curr->body_b);
                uint32_t bucket = (uint32_t)(UpeWorld_HashPairKey(pair_key) & mask);

                uint32_t entry = (bucket_heads[bucket] >= 0) ? (uint32_t)bucket_heads[bucket] : 0xFFFFFFFFu;
                while (entry != 0xFFFFFFFFu) {
                    UpeSolverConstraint *prev = &world->prev_constraints[entry];
                    if (UpeWorld_OrderedBodyPairKey(prev->body_a, prev->body_b) == pair_key) {
                        UpeVec3 pos_diff = UpeVec3_Sub(curr->contact_point, prev->contact_point);
                        float dist_sq = UpeVec3_Dot(pos_diff, pos_diff);
                        if (dist_sq < 0.01f) {
                            curr->lambda = prev->lambda;
                            curr->lambda_t1 = prev->lambda_t1;
                            curr->lambda_t2 = prev->lambda_t2;
                            break;
                        }
                    }
                    entry = bucket_next[entry];
                }
            }
        }
    }

    UpeWorldSort_Joints(world->joints, world->joint_count, world->memory);
    UpeWorld_EndStage(world, UPE_STEP_STAGE_ISLANDS, constraint_count);
    UpeWorld_BeginStage(world, UPE_STEP_STAGE_SOLVER, constraint_count);

    if (world->profiler) {
        UpeProfiler_EndZone(world->profiler);
        UpeProfiler_BeginZone(world->profiler, "solver");
    }

    // 5. Solve constraints (PGS iterations)
    UpeBodyState *solver_bodies = nullptr;
    if (world->memory && world->memory->frame_arena) {
        solver_bodies = (UpeBodyState*)UpeFrameArena_Alloc(
            world->memory->frame_arena,
            world->body_capacity * sizeof(UpeBodyState),
            16);
    } else {
        if (UpeWorld_EnsureScratch(&world->scratch_solver_bodies,
                                   &world->scratch_solver_bodies_capacity,
                                   world->body_capacity,
                                   sizeof(UpeBodyState))) {
            solver_bodies = (UpeBodyState*)world->scratch_solver_bodies;
        }
    }

    if (!solver_bodies) {
        if (world->memory) {
            UpeMemoryContext_FinalizeFrame(world->memory, world->fixed_dt * 1000.0f);
        }
        world->query_bvh_dirty = 1;
        if (world->profiler) {
            UpeProfiler_EndZone(world->profiler);
            UpeProfiler_EndFrame(world->profiler);
        }
        return;
    }

    for (uint32_t dyn = 0; dyn < active_body_count; ++dyn) {
        uint32_t i = body_indices[dyn];
        solver_bodies[i].position = world->bodies[i].position;
        solver_bodies[i].velocity = world->bodies[i].velocity;
        solver_bodies[i].rotation = world->bodies[i].rotation;
        solver_bodies[i].angular_velocity = world->bodies[i].angular_velocity;
        solver_bodies[i].inv_mass = world->bodies[i].inv_mass;
        solver_bodies[i].inv_inertia = world->bodies[i].inv_inertia;
    }

    const uint32_t solver_iterations = (world->solver_iterations > 0) ? world->solver_iterations : 1;
    world->last_solver_iterations = solver_iterations;
    world->last_solver_parallel_islands = 0;
    world->last_solver_serial_islands = 0;
    world->last_solver_island_task_count = 0;

    UpeIslandSolveTask *island_tasks = nullptr;
    uint32_t island_task_count = 0;
    uint32_t parallel_eligible_task_count = 0;
    uint32_t parallel_eligible_constraint_count = 0;
    const int can_parallelize_islands =
        (world->enable_parallel_island_solver != 0) &&
        (world->solver_thread_count > 1) &&
        (world->island_count > 1) &&
        (constraint_count > 0) &&
        (world->island_constraint_offsets != nullptr) &&
        (world->island_constraint_counts != nullptr);

    if (can_parallelize_islands) {
        if (world->memory && world->memory->frame_arena) {
            island_tasks = (UpeIslandSolveTask*)UpeFrameArena_Alloc(
                world->memory->frame_arena,
                world->island_count * sizeof(UpeIslandSolveTask),
                16);
        } else {
            if (UpeWorld_EnsureScratch(&world->scratch_island_tasks,
                                       &world->scratch_island_tasks_capacity,
                                       world->island_count,
                                       sizeof(UpeIslandSolveTask))) {
                island_tasks = (UpeIslandSolveTask*)world->scratch_island_tasks;
            }
        }

        if (island_tasks) {
            for (uint32_t island = 0; island < world->island_count; ++island) {
                uint32_t count = world->island_constraint_counts[island];
                if (count == 0) {
                    continue;
                }

                island_tasks[island_task_count].solver_bodies = solver_bodies;
                island_tasks[island_task_count].body_capacity = world->body_capacity;
                island_tasks[island_task_count].constraints = constraints;
                island_tasks[island_task_count].constraint_offset = world->island_constraint_offsets[island];
                island_tasks[island_task_count].constraint_count = count;

                uint32_t allow_parallel = 1;
                uint32_t begin = world->island_constraint_offsets[island];
                uint32_t end = begin + count;
                for (uint32_t ci = begin; ci < end; ++ci) {
                    const UpeSolverConstraint *constraint = &constraints[ci];
                    if (constraint->body_a >= world->body_capacity || constraint->body_b >= world->body_capacity) {
                        allow_parallel = 0;
                        break;
                    }
                    if (world->bodies[constraint->body_a].type != UPE_BODY_DYNAMIC ||
                        world->bodies[constraint->body_b].type != UPE_BODY_DYNAMIC) {
                        allow_parallel = 0;
                        break;
                    }
                }
                island_tasks[island_task_count].allow_parallel = allow_parallel;
                if (allow_parallel) {
                    parallel_eligible_task_count += 1;
                    parallel_eligible_constraint_count += count;
                }
                island_task_count += 1;
            }
            world->last_solver_island_task_count = island_task_count;
        }
    }

    const uint32_t min_parallel_tasks = 2;
    const uint32_t min_parallel_constraints_total = 2048;
    const uint32_t min_parallel_constraints_per_task = 128;
    const uint32_t avg_parallel_constraints =
        (parallel_eligible_task_count > 0)
            ? (parallel_eligible_constraint_count / parallel_eligible_task_count)
            : 0;
    const int use_parallel_dispatch =
        (island_tasks != nullptr) &&
        (island_task_count > 1) &&
        (parallel_eligible_task_count >= min_parallel_tasks) &&
        (parallel_eligible_constraint_count >= min_parallel_constraints_total) &&
        (avg_parallel_constraints >= min_parallel_constraints_per_task);

    UpeThreadWorkItem *parallel_work_items = nullptr;
    uint32_t parallel_work_item_count = 0;
    uint32_t *parallel_task_indices = nullptr;
    UpeIslandSolveBatchTask *parallel_batches = nullptr;
    uint32_t parallel_batch_count = 0;
    if (use_parallel_dispatch) {
        uint32_t max_batch_count = world->solver_thread_count;
        if (max_batch_count == 0) {
            max_batch_count = 1;
        }
        if (max_batch_count > parallel_eligible_task_count) {
            max_batch_count = parallel_eligible_task_count;
        }

        if (world->memory && world->memory->frame_arena) {
            parallel_task_indices = (uint32_t*)UpeFrameArena_Alloc(
                world->memory->frame_arena,
                parallel_eligible_task_count * sizeof(uint32_t),
                16);
            parallel_batches = (UpeIslandSolveBatchTask*)UpeFrameArena_Alloc(
                world->memory->frame_arena,
                max_batch_count * sizeof(UpeIslandSolveBatchTask),
                16);
            parallel_work_items = (UpeThreadWorkItem*)UpeFrameArena_Alloc(
                world->memory->frame_arena,
                max_batch_count * sizeof(UpeThreadWorkItem),
                16);
        } else {
            if (UpeWorld_EnsureScratch((void**)&world->scratch_parallel_task_indices,
                                       &world->scratch_parallel_task_indices_capacity,
                                       parallel_eligible_task_count,
                                       sizeof(uint32_t))) {
                parallel_task_indices = world->scratch_parallel_task_indices;
            }
            if (UpeWorld_EnsureScratch(&world->scratch_island_batch_tasks,
                                       &world->scratch_island_batch_tasks_capacity,
                                       max_batch_count,
                                       sizeof(UpeIslandSolveBatchTask))) {
                parallel_batches = (UpeIslandSolveBatchTask*)world->scratch_island_batch_tasks;
            }
            if (UpeWorld_EnsureScratch(&world->scratch_thread_work_items,
                                       &world->scratch_thread_work_items_capacity,
                                       max_batch_count,
                                       sizeof(UpeThreadWorkItem))) {
                parallel_work_items = (UpeThreadWorkItem*)world->scratch_thread_work_items;
            }
        }

        if (parallel_task_indices && parallel_batches && parallel_work_items) {
            for (uint32_t t = 0; t < island_task_count; ++t) {
                if (island_tasks[t].allow_parallel == 0) {
                    continue;
                }
                parallel_task_indices[parallel_work_item_count++] = t;
            }

            parallel_batch_count = max_batch_count;
            uint32_t base = parallel_work_item_count / parallel_batch_count;
            uint32_t rem = parallel_work_item_count % parallel_batch_count;
            uint32_t cursor = 0;
            for (uint32_t b = 0; b < parallel_batch_count; ++b) {
                uint32_t count = base + ((b < rem) ? 1u : 0u);
                parallel_batches[b].island_tasks = island_tasks;
                parallel_batches[b].island_task_indices = parallel_task_indices;
                parallel_batches[b].start = cursor;
                parallel_batches[b].count = count;
                parallel_work_items[b].func = UpeWorld_SolveIslandBatchTask;
                parallel_work_items[b].data = &parallel_batches[b];
                cursor += count;
            }

            parallel_work_item_count = parallel_batch_count;
        }
    }

    const int use_parallel_dispatch_runtime =
        use_parallel_dispatch &&
        (parallel_task_indices != nullptr) &&
        (parallel_batches != nullptr) &&
        (parallel_work_items != nullptr) &&
        (parallel_work_item_count > 1);

    for (uint32_t iter = 0; iter < solver_iterations; ++iter) {
        if (use_parallel_dispatch_runtime) {
            uint32_t parallel_solved = 0;
            uint32_t serial_solved = 0;

            uint32_t submitted = UpeThreading_SubmitBatchWork(parallel_work_items, parallel_work_item_count);
            for (uint32_t b = 0; b < submitted; ++b) {
                parallel_solved += parallel_batches[b].count;
            }

            if (submitted > 0) {
                UpeThreading_WaitAll();
            }

            for (uint32_t i = submitted; i < parallel_work_item_count; ++i) {
                UpeThreadWorkItem *item = &parallel_work_items[i];
                if (item->func) {
                    item->func(item->data);
                    serial_solved += parallel_batches[i].count;
                }
            }

            for (uint32_t t = 0; t < island_task_count; ++t) {
                if (island_tasks[t].allow_parallel != 0) {
                    continue;
                }
                UpeWorld_SolveIslandConstraintsTask(&island_tasks[t]);
                serial_solved += 1;
            }

            world->last_solver_parallel_islands = parallel_solved;
            world->last_solver_serial_islands = serial_solved;
        } else {
            UpeSolver_PgsIteration(solver_bodies, world->body_capacity, constraints, constraint_count);
            world->last_solver_serial_islands = (island_task_count > 0) ? island_task_count : ((constraint_count > 0) ? 1u : 0u);
        }

        // Solve joint constraints in same solver iteration
        for (uint32_t j = 0; j < world->joint_count; ++j) {
            UpeJoint_Solve(&world->joints[j], solver_bodies, world->body_capacity);
        }

    }

    // Copy back velocities once after all iterations
    for (uint32_t dyn = 0; dyn < active_body_count; ++dyn) {
        uint32_t i = body_indices[dyn];
        world->bodies[i].velocity = solver_bodies[i].velocity;
        world->bodies[i].angular_velocity = solver_bodies[i].angular_velocity;
    }
    UpeWorld_EndStage(world, UPE_STEP_STAGE_SOLVER, constraint_count);
    UpeWorld_BeginStage(world, UPE_STEP_STAGE_INTEGRATION, world->body_count);

    if (world->joint_count > 0) {
        uint32_t write_idx = 0;
        for (uint32_t j = 0; j < world->joint_count; ++j) {
            if (world->joints[j].broken) {
                continue;
            }
            if (write_idx != j) {
                world->joints[write_idx] = world->joints[j];
            }
            write_idx++;
        }
        world->joint_count = write_idx;
    }

    // Save current constraints for next frame's warm-starting
    if (world->enable_warm_starting && constraint_count > 0) {
        if (world->prev_constraints && constraint_count <= world->prev_constraint_capacity) {
            memcpy(world->prev_constraints, constraints, constraint_count * sizeof(UpeSolverConstraint));
            world->prev_constraint_count = constraint_count;
        }
    } else {
        world->prev_constraint_count = 0;
    }

    if (world->profiler) {
        UpeProfiler_EndZone(world->profiler);
        UpeProfiler_BeginZone(world->profiler, "integration");
    }

    uint32_t awake_dynamic_count = active_body_count;
    int has_awake_dynamic_list = 0;
    if (UpeWorld_EnsureScratch((void**)&world->awake_dynamic_ids,
                               &world->awake_dynamic_capacity,
                               active_body_count,
                               sizeof(uint32_t))) {
        if (world->enable_sleeping) {
            uint32_t awake_write = 0;
            for (uint32_t dyn = 0; dyn < active_body_count; ++dyn) {
                uint32_t i = body_indices[dyn];
                uint32_t island_id = world->body_island_ids[i];
                if (island_id < world->island_count && !world->islands[island_id].awake) {
                    continue;
                }
                world->awake_dynamic_ids[awake_write++] = i;
            }
            world->awake_dynamic_count = awake_write;
            awake_dynamic_count = awake_write;
        } else {
            if (active_body_count > 0) {
                memcpy(world->awake_dynamic_ids, body_indices, active_body_count * sizeof(uint32_t));
            }
            world->awake_dynamic_count = active_body_count;
            awake_dynamic_count = active_body_count;
        }
        has_awake_dynamic_list = 1;
    }

    // 6. Integrate positions after constraint solving (only awake bodies)
    if (has_awake_dynamic_list) {
        for (uint32_t dyn = 0; dyn < awake_dynamic_count; ++dyn) {
            uint32_t i = world->awake_dynamic_ids[dyn];
            UpeIntegrator_SemiImplicit(&world->bodies[i], world->fixed_dt, world->gravity);
            UpeBody_ClearForces(&world->bodies[i]);
        }
    } else {
        for (uint32_t dyn = 0; dyn < active_body_count; ++dyn) {
            uint32_t i = body_indices[dyn];
            uint32_t island_id = world->body_island_ids[i];
            if (world->enable_sleeping && island_id < world->island_count && !world->islands[island_id].awake) {
                continue;
            }
            UpeIntegrator_SemiImplicit(&world->bodies[i], world->fixed_dt, world->gravity);
            UpeBody_ClearForces(&world->bodies[i]);
        }
    }
    
    // Calculate energy for diagnostics
    world->total_kinetic_energy = 0.0;
    world->total_potential_energy = 0.0;
    for (uint32_t dyn = 0; dyn < active_body_count; ++dyn) {
        uint32_t i = body_indices[dyn];
        UpeBody *body = &world->bodies[i];
        double mass = (double)body->mass;
        
        // Kinetic energy: (1/2) * m * v^2 + (1/2) * I * omega^2
        float speed_sq = UpeVec3_Dot(body->velocity, body->velocity);
        world->total_kinetic_energy += 0.5 * mass * speed_sq;
        
        float angular_speed_sq = UpeVec3_Dot(body->angular_velocity, body->angular_velocity);
        // Simple I ≈ (2/5) * m * r^2 for rough equivalent
        world->total_kinetic_energy += 0.2 * mass * angular_speed_sq;
        
        // Potential energy: m * g * h (relative to y=0)
        world->total_potential_energy += mass * world->gravity.y * body->position.y;
    }

    UpeWorld_EndStage(world, UPE_STEP_STAGE_INTEGRATION, world->body_count);
    UpeWorld_BeginStage(world, UPE_STEP_STAGE_POST_STEP, world->contact_count);
    
    if (world->memory) {
        UpeMemoryContext_FinalizeFrame(world->memory, world->fixed_dt * 1000.0f);
    }
    world->time += world->fixed_dt;
    world->frame_index += 1u;
    {
        UpeStateHash state_hash = UpeDeterminism_HashState(world);
        world->last_state_hash = state_hash.state;
        if (world->replay && world->replay->recording) {
            UpeEvent evt = {};
            evt.frame = world->frame_index;
            evt.type = UPE_EVENT_CUSTOM;
            evt.entity_id = 0xFFFFFFFFu;
            evt.data[0] = UpeWorld_BitCastU32ToF32((uint32_t)(world->last_state_hash & 0xFFFFFFFFu));
            evt.data[1] = UpeWorld_BitCastU32ToF32((uint32_t)(world->last_state_hash >> 32));
            evt.data[2] = UpeWorld_BitCastU32ToF32((uint32_t)(world->frame_index & 0xFFFFFFFFu));
            evt.data[3] = UpeWorld_BitCastU32ToF32((uint32_t)(world->frame_index >> 32));
            UpeReplay_RecordEvent(world->replay, &evt);
        }
    }
    world->query_bvh_dirty = 1;
    if (!using_frame_arena) {
        world->contacts = nullptr;
    }
    if (world->profiler) {
        UpeProfiler_EndZone(world->profiler);
        UpeProfiler_EndFrame(world->profiler);
    }
    UpeWorld_RecordTelemetry(world);
    UpeWorld_EndStage(world, UPE_STEP_STAGE_POST_STEP, world->contact_count);
}

float UpeWorld_Time(const UpeWorld *world) {
    return world ? world->time : 0.0f;
}

void UpeWorld_AttachReplayRecorder(UpeWorld *world, struct UpeReplayRecorder *replay, int enable_apply) {
    if (!world) {
        return;
    }
    world->replay = replay;
    world->replay_apply_enabled = (enable_apply != 0) ? 1 : 0;
}

struct UpeReplayRecorder *UpeWorld_GetReplayRecorder(const UpeWorld *world) {
    if (!world) {
        return nullptr;
    }
    return world->replay;
}

uint64_t UpeWorld_GetFrameIndex(const UpeWorld *world) {
    if (!world) {
        return 0;
    }
    return world->frame_index;
}

uint64_t UpeWorld_GetLastStateHash(const UpeWorld *world) {
    if (!world) {
        return 0;
    }
    return world->last_state_hash;
}

void UpeWorld_SetStepStageCallback(UpeWorld *world, UpeStepStageCallback callback, void *user_data) {
    if (!world) {
        return;
    }
    world->step_stage_callback = callback;
    world->step_stage_user_data = user_data;
}

uint32_t UpeWorld_GetLastStepStageStats(const UpeWorld *world, UpeStepStageStat *out_stats, uint32_t max_stats) {
    if (!world) {
        return 0;
    }
    uint32_t available = world->last_step_stats_count;
    if (!out_stats || max_stats == 0) {
        return available;
    }
    uint32_t to_copy = (max_stats < available) ? max_stats : available;
    memcpy(out_stats, world->last_step_stats, to_copy * sizeof(UpeStepStageStat));
    return to_copy;
}

int UpeWorld_GetStepMetrics(const UpeWorld *world, UpeWorldStepMetrics *out_metrics) {
    if (!world || !out_metrics) {
        return 0;
    }

    out_metrics->frame_index = world->frame_index;
    out_metrics->state_hash = world->last_state_hash;
    out_metrics->active_dynamic_bodies = world->last_active_body_count;
    out_metrics->broadphase_pairs = world->last_broadphase_pair_count;
    out_metrics->contacts = world->last_contact_count;
    out_metrics->constraints = world->last_constraint_count;
    out_metrics->islands = world->last_island_count;
    out_metrics->solver_iterations = world->last_solver_iterations;
    out_metrics->solver_threads = world->solver_thread_count;
    out_metrics->solver_island_tasks = world->last_solver_island_task_count;
    out_metrics->solver_parallel_islands = world->last_solver_parallel_islands;
    out_metrics->solver_serial_islands = world->last_solver_serial_islands;
    out_metrics->ccd_candidate_pairs = world->ccd_candidate_pairs;
    out_metrics->ccd_tested_pairs = world->ccd_tested_pairs;
    out_metrics->ccd_contact_hits = world->ccd_contact_hits;
    return 1;
}
