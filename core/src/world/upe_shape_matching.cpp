#include "upe/upe_shape_matching.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* -----------------------------------------------------------------------
   Polar Decomposition via iterative symmetric eigenanalysis
   Uses the Higham-Schur iteration: repeatedly symmetrizes M to converge on R.
   Fast for 3x3, stable for near-rotation matrices.
   ----------------------------------------------------------------------- */

static UpeMat3 mat3_mul(UpeMat3 a, UpeMat3 b) {
    UpeMat3 r;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            r.m[i][j] = 0.0f;
            for (int k = 0; k < 3; k++)
                r.m[i][j] += a.m[i][k] * b.m[k][j];
        }
    return r;
}

static UpeMat3 mat3_transpose(UpeMat3 a) {
    UpeMat3 r;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            r.m[i][j] = a.m[j][i];
    return r;
}

static float mat3_det(UpeMat3 m) {
    return m.m[0][0]*(m.m[1][1]*m.m[2][2] - m.m[1][2]*m.m[2][1])
          -m.m[0][1]*(m.m[1][0]*m.m[2][2] - m.m[1][2]*m.m[2][0])
          +m.m[0][2]*(m.m[1][0]*m.m[2][1] - m.m[1][1]*m.m[2][0]);
}

static int mat3_inverse(UpeMat3 m, UpeMat3 *out) {
    float det = mat3_det(m);
    if (fabsf(det) < 1e-10f) return 0;
    float inv_det = 1.0f / det;
    out->m[0][0] = (m.m[1][1]*m.m[2][2] - m.m[1][2]*m.m[2][1]) * inv_det;
    out->m[0][1] = (m.m[0][2]*m.m[2][1] - m.m[0][1]*m.m[2][2]) * inv_det;
    out->m[0][2] = (m.m[0][1]*m.m[1][2] - m.m[0][2]*m.m[1][1]) * inv_det;
    out->m[1][0] = (m.m[1][2]*m.m[2][0] - m.m[1][0]*m.m[2][2]) * inv_det;
    out->m[1][1] = (m.m[0][0]*m.m[2][2] - m.m[0][2]*m.m[2][0]) * inv_det;
    out->m[1][2] = (m.m[0][2]*m.m[1][0] - m.m[0][0]*m.m[1][2]) * inv_det;
    out->m[2][0] = (m.m[1][0]*m.m[2][1] - m.m[1][1]*m.m[2][0]) * inv_det;
    out->m[2][1] = (m.m[0][1]*m.m[2][0] - m.m[0][0]*m.m[2][1]) * inv_det;
    out->m[2][2] = (m.m[0][0]*m.m[1][1] - m.m[0][1]*m.m[1][0]) * inv_det;
    return 1;
}

int UpeShapeMatch_PolarDecompose(const UpeMat3 *M, UpeMat3 *R, UpeMat3 *S) {
    if (!M || !R || !S) return -1;

    /* Higham iteration: Mk+1 = 0.5*(Mk + (Mk^-T)) converges to R */
    UpeMat3 cur = *M;
    for (int iter = 0; iter < 20; ++iter) {
        UpeMat3 cur_inv;
        if (!mat3_inverse(cur, &cur_inv)) break;
        UpeMat3 cur_inv_t = mat3_transpose(cur_inv);
        float diff = 0.0f;
        UpeMat3 next;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) {
                next.m[i][j] = 0.5f * (cur.m[i][j] + cur_inv_t.m[i][j]);
                diff += (next.m[i][j] - cur.m[i][j]) * (next.m[i][j] - cur.m[i][j]);
            }
        cur = next;
        if (diff < 1e-12f) break;
    }
    *R = cur;
    /* S = R^T * M */
    *S = mat3_mul(mat3_transpose(cur), *M);
    /* Ensure det(R) = +1 (not a reflection) */
    if (mat3_det(*R) < 0.0f) {
        for (int i = 0; i < 3; i++) R->m[i][2] = -R->m[i][2];
    }
    return 0;
}

/* -----------------------------------------------------------------------
   Core shape matching
   ----------------------------------------------------------------------- */

UpeVec3 UpeShapeMatch_CenterOfMass(const UpeVec3 *positions,
                                    const float   *masses,
                                    uint32_t       n_particles) {
    UpeVec3 com = {0.0f, 0.0f, 0.0f};
    float total_mass = 0.0f;
    for (uint32_t i = 0; i < n_particles; ++i) {
        float m = masses ? masses[i] : 1.0f;
        com.x += m * positions[i].x;
        com.y += m * positions[i].y;
        com.z += m * positions[i].z;
        total_mass += m;
    }
    if (total_mass > 0.0f) {
        float inv = 1.0f / total_mass;
        com.x *= inv; com.y *= inv; com.z *= inv;
    }
    return com;
}

