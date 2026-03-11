#include "upe/upe_animation.h"
#include "upe/upe_math.h"
#include "upe/upe_world_body_api.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>

typedef struct {
    UpeAnimationController *controller;
    UpeIKConstraint constraints[32];
    uint32_t constraint_count;
} UpeControllerIkState;

static std::vector<UpeControllerIkState> g_ik_states;

static UpeControllerIkState *UpeAnimation_GetIkState(UpeAnimationController *controller, int create_if_missing) {
    if (!controller) {
        return NULL;
    }
    for (size_t i = 0; i < g_ik_states.size(); ++i) {
        if (g_ik_states[i].controller == controller) {
            return &g_ik_states[i];
        }
    }
    if (!create_if_missing) {
        return NULL;
    }
    UpeControllerIkState state = {};
    state.controller = controller;
    state.constraint_count = 0;
    g_ik_states.push_back(state);
    return &g_ik_states[g_ik_states.size() - 1];
}

static void UpeAnimation_RemoveIkState(UpeAnimationController *controller) {
    for (size_t i = 0; i < g_ik_states.size(); ++i) {
        if (g_ik_states[i].controller == controller) {
            g_ik_states[i] = g_ik_states[g_ik_states.size() - 1];
            g_ik_states.pop_back();
            return;
        }
    }
}

static float UpeAnimation_Clamp(float value, float min_v, float max_v) {
    if (value < min_v) return min_v;
    if (value > max_v) return max_v;
    return value;
}

static UpeQuat UpeAnimation_QuatNormalize(UpeQuat q) {
    float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len < 1.0e-8f) {
        return UpeQuat_Identity();
    }
    float inv = 1.0f / len;
    UpeQuat out;
    out.x = q.x * inv;
    out.y = q.y * inv;
    out.z = q.z * inv;
    out.w = q.w * inv;
    return out;
}

static UpeQuat UpeAnimation_QuatFromAxisAngle(UpeVec3 axis, float angle) {
    float axis_len = UpeVec3_Length(axis);
    if (axis_len < 1.0e-8f || fabsf(angle) < 1.0e-8f) {
        return UpeQuat_Identity();
    }
    UpeVec3 n = UpeVec3_Scale(axis, 1.0f / axis_len);
    float half = 0.5f * angle;
    float s = sinf(half);
    UpeQuat q;
    q.x = n.x * s;
    q.y = n.y * s;
    q.z = n.z * s;
    q.w = cosf(half);
    return UpeAnimation_QuatNormalize(q);
}

/*================================================
  SKELETON MANAGEMENT
================================================*/

UpeSkeleton* UpeSkeleton_Create(uint32_t max_bones) {
    UpeSkeleton *skeleton = (UpeSkeleton*)malloc(sizeof(UpeSkeleton));
    if (!skeleton) return NULL;
    
    skeleton->skeleton_id = 0;
    skeleton->bone_count = 0;
    skeleton->root_bone_id = UINT32_MAX;
    
    memset(skeleton->bones, 0, sizeof(skeleton->bones));
    memset(skeleton->bone_world_positions, 0, sizeof(skeleton->bone_world_positions));
    memset(skeleton->bone_world_rotations, 0, sizeof(skeleton->bone_world_rotations));
    
    return skeleton;
}

void UpeSkeleton_Destroy(UpeSkeleton *skeleton) {
    if (skeleton) {
        free(skeleton);
    }
}

uint32_t UpeSkeleton_AddBone(UpeSkeleton *skeleton, const char *name, 
                              uint32_t parent_id, float length) {
    if (!skeleton || skeleton->bone_count >= UPE_MAX_BONES) {
        return UINT32_MAX;
    }
    
    uint32_t bone_id = skeleton->bone_count;
    UpeBone *bone = &skeleton->bones[bone_id];
    
    bone->bone_id = bone_id;
    strncpy(bone->name, name, 63);
    bone->name[63] = '\0';
    bone->parent_id = parent_id;
    bone->body_id = UINT32_MAX;
    bone->length = length;
    bone->flags = 0;
    bone->blend_weight = 0.0f;
    bone->blend_mode = UPE_BLEND_REPLACE;
    
    /* Initialize local transforms */
    bone->local_rotation.x = 0.0f;
    bone->local_rotation.y = 0.0f;
    bone->local_rotation.z = 0.0f;
    bone->local_rotation.w = 1.0f;
    bone->local_position.x = 0.0f;
    bone->local_position.y = 0.0f;
    bone->local_position.z = 0.0f;
    
    /* Set root if no parent */
    if (parent_id == UINT32_MAX) {
        skeleton->root_bone_id = bone_id;
        bone->flags |= UPE_BONE_FLAG_ROOT;
    }
    
    skeleton->bone_count++;
    return bone_id;
}

