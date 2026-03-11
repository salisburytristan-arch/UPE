#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Portal Physics (Exotic/Game-specific — Phase 9)

  Implements the mathematics for seamless portal teleportation:
    - Velocity & position transform through portal pair
    - Angular velocity rotation through portal
    - Object clipping plane at portal boundary (half-space test)
    - Continuity of momentum across portals (conservation)
    - Recursive portal depth check
    - Portal camera transform (for rendering — returned as UpeMat4)
    - Audio occlusion toggle based on portal line-of-sound

  APPROXIMATION NOTE: No curved-space semantics. Portals are flat affine
  transforms between two oriented planes.
*/

#define UPE_PORTAL_MAX_PAIRS 32

/* ---- A single portal (one side of a pair) ---- */
typedef struct UpePortal {
    UpeVec3 position;      /* center of portal plane */
    UpeVec3 normal;        /* outward facing normal (into the portal) */
    UpeVec3 up;            /* portal "up" direction (for roll matching) */
    float   width;
    float   height;
    uint32_t id;
    uint32_t linked_id;    /* partner portal ID */
    int      active;
} UpePortal;

/* ---- Portal transform result ---- */
typedef struct UpePortalTransfer {
    UpeVec3 new_position;
    UpeVec3 new_velocity;
    UpeVec3 new_angular_vel;
    UpeMat3 new_orientation; /* new basis rotation */
    int     transferred;     /* 1 if object crossed and was teleported */
} UpePortalTransfer;

/* --- Init a portal --- */
void UpePortal_Init(UpePortal *portal, uint32_t id, UpeVec3 pos,
                    UpeVec3 normal, UpeVec3 up, float w, float h);

/* --- Link two portals together --- */
void UpePortal_Link(UpePortal *a, UpePortal *b);

/* --- Half-space test: returns +1 if pos is on the "outward" side of portal --- */
int UpePortal_SideOf(const UpePortal *portal, UpeVec3 pos);

/* --- Check if object crossed the portal this step ---
   prev_pos: position last frame, cur_pos: current position.
   Returns 1 if the segment prev→cur intersects the portal plane within its bounds. */
int UpePortal_DidCross(const UpePortal *portal, UpeVec3 prev_pos, UpeVec3 cur_pos);

/* --- Compute the portal-to-portal rotation matrix R (from portal A to portal B).
   Transforms directions in A's frame to B's frame. */
UpeMat3 UpePortal_CrossRotation(const UpePortal *a, const UpePortal *b);

/* --- Teleport: compute new state after crossing portal A into portal B.
   Velocity and angular velocity are rotated by the portal cross-rotation matrix.
   Position is re-expressed relative to B's center. */
UpePortalTransfer UpePortal_Teleport(const UpePortal *portal_in,
                                      const UpePortal *portal_out,
                                      UpeVec3 position,
                                      UpeVec3 velocity,
                                      UpeVec3 angular_velocity,
                                      UpeMat3 orientation);

/* --- Camera transform: the UpeMat4 that maps world view through portal pair
   (used by renderer; engine only provides the math). */
UpeMat4 UpePortal_CameraTransform(const UpePortal *portal_in,
                                   const UpePortal *portal_out,
                                   UpeMat4 camera_view);

/* --- Clipping plane at portal: returns plane equation (normal, d) for near-clip
   to prevent rendering objects behind the portal plane. */
void UpePortal_ClipPlane(const UpePortal *portal, UpeVec4 *out_plane);

/* --- Point-in-portal bounds test (2D projection) --- */
int UpePortal_ContainsPoint(const UpePortal *portal, UpeVec3 world_point);

#ifdef __cplusplus
}
#endif