UpeMat3 UpeShapeMatch_ComputeApq(const UpeShapeMatchingBody *body) {
    UpeMat3 Apq;
    memset(&Apq, 0, sizeof(Apq));
    if (!body) return Apq;

    for (uint32_t i = 0; i < body->n_particles; ++i) {
        float m = body->masses ? body->masses[i] : 1.0f;
        /* pi = current - current_com */
        float px = body->positions[i].x - body->current_com.x;
        float py = body->positions[i].y - body->current_com.y;
        float pz = body->positions[i].z - body->current_com.z;
        /* qi = rest - rest_com */
        float qx = body->rest_positions[i].x - body->rest_com.x;
        float qy = body->rest_positions[i].y - body->rest_com.y;
        float qz = body->rest_positions[i].z - body->rest_com.z;
        /* Apq += m * outer(p, q) */
        UpeVec3 p = {px, py, pz};
        UpeVec3 q = {qx, qy, qz};
        Apq.m[0][0] += m * p.x * q.x;
        Apq.m[0][1] += m * p.x * q.y;
        Apq.m[0][2] += m * p.x * q.z;
        Apq.m[1][0] += m * p.y * q.x;
        Apq.m[1][1] += m * p.y * q.y;
        Apq.m[1][2] += m * p.y * q.z;
        Apq.m[2][0] += m * p.z * q.x;
        Apq.m[2][1] += m * p.z * q.y;
        Apq.m[2][2] += m * p.z * q.z;
    }
    return Apq;
}

void UpeShapeMatch_Init(UpeShapeMatchingBody *body, uint32_t n_particles,
                        float stiffness, float damping) {
    if (!body || n_particles == 0) return;
    memset(body, 0, sizeof(*body));
    body->n_particles = n_particles;
    body->stiffness   = (stiffness < 0.0f) ? 0.0f : (stiffness > 1.0f ? 1.0f : stiffness);
    body->damping     = (damping < 0.0f) ? 0.0f : damping;
    body->rest_positions = (UpeVec3 *)malloc(n_particles * sizeof(UpeVec3));
    body->masses         = (float  *)malloc(n_particles * sizeof(float));
    body->positions      = (UpeVec3 *)malloc(n_particles * sizeof(UpeVec3));
    body->velocities     = (UpeVec3 *)malloc(n_particles * sizeof(UpeVec3));
    if (!body->rest_positions || !body->masses || !body->positions || !body->velocities) {
        free(body->rest_positions); free(body->masses);
        free(body->positions);      free(body->velocities);
        memset(body, 0, sizeof(*body));
        return;
    }
    for (uint32_t i = 0; i < n_particles; ++i) {
        body->masses[i] = 1.0f;
        UpeVec3 zero = {0.0f, 0.0f, 0.0f};
        body->rest_positions[i] = zero;
        body->positions[i]      = zero;
        body->velocities[i]     = zero;
    }
    body->rotation = UpeMat3_Identity();
}

void UpeShapeMatch_Destroy(UpeShapeMatchingBody *body) {
    if (!body) return;
    free(body->rest_positions);
    free(body->masses);
    free(body->positions);
    free(body->velocities);
    memset(body, 0, sizeof(*body));
}

void UpeShapeMatch_SetRestPose(UpeShapeMatchingBody *body) {
    if (!body) return;
    memcpy(body->rest_positions, body->positions, body->n_particles * sizeof(UpeVec3));
    body->rest_com = UpeShapeMatch_CenterOfMass(body->rest_positions,
                                                body->masses, body->n_particles);
}

void UpeShapeMatch_Step(UpeShapeMatchingBody *body, float dt) {
    if (!body || dt <= 0.0f || body->n_particles == 0) return;

    /* 1. Update current center of mass */
    body->current_com = UpeShapeMatch_CenterOfMass(body->positions,
                                                    body->masses,
                                                    body->n_particles);
    body->translation.x = body->current_com.x - body->rest_com.x;
    body->translation.y = body->current_com.y - body->rest_com.y;
    body->translation.z = body->current_com.z - body->rest_com.z;

    /* 2. Compute Apq */
    UpeMat3 Apq = UpeShapeMatch_ComputeApq(body);

    /* 3. Polar decomposition: Apq = R * S */
    UpeMat3 S;
    UpeShapeMatch_PolarDecompose(&Apq, &body->rotation, &S);

    /* 4. Compute goal positions: g_i = R * (q_i) + com_current */
    float inv_dt = (dt > 0.0f) ? 1.0f / dt : 0.0f;
    float alpha  = body->stiffness;

    for (uint32_t i = 0; i < body->n_particles; ++i) {
        /* q_i = rest_position - rest_com */
        float qx = body->rest_positions[i].x - body->rest_com.x;
        float qy = body->rest_positions[i].y - body->rest_com.y;
        float qz = body->rest_positions[i].z - body->rest_com.z;

        /* goal = R * q + current_com */
        float gx = body->rotation.m[0][0]*qx + body->rotation.m[0][1]*qy + body->rotation.m[0][2]*qz + body->current_com.x;
        float gy = body->rotation.m[1][0]*qx + body->rotation.m[1][1]*qy + body->rotation.m[1][2]*qz + body->current_com.y;
        float gz = body->rotation.m[2][0]*qx + body->rotation.m[2][1]*qy + body->rotation.m[2][2]*qz + body->current_com.z;

        /* Restoring velocity delta proportional to goal displacement */
        float dx = gx - body->positions[i].x;
        float dy = gy - body->positions[i].y;
        float dz = gz - body->positions[i].z;

        body->velocities[i].x += alpha * dx * inv_dt - body->damping * body->velocities[i].x;
        body->velocities[i].y += alpha * dy * inv_dt - body->damping * body->velocities[i].y;
        body->velocities[i].z += alpha * dz * inv_dt - body->damping * body->velocities[i].z;

        /* Integrate position */
        body->positions[i].x += body->velocities[i].x * dt;
        body->positions[i].y += body->velocities[i].y * dt;
        body->positions[i].z += body->velocities[i].z * dt;
    }
}