/*================================================
  ANIMATION CLIP MANAGEMENT
================================================*/

UpeAnimationClip* UpeAnimationClip_Create(const char *name, float duration) {
    UpeAnimationClip *clip = (UpeAnimationClip*)malloc(sizeof(UpeAnimationClip));
    if (!clip) return NULL;
    
    strncpy(clip->name, name, 63);
    clip->name[63] = '\0';
    clip->duration = duration;
    clip->track_count = 0;
    clip->flags = 0;
    clip->tracks = NULL;
    
    return clip;
}

void UpeAnimationClip_Destroy(UpeAnimationClip *clip) {
    if (clip) {
        if (clip->tracks) {
            for (uint32_t i = 0; i < clip->track_count; i++) {
                if (clip->tracks[i].keyframes) {
                    free(clip->tracks[i].keyframes);
                }
            }
            free(clip->tracks);
        }
        free(clip);
    }
}

void UpeAnimationClip_AddTrack(UpeAnimationClip *clip, uint32_t bone_id,
                                const UpeKeyframe *keyframes, uint32_t count) {
    if (!clip || !keyframes || count == 0) return;
    
    /* Resize tracks array */
    UpeAnimationTrack *new_tracks = (UpeAnimationTrack*)realloc(clip->tracks,
        (clip->track_count + 1) * sizeof(UpeAnimationTrack));
    if (!new_tracks) return;
    
    clip->tracks = new_tracks;
    UpeAnimationTrack *track = &clip->tracks[clip->track_count];
    track->bone_id = bone_id;
    track->keyframe_count = count;
    track->duration = clip->duration;
    
    /* Copy keyframes */
    track->keyframes = (UpeKeyframe*)malloc(count * sizeof(UpeKeyframe));
    if (track->keyframes) {
        memcpy(track->keyframes, keyframes, count * sizeof(UpeKeyframe));
    }
    
    clip->track_count++;
}

/*================================================
  ANIMATION CONTROLLER
================================================*/

UpeAnimationController* UpeAnimationController_Create(UpeSkeleton *skeleton) {
    if (!skeleton) return NULL;
    
    UpeAnimationController *controller = (UpeAnimationController*)
        malloc(sizeof(UpeAnimationController));
    if (!controller) return NULL;
    
    controller->controller_id = 0;
    controller->skeleton = skeleton;
    controller->animation_count = 0;
    controller->ragdoll_blend = 0.0f;
    controller->enable_physics_influence = true;
    
    memset(controller->animations, 0, sizeof(controller->animations));
    
    /* Initialize playback */
    controller->playback.current_animation_id = UINT32_MAX;
    controller->playback.playback_time = 0.0f;
    controller->playback.state = UPE_ANIM_STATE_STOPPED;
    controller->playback.playback_speed = 1.0f;
    controller->playback.blend_target_animation = UINT32_MAX;
    controller->playback.blend_duration = 0.0f;
    controller->playback.blend_start_time = 0.0f;
    
    memset(controller->playback.bone_blend_weights, 0, sizeof(controller->playback.bone_blend_weights));
    
    return controller;
}

void UpeAnimationController_Destroy(UpeAnimationController *controller) {
    if (controller) {
        UpeAnimation_RemoveIkState(controller);
        for (uint32_t i = 0; i < controller->animation_count; i++) {
            UpeAnimationClip_Destroy(controller->animations[i]);
        }
        free(controller);
    }
}

/*================================================
  PLAYBACK CONTROL
================================================*/

void UpeAnimationController_PlayAnimation(UpeAnimationController *controller,
                                           UpeAnimationClip *clip, bool loop) {
    if (!controller || !clip) return;
    
    controller->playback.current_animation_id = controller->animation_count;
    controller->playback.playback_time = 0.0f;
    controller->playback.state = loop ? UPE_ANIM_STATE_LOOPING : UPE_ANIM_STATE_PLAYING;
    controller->playback.blend_target_animation = UINT32_MAX;
    
    /* Store animation if not already there */
    if (controller->animation_count < UPE_MAX_ANIMATIONS) {
        controller->animations[controller->animation_count] = clip;
        controller->animation_count++;
    }
}

