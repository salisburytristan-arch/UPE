#include <stdlib.h>
#include <string.h>
#include "upe/upe_articulated.h"
#include "upe/upe_body.h"
#include "upe/upe_joint.h"
#include "upe/upe_world_body_api.h"

// Create articulated body
UpeArticulatedBody *UpeArticulatedBody_Create(UpeWorld *world, const UpeArticulatedDesc *desc)
{
    if (!desc || desc->link_count == 0) {
        return NULL;
    }

    UpeArticulatedBody *artbody = (UpeArticulatedBody *)malloc(sizeof(UpeArticulatedBody));
    if (!artbody) return NULL;

    artbody->id = (uint32_t)((uintptr_t)artbody / sizeof(UpeArticulatedBody)); // Simple ID
    artbody->root_link = desc->root_link;

    // Allocate link array
    artbody->link_count = desc->link_count;
    artbody->links = (UpeArticulatedLink *)malloc(sizeof(UpeArticulatedLink) * desc->link_count);
    if (!artbody->links) {
        free(artbody);
        return NULL;
    }

    // Copy link descriptors
    memcpy(artbody->links, desc->links, sizeof(UpeArticulatedLink) * desc->link_count);

    // Allocate alive tracking and position caching
    artbody->link_alive = (uint8_t *)calloc(desc->link_count, sizeof(uint8_t));
    artbody->link_positions = (UpeVec3 *)calloc(desc->link_count, sizeof(UpeVec3));

    if (!artbody->link_alive || !artbody->link_positions) {
        free(artbody->links);
        free(artbody->link_alive);
        free(artbody->link_positions);
        free(artbody);
        return NULL;
    }

    // Mark all links as alive
    for (uint32_t i = 0; i < desc->link_count; i++) {
        artbody->link_alive[i] = 1;
    }

    // If a world was provided, ensure bodies exist and add joints
    if (world) {
        for (uint32_t i = 0; i < artbody->link_count; ++i) {
            UpeArticulatedLink *link = &artbody->links[i];
            // If the descriptor supplied a valid body id, verify it; otherwise create one.
            if (link->body_id == (uint32_t)-1 || UpeWorld_GetBody(world, link->body_id) == NULL) {
                UpeBodyDesc bd;
                bd.type = UPE_BODY_DYNAMIC;
                bd.position = {0.0f, 0.0f, 0.0f};
                bd.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
                bd.mass = 1.0f;
                bd.collision_layer = 1;
                bd.collision_mask = 0xFFFFFFFF;
                uint32_t new_id = UpeWorld_CreateBody(world, &bd);
                link->body_id = new_id;
            }
        }

        /* create joints for each non-root link */
        for (uint32_t i = 0; i < artbody->link_count; ++i) {
            UpeArticulatedLink *link = &artbody->links[i];
            if (link->parent_link != (uint32_t)-1 && link->parent_link < artbody->link_count) {
                UpeJointDesc jdesc = {};
                jdesc.type = UPE_JOINT_BALL;
                jdesc.body_a = artbody->links[link->parent_link].body_id;
                jdesc.body_b = link->body_id;
                /* anchors roughly at the link's local center-of-mass */
                jdesc.anchor_a = link->local_com;
                jdesc.anchor_b.x = 0.0f;
                jdesc.anchor_b.y = 0.0f;
                jdesc.anchor_b.z = 0.0f;
                UpeWorld_AddJoint(world, &jdesc);
                /* store the joint id in parent_joint_id for reference */
                link->parent_joint_id = UpeWorld_GetJointCount(world) - 1u;
            }
        }
    }

    return artbody;
}

// Destroy articulated body
void UpeArticulatedBody_Destroy(UpeWorld *world, UpeArticulatedBody *body)
{
    if (!body) return;

    (void)world;

    // Note: Actual destruction of world bodies/joints should be handled
    // by the world, not here. This function just frees the articulated
    // body metadata.

    if (body->links) {
        free(body->links);
        body->links = NULL;
    }

    if (body->link_alive) {
        free(body->link_alive);
        body->link_alive = NULL;
    }

    if (body->link_positions) {
        free(body->link_positions);
        body->link_positions = NULL;
    }

    free(body);
}

// Get world-space position of a link
UpeVec3 UpeArticulatedBody_GetLinkPosition(UpeArticulatedBody *body, uint32_t link_idx,
                                          UpeWorld *world)
{
    UpeVec3 zero = {0.0f, 0.0f, 0.0f};
    if (!body || link_idx >= body->link_count) {
        return zero;
    }
    if (!body->link_alive[link_idx]) {
        return zero;
    }
    if (world) {
        UpeBody *b = UpeWorld_GetBody(world, body->links[link_idx].body_id);
        if (b) {
            return b->position;
        }
    }
    return body->link_positions[link_idx];
}

// Apply impulse to link
void UpeArticulatedBody_ApplyLinkImpulse(UpeArticulatedBody *body, uint32_t link_idx,
                                        UpeVec3 impulse, UpeWorld *world)
{
    if (!body || link_idx >= body->link_count || !world) {
        return;
    }

    if (!body->link_alive[link_idx]) {
        return;
    }

    UpeWorld_ApplyImpulse(world, body->links[link_idx].body_id, impulse);
}

// Set root velocity
void UpeArticulatedBody_SetRootVelocity(UpeArticulatedBody *body, UpeVec3 velocity,
                                       UpeWorld *world)
{
    if (!body || body->root_link >= body->link_count || !world) {
        return;
    }

    uint32_t root_id = body->links[body->root_link].body_id;
    UpeBody *b = UpeWorld_GetBody(world, root_id);
    if (b) {
        b->velocity = velocity;
    }
    // optionally propagate to others if needed; solver will enforce constraints
}

