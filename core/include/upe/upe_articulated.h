#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

// Articulated body: collection of rigid bodies connected by joints
// Useful for ragdolls, skeletons, linkages, robots
typedef struct UpeArticulatedLink {
    uint32_t body_id;           // Body ID in world
    uint32_t parent_link;       // Index of parent link (-1 if root)
    uint32_t parent_joint_id;   // Joint connecting to parent
    UpeVec3 local_com;          // Center of mass relative to parent
} UpeArticulatedLink;

typedef struct UpeArticulatedBody {
    uint32_t id;                // Articulated body ID
    UpeArticulatedLink *links;  // Array of links
    uint32_t link_count;        // Number of links
    uint32_t root_link;         // Index of root link
    
    // Optional: cache for fast updates
    uint8_t *link_alive;        // Mark alive/dead links
    UpeVec3 *link_positions;    // Cached world-space positions
} UpeArticulatedBody;

typedef struct UpeArticulatedDesc {
    uint32_t link_count;
    UpeArticulatedLink *links;
    uint32_t root_link;
} UpeArticulatedDesc;

// Create articulated body from description
UpeArticulatedBody *UpeArticulatedBody_Create(struct UpeWorld *world, const UpeArticulatedDesc *desc);

// Destroy articulated body and its links
void UpeArticulatedBody_Destroy(struct UpeWorld *world, UpeArticulatedBody *body);

// Get world-space position of a link
UpeVec3 UpeArticulatedBody_GetLinkPosition(UpeArticulatedBody *body, uint32_t link_idx,
                                          struct UpeWorld *world);

// Apply impulse to articulated body link
void UpeArticulatedBody_ApplyLinkImpulse(UpeArticulatedBody *body, uint32_t link_idx,
                                        UpeVec3 impulse, struct UpeWorld *world);

// Set root body velocity (propagates to children based on constraints)
void UpeArticulatedBody_SetRootVelocity(UpeArticulatedBody *body, UpeVec3 velocity,
                                       struct UpeWorld *world);

#ifdef __cplusplus
}
#endif