void UpeAnimationController_BlendAnimation(UpeAnimationController *controller,
                                            UpeAnimationClip *target_clip, float duration) {
    if (!controller || !target_clip) return;
    
    controller->playback.blend_target_animation = controller->animation_count;
    controller->playback.blend_duration = duration;
    controller->playback.blend_start_time = 0.0f;
    
    if (controller->animation_count < UPE_MAX_ANIMATIONS) {
        controller->animations[controller->animation_count] = target_clip;
        controller->animation_count++;
    }
}

void UpeAnimationController_Stop(UpeAnimationController *controller) {
    if (!controller) return;
    controller->playback.state = UPE_ANIM_STATE_STOPPED;
    controller->playback.playback_time = 0.0f;
}

void UpeAnimationController_SetPlaybackSpeed(UpeAnimationController *controller, float speed) {
    if (controller) {
        controller->playback.playback_speed = speed > 0.0f ? speed : 1.0f;
    }
}

/*================================================
  ANIMATION SAMPLING
================================================*/

static void UpeAnimationTrack_SampleKeyframe(const UpeAnimationTrack *track, float time,
                                              UpeQuat *out_rotation, UpeVec3 *out_translation) {
    if (!track || !out_rotation || !out_translation) return;
    if (track->keyframe_count == 0 || !track->keyframes) return;
    
    /* Clamp time */
    time = fmaxf(0.0f, fminf(time, track->duration));
    
    /* Find surrounding keyframes */
    int before_idx = 0;
    int after_idx = (int)track->keyframe_count - 1;
    for (uint32_t i = 0; i < track->keyframe_count; i++) {
        if (track->keyframes[i].time <= time) {
            before_idx = i;
        }
        if (track->keyframes[i].time >= time) {
            after_idx = i;
            break;
        }
    }
    
    /* Handle edge cases */
    if (before_idx == after_idx || track->keyframe_count == 1) {
        *out_rotation = track->keyframes[before_idx].rotation;
        *out_translation = track->keyframes[before_idx].translation;
        return;
    }
    
    /* Linear interpolation */
    float t0 = track->keyframes[before_idx].time;
    float t1 = track->keyframes[after_idx].time;
    float alpha = (t1 > t0) ? ((time - t0) / (t1 - t0)) : 0.0f;
    alpha = fmaxf(0.0f, fminf(1.0f, alpha));
    
    /* Interpolate rotation (SLERP) */
    *out_rotation = UpeQuat_Slerp(track->keyframes[before_idx].rotation,
                                   track->keyframes[after_idx].rotation, alpha);
    
    /* Interpolate translation (LERP) */
    out_translation->x = track->keyframes[before_idx].translation.x * (1.0f - alpha) +
                         track->keyframes[after_idx].translation.x * alpha;
    out_translation->y = track->keyframes[before_idx].translation.y * (1.0f - alpha) +
                         track->keyframes[after_idx].translation.y * alpha;
    out_translation->z = track->keyframes[before_idx].translation.z * (1.0f - alpha) +
                         track->keyframes[after_idx].translation.z * alpha;
}

/*================================================
  WORLD TRANSFORM COMPUTATION
================================================*/

void UpeSkeleton_UpdateWorldTransforms(UpeSkeleton *skeleton) {
    /* Start with root bone */
    if (skeleton->root_bone_id != UINT32_MAX) {
        UpeBone *root = &skeleton->bones[skeleton->root_bone_id];
        skeleton->bone_world_positions[skeleton->root_bone_id] = root->local_position;
        skeleton->bone_world_rotations[skeleton->root_bone_id] = root->local_rotation;
    }
    
    /* Forward kinematics: update each bone based on parent */
    for (uint32_t i = 0; i < skeleton->bone_count; i++) {
        UpeBone *bone = &skeleton->bones[i];
        
        if (bone->parent_id != UINT32_MAX) {
            UpeVec3 parent_pos = skeleton->bone_world_positions[bone->parent_id];
            UpeQuat parent_rot = skeleton->bone_world_rotations[bone->parent_id];
            
            /* Rotate local position by parent rotation */
            UpeVec3 rotated_pos = UpeQuat_RotateVec3(parent_rot, bone->local_position);
            
            /* World position = parent_pos + rotated_pos */
            skeleton->bone_world_positions[i] = UpeVec3_Add(parent_pos, rotated_pos);
            
            /* World rotation = parent_rot * local_rot */
            skeleton->bone_world_rotations[i] = UpeQuat_Multiply(parent_rot, bone->local_rotation);
        }
    }
}

