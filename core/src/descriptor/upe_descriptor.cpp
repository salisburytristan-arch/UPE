#include "upe/upe_descriptor.h"
#include <cmath>

UpeWorldDesc UpeWorldDesc_Default(void) {
    UpeWorldDesc desc = {};
    desc.fixed_dt = 1.0f / 60.0f;
    desc.gravity[0] = 0.0f;
    desc.gravity[1] = -9.81f;
    desc.gravity[2] = 0.0f;
    desc.solver_algorithm = UPE_SOLVER_PGS;
    desc.solver_iterations = 4;
    desc.solver_friction_iterations = 1;
    desc.slider_bias = 0.05f;
    desc.restitution_threshold = 0.5f;
    desc.broadphase_algorithm = UPE_BROADPHASE_SAP;
    desc.broadphase_grid_cells = 32;
    desc.max_contacts = 4096;
    desc.max_pairs = 16384;
    desc.ccd_threshold = 10.0f;
    desc.ccd_enabled = 0;
    desc.enable_determinism_mode = 0;
    desc.determinism_epsilon = 1e-6f;
    desc.thread_pool_size = 0.0f;
    desc.contact_persistence_frames = 4;
    desc.max_temp_memory = 16 * 1024 * 1024;
    desc.enable_sleeping = 1;
    desc.enable_ccd = 0;
    desc.enable_warm_starting = 1;
    desc.enable_manifold_caching = 1;
    return desc;
}

UpeError UpeWorldDesc_Validate(const UpeWorldDesc *desc) {
    if (desc == nullptr) {
        return UPE_ERR_INVALID_ARGUMENT;
    }

    if (desc->fixed_dt <= 0.0f || desc->fixed_dt > 1.0f) {
        return UPE_ERR_INVALID_ARGUMENT;
    }

    if (std::isnan(desc->gravity[0]) || std::isnan(desc->gravity[1]) || std::isnan(desc->gravity[2])) {
        return UPE_ERR_INVALID_ARGUMENT;
    }

    if (desc->solver_iterations == 0) {
        return UPE_ERR_INVALID_ARGUMENT;
    }

    if (desc->max_contacts == 0 || desc->max_pairs == 0) {
        return UPE_ERR_INVALID_ARGUMENT;
    }

    if (desc->slider_bias < 0.0f || desc->slider_bias > 1.0f) {
        return UPE_ERR_INVALID_ARGUMENT;
    }

    return UPE_OK;
}

UpeSolverDesc UpeSolverDesc_Default(void) {
    UpeSolverDesc desc = {};
    desc.algorithm = UPE_SOLVER_PGS;
    desc.iterations = 4;
    desc.friction_iterations = 1;
    desc.bias = 0.05f;
    desc.restitution_threshold = 0.5f;
    desc.damping = 0.1f;
    desc.enable_warm_starting = 1;
    return desc;
}

UpeError UpeSolverDesc_Validate(const UpeSolverDesc *desc) {
    if (desc == nullptr) {
        return UPE_ERR_INVALID_ARGUMENT;
    }

    if (desc->iterations == 0) {
        return UPE_ERR_INVALID_ARGUMENT;
    }

    if (desc->bias < 0.0f || desc->bias > 1.0f) {
        return UPE_ERR_INVALID_ARGUMENT;
    }

    return UPE_OK;
}

UpeBodyDesc_Ex UpeBodyDesc_ExDefault(void) {
    UpeBodyDesc_Ex desc = {};
    desc.position[0] = 0.0f;
    desc.position[1] = 0.0f;
    desc.position[2] = 0.0f;
    desc.rotation[0] = 0.0f;
    desc.rotation[1] = 0.0f;
    desc.rotation[2] = 0.0f;
    desc.rotation[3] = 1.0f;
    desc.linear_velocity[0] = 0.0f;
    desc.linear_velocity[1] = 0.0f;
    desc.linear_velocity[2] = 0.0f;
    desc.angular_velocity[0] = 0.0f;
    desc.angular_velocity[1] = 0.0f;
    desc.angular_velocity[2] = 0.0f;
    desc.mass = 1.0f;
    desc.inertia_scale = 1.0f;
    desc.body_type = 0;
    desc.collision_layer = 1;
    desc.collision_mask = 0xFFFFFFFFu;
    return desc;
}

UpeError UpeBodyDesc_ExValidate(const UpeBodyDesc_Ex *desc) {
    if (desc == nullptr) {
        return UPE_ERR_INVALID_ARGUMENT;
    }

    if (std::isnan(desc->position[0]) || std::isnan(desc->position[1]) || std::isnan(desc->position[2])) {
        return UPE_ERR_INVALID_ARGUMENT;
    }

    if (std::isnan(desc->rotation[0]) || std::isnan(desc->rotation[1]) ||
        std::isnan(desc->rotation[2]) || std::isnan(desc->rotation[3])) {
        return UPE_ERR_INVALID_ARGUMENT;
    }

    if (desc->body_type == 0 && desc->mass <= 0.0f) {
        return UPE_ERR_INVALID_ARGUMENT;
    }

    if (desc->body_type < 0 || desc->body_type > 2) {
        return UPE_ERR_INVALID_ARGUMENT;
    }

    if (desc->body_type == 1 && desc->mass != 0.0f) {
        return UPE_ERR_INVALID_ARGUMENT;
    }

    return UPE_OK;
}
