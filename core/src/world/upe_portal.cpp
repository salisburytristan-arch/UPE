#include "upe/upe_portal.h"
#include <math.h>
#include <string.h>

static float v3_dot(UpeVec3 a, UpeVec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static UpeVec3 v3_cross(UpeVec3 a, UpeVec3 b) {
    UpeVec3 r = { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
    return r;
}
static UpeVec3 v3_norm(UpeVec3 v) {
    float l = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (l < 1e-7f) { UpeVec3 r = {0.0f,1.0f,0.0f}; return r; }
    UpeVec3 r = { v.x/l, v.y/l, v.z/l }; return r;
}
static UpeVec3 v3_sub(UpeVec3 a, UpeVec3 b) { UpeVec3 r = {a.x-b.x,a.y-b.y,a.z-b.z}; return r; }
static UpeVec3 v3_add(UpeVec3 a, UpeVec3 b) { UpeVec3 r = {a.x+b.x,a.y+b.y,a.z+b.z}; return r; }
static UpeVec3 v3_scale(UpeVec3 v, float s) { UpeVec3 r = {v.x*s,v.y*s,v.z*s}; return r; }
static UpeVec3 mat3_mulv(UpeMat3 m, UpeVec3 v) {
    UpeVec3 r;
    r.x = m.m[0][0]*v.x + m.m[0][1]*v.y + m.m[0][2]*v.z;
    r.y = m.m[1][0]*v.x + m.m[1][1]*v.y + m.m[1][2]*v.z;
    r.z = m.m[2][0]*v.x + m.m[2][1]*v.y + m.m[2][2]*v.z;
    return r;
}

void UpePortal_Init(UpePortal *portal, uint32_t id, UpeVec3 pos,
                    UpeVec3 normal, UpeVec3 up, float w, float h) {
    if (!portal) return;
    portal->position  = pos;
    portal->normal    = v3_norm(normal);
    portal->up        = v3_norm(up);
    portal->width     = w;
    portal->height    = h;
    portal->id        = id;
    portal->linked_id = 0;
    portal->active    = 1;
}

void UpePortal_Link(UpePortal *a, UpePortal *b) {
    if (!a || !b) return;
    a->linked_id = b->id;
    b->linked_id = a->id;
}

int UpePortal_SideOf(const UpePortal *portal, UpeVec3 pos) {
    if (!portal) return 1;
    float d = v3_dot(v3_sub(pos, portal->position), portal->normal);
    return (d >= 0.0f) ? 1 : -1;
}

int UpePortal_DidCross(const UpePortal *portal, UpeVec3 prev_pos, UpeVec3 cur_pos) {
    if (!portal || !portal->active) return 0;
    /* Line-plane intersection */
    float d0 = v3_dot(v3_sub(prev_pos, portal->position), portal->normal);
    float d1 = v3_dot(v3_sub(cur_pos,  portal->position), portal->normal);
    if (d0 * d1 >= 0.0f) return 0;  /* same side — no crossing */

    /* Crossing parameter t in [0,1] */
    float denom = d0 - d1;
    if (fabsf(denom) < 1e-7f) return 0;
    float t = d0 / denom;
    UpeVec3 hit;
    hit.x = prev_pos.x + (cur_pos.x - prev_pos.x) * t;
    hit.y = prev_pos.y + (cur_pos.y - prev_pos.y) * t;
    hit.z = prev_pos.z + (cur_pos.z - prev_pos.z) * t;

    return UpePortal_ContainsPoint(portal, hit);
}

int UpePortal_ContainsPoint(const UpePortal *portal, UpeVec3 world_point) {
    if (!portal) return 0;
    UpeVec3 local = v3_sub(world_point, portal->position);
    /* Project onto portal plane axes */
    UpeVec3 right = v3_cross(portal->normal, portal->up);
    float px = v3_dot(local, right);
    float py = v3_dot(local, portal->up);
    return (fabsf(px) <= portal->width * 0.5f && fabsf(py) <= portal->height * 0.5f) ? 1 : 0;
}

/* Build portal-space basis: columns are [right, up, normal] */
static void portal_basis(const UpePortal *p, UpeVec3 *right, UpeVec3 *up, UpeVec3 *n) {
    *n     = p->normal;
    *up    = p->up;
    *right = v3_cross(*n, *up);
    *right = v3_norm(*right);
    *up    = v3_cross(*right, *n);
    *up    = v3_norm(*up);
}

UpeMat3 UpePortal_CrossRotation(const UpePortal *a, const UpePortal *b) {
    /* R = Mb * Ma^T  (rotate from A's frame into world, then into B's frame) */
    UpeMat3 R;
    UpeVec3 ar, au, an, br, bu, bn;
    portal_basis(a, &ar, &au, &an);
    portal_basis(b, &br, &bu, &bn);

    /* Ma:  columns = [ar, au, an]  but we flip B's normal (portals face each other) */
    bn = v3_scale(bn, -1.0f);  /* exit faces opposite to entry */

    /* R[i][j] = dot(b_axis_i, a_axis_j) */
    UpeVec3 ba[3] = {br, bu, bn};
    UpeVec3 aa[3] = {ar, au, an};
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            R.m[i][j] = v3_dot(ba[i], aa[j]);
    return R;
}

UpePortalTransfer UpePortal_Teleport(const UpePortal *portal_in,
                                      const UpePortal *portal_out,
                                      UpeVec3 position,
                                      UpeVec3 velocity,
                                      UpeVec3 angular_velocity,
                                      UpeMat3 orientation) {
    UpePortalTransfer result;
    memset(&result, 0, sizeof(result));
    if (!portal_in || !portal_out) return result;

    UpeMat3 R = UpePortal_CrossRotation(portal_in, portal_out);

    /* Translate position relative to portal_in center, rotate, then offset by portal_out center */
    UpeVec3 local = v3_sub(position, portal_in->position);
    UpeVec3 rotated_local = mat3_mulv(R, local);
    result.new_position = v3_add(portal_out->position, rotated_local);
    result.new_velocity       = mat3_mulv(R, velocity);
    result.new_angular_vel    = mat3_mulv(R, angular_velocity);

    /* Rotate orientation: R * (current orientation columns) */
    for (int col = 0; col < 3; col++) {
        UpeVec3 oc = { orientation.m[0][col], orientation.m[1][col], orientation.m[2][col] };
        UpeVec3 nc = mat3_mulv(R, oc);
        result.new_orientation.m[0][col] = nc.x;
        result.new_orientation.m[1][col] = nc.y;
        result.new_orientation.m[2][col] = nc.z;
    }
    result.transferred = 1;
    return result;
}

UpeMat4 UpePortal_CameraTransform(const UpePortal *portal_in,
                                   const UpePortal *portal_out,
                                   UpeMat4 camera_view) {
    /* P_out * P_in^{-1} * V_camera */
    /* For now: build 4x4 by embedding 3x3 portal cross-rotation + offset */
    UpeMat4 result = camera_view;   /* fallback: identity copy */
    if (!portal_in || !portal_out) return result;
    UpeMat3 R = UpePortal_CrossRotation(portal_in, portal_out);
    /* Embed R into upper-left 3x3 of result, preserving projective rows */
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            result.m[i][j] = R.m[i][j];
    result.m[0][3] = portal_out->position.x - R.m[0][0]*portal_in->position.x
                                             - R.m[0][1]*portal_in->position.y
                                             - R.m[0][2]*portal_in->position.z;
    result.m[1][3] = portal_out->position.y - R.m[1][0]*portal_in->position.x
                                             - R.m[1][1]*portal_in->position.y
                                             - R.m[1][2]*portal_in->position.z;
    result.m[2][3] = portal_out->position.z - R.m[2][0]*portal_in->position.x
                                             - R.m[2][1]*portal_in->position.y
                                             - R.m[2][2]*portal_in->position.z;
    result.m[3][0] = 0.0f; result.m[3][1] = 0.0f;
    result.m[3][2] = 0.0f; result.m[3][3] = 1.0f;
    return result;
}

void UpePortal_ClipPlane(const UpePortal *portal, UpeVec4 *out_plane) {
    if (!portal || !out_plane) return;
    out_plane->x = portal->normal.x;
    out_plane->y = portal->normal.y;
    out_plane->z = portal->normal.z;
    out_plane->w = -(v3_dot(portal->normal, portal->position));
}