/*================================================
  BLENDING FUNCTIONS
================================================*/

static UpeQuat UpeQuat_Blend(UpeQuat a, UpeQuat b, float t) {
    return UpeQuat_Slerp(a, b, t);
}

static UpeVec3 UpeVec3_Blend(UpeVec3 a, UpeVec3 b, float t) {
    UpeVec3 result;
    result.x = a.x * (1.0f - t) + b.x * t;
    result.y = a.y * (1.0f - t) + b.y * t;
    result.z = a.z * (1.0f - t) + b.z * t;
    return result;
}

/*================================================
  UPDATE LOOP
================================================*/

void UpeAnimationController_Update(UpeAnimationController *controller, float dt,
                                    UpeWorld *physics_world) {
    if (!controller || !controller->skeleton) return;
    
    /* Update playback time */
    if (controller->playback.state == UPE_ANIM_STATE_PLAYING ||
        controller->playback.state == UPE_ANIM_STATE_LOOPING) {
        
        controller->playback.playback_time += dt * controller->playback.playback_speed;
        
        if (controller->playback.current_animation_id < controller->animation_count) {
            UpeAnimationClip *current_clip = controller->animations[controller->playback.current_animation_id];
            
            if (controller->playback.playback_time > current_clip->duration) {
                if (controller->playback.state == UPE_ANIM_STATE_LOOPING) {
                    controller->playback.playback_time = fmodf(controller->playback.playback_time,
                                                                current_clip->duration);
                } else {
                    controller->playback.state = UPE_ANIM_STATE_STOPPED;
                }
            }
        }
    }
    
    /* Update blend animation if active */
    if (controller->playback.blend_target_animation != UINT32_MAX &&
        controller->playback.blend_duration > 0.0f) {
        controller->playback.blend_start_time += dt;
        
        if (controller->playback.blend_start_time >= controller->playback.blend_duration) {
            /* Blend complete, switch to target */
            controller->playback.current_animation_id = controller->playback.blend_target_animation;
            controller->playback.blend_target_animation = UINT32_MAX;
            controller->playback.playback_time = 0.0f;
        }
    }
    
    /* Sample animation into skeleton */
    if (controller->playback.current_animation_id < controller->animation_count) {
        UpeAnimationClip *current_clip = controller->animations[controller->playback.current_animation_id];
        
        for (uint32_t i = 0; i < current_clip->track_count; i++) {
            UpeAnimationTrack *track = &current_clip->tracks[i];
            if (track->bone_id >= controller->skeleton->bone_count) continue;
            
            UpeBone *bone = &controller->skeleton->bones[track->bone_id];
            
            UpeQuat sampled_rot = {0.0f, 0.0f, 0.0f, 1.0f};
            UpeVec3 sampled_pos = {0.0f, 0.0f, 0.0f};
            UpeAnimationTrack_SampleKeyframe(track, controller->playback.playback_time,
                                              &sampled_rot, &sampled_pos);
            
            /* Blend with physics if enabled */
            float blend_weight = bone->blend_weight;
            
            if (blend_weight > 0.0f) {
                bone->local_rotation = UpeQuat_Blend(bone->local_rotation, sampled_rot, blend_weight);
                bone->local_position = UpeVec3_Blend(bone->local_position, sampled_pos, blend_weight);
            }
        }
    }
    
    /* Update world transforms */
    UpeSkeleton_UpdateWorldTransforms(controller->skeleton);
    
    /* Apply ragdoll blend if physics bodies exist */
    if (controller->ragdoll_blend > 0.0f && physics_world) {
        for (uint32_t i = 0; i < controller->skeleton->bone_count; i++) {
            UpeBone *bone = &controller->skeleton->bones[i];
            if (bone->body_id == UINT32_MAX) continue;
            
            UpeBody *physics_body = UpeWorld_GetBody(physics_world, bone->body_id);
            if (!physics_body) continue;
            
            /* Blend animation pose with physics pose */
            UpeVec3 blended_pos = UpeVec3_Blend(controller->skeleton->bone_world_positions[i],
                                                physics_body->position, controller->ragdoll_blend);
            UpeQuat blended_rot = UpeQuat_Blend(controller->skeleton->bone_world_rotations[i],
                                                physics_body->rotation, controller->ragdoll_blend);
            
            controller->skeleton->bone_world_positions[i] = blended_pos;
            controller->skeleton->bone_world_rotations[i] = blended_rot;
        }
    }
}

/*================================================
  BLENDING API
================================================*/

void UpeAnimationController_SetRagdollBlend(UpeAnimationController *controller, float blend) {
    if (controller) {
        controller->ragdoll_blend = fmaxf(0.0f, fminf(1.0f, blend));
    }
}

void UpeAnimationController_SetBoneBoneBlendWeight(UpeAnimationController *controller,
                                                    uint32_t bone_id, float weight) {
    if (controller && bone_id < controller->skeleton->bone_count) {
        controller->skeleton->bones[bone_id].blend_weight = 
            fmaxf(0.0f, fminf(1.0f, weight));
    }
}

/*================================================
  IK CONSTRAINT SYSTEM (Iterative CCD-style Solver)
  (full implementation replacing earlier stub comments)
================================================*/

void UpeAnimationController_AddIKConstraint(UpeAnimationController *controller,
                                             UpeIKConstraint constraint) {
    if (!controller) return;

    UpeControllerIkState *state = UpeAnimation_GetIkState(controller, 1);
    if (!state) {
        return;
    }

    for (uint32_t i = 0; i < state->constraint_count; ++i) {
        if (state->constraints[i].constraint_id == constraint.constraint_id) {
            state->constraints[i] = constraint;
            return;
        }
    }

    if (state->constraint_count < 32u) {
        state->constraints[state->constraint_count++] = constraint;
    }
}

void UpeAnimationController_SolveIK(UpeAnimationController *controller) {
    if (!controller) return;
    if (!controller->skeleton) {
        return;
    }

    UpeControllerIkState *state = UpeAnimation_GetIkState(controller, 0);
    if (!state || state->constraint_count == 0) {
        return;
    }

    for (uint32_t ci = 0; ci < state->constraint_count; ++ci) {
        UpeIKConstraint *constraint = &state->constraints[ci];
        if (!constraint->enabled || constraint->link_count == 0) {
            continue;
        }

        uint32_t effector_bone = constraint->links[constraint->link_count - 1].bone_id;
        if (effector_bone >= controller->skeleton->bone_count) {
            constraint->solved = false;
            continue;
        }

        uint32_t iterations = constraint->max_iterations > 0 ? constraint->max_iterations : 8u;
        if (iterations > 64u) {
            iterations = 64u;
        }

        float damping = UpeAnimation_Clamp(constraint->damping, 0.01f, 1.0f);
        float weight = UpeAnimation_Clamp(constraint->weight, 0.0f, 1.0f);

        for (uint32_t iter = 0; iter < iterations; ++iter) {
            UpeVec3 effector_pos = controller->skeleton->bone_world_positions[effector_bone];
            UpeVec3 to_target = UpeVec3_Sub(constraint->target_position, effector_pos);
            float error = UpeVec3_Length(to_target);
            constraint->last_error = error;
            if (error <= constraint->target_radius) {
                break;
            }

            for (int32_t li = (int32_t)constraint->link_count - 1; li >= 0; --li) {
                uint32_t bone_id = constraint->links[(uint32_t)li].bone_id;
                if (bone_id >= controller->skeleton->bone_count) {
                    continue;
                }

                UpeBone *bone = &controller->skeleton->bones[bone_id];
                UpeVec3 joint_pos = controller->skeleton->bone_world_positions[bone_id];
                UpeVec3 eff_pos = controller->skeleton->bone_world_positions[effector_bone];

                UpeVec3 cur_vec = UpeVec3_Sub(eff_pos, joint_pos);
                UpeVec3 target_vec = UpeVec3_Sub(constraint->target_position, joint_pos);

                float cur_len = UpeVec3_Length(cur_vec);
                float tgt_len = UpeVec3_Length(target_vec);
                if (cur_len < 1.0e-6f || tgt_len < 1.0e-6f) {
                    continue;
                }

                UpeVec3 cur_dir = UpeVec3_Scale(cur_vec, 1.0f / cur_len);
                UpeVec3 tgt_dir = UpeVec3_Scale(target_vec, 1.0f / tgt_len);

                float cos_theta = UpeAnimation_Clamp(UpeVec3_Dot(cur_dir, tgt_dir), -1.0f, 1.0f);
                float angle = acosf(cos_theta);
                if (angle < 1.0e-5f) {
                    continue;
                }

                UpeVec3 axis = UpeVec3_Cross(cur_dir, tgt_dir);
                if (UpeVec3_Length(axis) < 1.0e-6f) {
                    axis = constraint->pole_vector;
                }

                float delta_angle = angle * damping * weight;
                UpeQuat delta = UpeAnimation_QuatFromAxisAngle(axis, delta_angle);
                bone->local_rotation = UpeAnimation_QuatNormalize(UpeQuat_Mul(delta, bone->local_rotation));
                UpeSkeleton_UpdateWorldTransforms(controller->skeleton);
            }
        }

        UpeVec3 effector_pos = controller->skeleton->bone_world_positions[effector_bone];
        constraint->last_error = UpeVec3_Length(UpeVec3_Sub(constraint->target_position, effector_pos));
        constraint->solved = (constraint->last_error <= constraint->target_radius);
    }
}

/*================================================
  POSE QUERIES
================================================*/

void UpeAnimationController_GetBoneWorldTransform(UpeAnimationController *controller,
                                                   uint32_t bone_id, UpeVec3 *pos, UpeQuat *rot) {
    if (!controller || !controller->skeleton || bone_id >= controller->skeleton->bone_count) {
        return;
    }
    
    if (pos) *pos = controller->skeleton->bone_world_positions[bone_id];
    if (rot) *rot = controller->skeleton->bone_world_rotations[bone_id];
}

void UpeAnimationController_GetBoneWorldPosition(UpeAnimationController *controller,
                                                  uint32_t bone_id, UpeVec3 *pos) {
    if (!controller || !controller->skeleton || !pos || bone_id >= controller->skeleton->bone_count) {
        return;
    }
    
    *pos = controller->skeleton->bone_world_positions[bone_id];
}

/*================================================
  RAGDOLL POSE CAPTURE/APPLY
================================================*/

void UpeAnimationController_CaptureRagdollPose(UpeAnimationController *controller,
                                                UpeWorld *physics_world, UpeRagdollPose *pose) {
    if (!controller || !physics_world || !pose) return;
    
    pose->body_count = 0;
    
    for (uint32_t i = 0; i < controller->skeleton->bone_count; i++) {
        UpeBone *bone = &controller->skeleton->bones[i];
        if (bone->body_id == UINT32_MAX) continue;
        
        UpeBody *physics_body = UpeWorld_GetBody(physics_world, bone->body_id);
        if (!physics_body) continue;
        
        if (pose->body_count < UPE_MAX_BONES) {
            pose->body_positions[pose->body_count] = physics_body->position;
            pose->body_rotations[pose->body_count] = physics_body->rotation;
            pose->body_count++;
        }
    }
}

void UpeAnimationController_ApplyRagdollPose(UpeAnimationController *controller,
                                              UpeWorld *physics_world, const UpeRagdollPose *pose) {
    if (!controller || !physics_world || !pose) return;
    
    uint32_t pose_idx = 0;
    for (uint32_t i = 0; i < controller->skeleton->bone_count && pose_idx < pose->body_count; i++) {
        UpeBone *bone = &controller->skeleton->bones[i];
        if (bone->body_id == UINT32_MAX) continue;
        
        UpeBody *physics_body = UpeWorld_GetBody(physics_world, bone->body_id);
        if (!physics_body) continue;
        
        physics_body->position = pose->body_positions[pose_idx];
        physics_body->rotation = pose->body_rotations[pose_idx];
        physics_body->velocity.x = 0.0f;
        physics_body->velocity.y = 0.0f;
        physics_body->velocity.z = 0.0f;
        physics_body->angular_velocity.x = 0.0f;
        physics_body->angular_velocity.y = 0.0f;
        physics_body->angular_velocity.z = 0.0f;
        
        pose_idx++;
    }
}

/*================================================
  BONE BINDING
================================================*/

void UpeBone_SetPhysicsBody(UpeBone *bone, uint32_t body_id) {
    if (bone) {
        bone->body_id = body_id;
        if (body_id != UINT32_MAX) {
            bone->flags |= UPE_BONE_FLAG_PHYSICAL;
        }
    }
}

uint32_t UpeBone_GetPhysicsBody(UpeBone *bone) {
    if (bone) {
        return bone->body_id;
    }
    return UINT32_MAX;
}
