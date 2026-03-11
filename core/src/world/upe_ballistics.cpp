#include "upe/upe_ballistics.h"
#include <math.h>
#include <string.h>

#define UPE_PI         3.14159265358979f
#define UPE_OMEGA_EARTH 7.292115e-5f    /* Earth rotation rad/s */

void UpeBallistic_Init(UpeBallistic *b) {
    if (!b) return;
    memset(b, 0, sizeof(*b));
    b->Cd            = 0.295f;          /* typical rifle bullet */
    b->Cl_magnus     = 0.02f;
    b->mass_kg       = 0.01f;           /* 10-gram bullet */
    b->diameter_m    = 0.00762f;        /* .30 caliber */
    b->Ix            = 1.2e-8f;
    b->Iy            = 4.0e-7f;
    b->muzzle_velocity_ms = 900.0f;
    b->barrel_twist_rps   = 300.0f;
}

void UpeBallistic_Fire(UpeBallistic *b, UpeVec3 muzzle_pos, UpeVec3 aim_dir,
                        float muzzle_velocity_ms, float latitude_rad) {
    if (!b) return;
    (void)latitude_rad;
    float len = sqrtf(aim_dir.x*aim_dir.x + aim_dir.y*aim_dir.y + aim_dir.z*aim_dir.z);
    if (len < 1e-6f) { aim_dir.x = 0; aim_dir.y = 0; aim_dir.z = 1.0f; len = 1.0f; }
    b->pos.x = muzzle_pos.x;
    b->pos.y = muzzle_pos.y;
    b->pos.z = muzzle_pos.z;
    b->vel.x = aim_dir.x / len * muzzle_velocity_ms;
    b->vel.y = aim_dir.y / len * muzzle_velocity_ms;
    b->vel.z = aim_dir.z / len * muzzle_velocity_ms;
    /* Right-hand spin along aim axis */
    b->spin.x = aim_dir.x / len * b->barrel_twist_rps * 2.0f * UPE_PI;
    b->spin.y = aim_dir.y / len * b->barrel_twist_rps * 2.0f * UPE_PI;
    b->spin.z = aim_dir.z / len * b->barrel_twist_rps * 2.0f * UPE_PI;
    b->muzzle_velocity_ms = muzzle_velocity_ms;
    b->time_of_flight_s   = 0.0f;
    b->range_m            = 0.0f;
    b->in_flight          = 1;
}

UpeVec3 UpeBallistic_DragForce(const UpeBallistic *b, UpeVec3 rel_vel, float air_density) {
    UpeVec3 F = {0.0f, 0.0f, 0.0f};
    if (!b || air_density <= 0.0f) return F;
    float speed_sq = rel_vel.x*rel_vel.x + rel_vel.y*rel_vel.y + rel_vel.z*rel_vel.z;
    if (speed_sq < 1e-8f) return F;
    float speed = sqrtf(speed_sq);
    float radius = b->diameter_m * 0.5f;
    float area   = UPE_PI * radius * radius;
    float Fd     = 0.5f * air_density * speed_sq * b->Cd * area;
    float inv    = -Fd / speed;
    F.x = rel_vel.x * inv;
    F.y = rel_vel.y * inv;
    F.z = rel_vel.z * inv;
    return F;
}

UpeVec3 UpeBallistic_MagnusForce(const UpeBallistic *b, float air_density) {
    UpeVec3 F = {0.0f, 0.0f, 0.0f};
    if (!b || air_density <= 0.0f) return F;
    /* F = Cl * rho * A * (omega × v) */
    float radius = b->diameter_m * 0.5f;
    float area   = UPE_PI * radius * radius;
    float scale  = b->Cl_magnus * air_density * area;
    /* omega × v */
    F.x = (b->spin.y * b->vel.z - b->spin.z * b->vel.y) * scale;
    F.y = (b->spin.z * b->vel.x - b->spin.x * b->vel.z) * scale;
    F.z = (b->spin.x * b->vel.y - b->spin.y * b->vel.x) * scale;
    return F;
}

UpeVec3 UpeBallistic_CoriolisAccel(UpeVec3 vel, float latitude_rad) {
    /* omega_earth vector (pointing at north pole, world up = Y) */
    UpeVec3 a = {0.0f, 0.0f, 0.0f};
    float omega = UPE_OMEGA_EARTH;
    /* Simplified: Coriolis = -2 * (omega_earth × vel) in local frame
       omega_local = (0, omega*cos(lat), omega*sin(lat)) in NED-like frame */
    float ox = 0.0f;
    float oy = omega * cosf(latitude_rad);
    float oz = omega * sinf(latitude_rad);
    /* a = -2 * (omega × vel) */
    a.x = -2.0f * (oy * vel.z - oz * vel.y);
    a.y = -2.0f * (oz * vel.x - ox * vel.z);
    a.z = -2.0f * (ox * vel.y - oy * vel.x);
    return a;
}

float UpeBallistic_DragMachFactor(float mach) {
    /* Empirical transonic multiplier — peaks at ~Mach 1 */
    if (mach < 0.8f)  return 1.0f;
    if (mach < 1.0f)  return 1.0f + 3.0f * (mach - 0.8f) * 5.0f;   /* ramp up */
    if (mach < 1.2f)  return 4.0f - 2.0f * (mach - 1.0f) * 5.0f;   /* ramp down */
    /* Supersonic: Cd proportional to 1/Mach */
    return 1.0f + 1.0f / mach;
}

void UpeBallistic_Step(UpeBallistic *b, const UpeBallisticWind *wind,
                        UpeVec3 gravity, float dt) {
    if (!b || !b->in_flight || dt <= 0.0f) return;

    float rho = wind ? wind->density_kg_m3 : 1.225f;
    float sound = wind ? wind->sound_speed_ms : 343.0f;

    /* Relative velocity to air */
    UpeVec3 wind_vel = wind ? wind->velocity_ms : UpeVec3{0.0f, 0.0f, 0.0f};
    UpeVec3 rel_vel = { b->vel.x - wind_vel.x,
                        b->vel.y - wind_vel.y,
                        b->vel.z - wind_vel.z };
    float speed = sqrtf(rel_vel.x*rel_vel.x + rel_vel.y*rel_vel.y + rel_vel.z*rel_vel.z);
    float mach  = (sound > 0.0f) ? speed / sound : 0.0f;
    float mach_factor = UpeBallistic_DragMachFactor(mach);

    /* Drag — scale Cd by mach factor */
    float saved_Cd = b->Cd;
    b->Cd *= mach_factor;
    UpeVec3 Fd = UpeBallistic_DragForce(b, rel_vel, rho);
    b->Cd = saved_Cd;

    /* Magnus */
    UpeVec3 Fm = UpeBallistic_MagnusForce(b, rho);

    /* Coriolis — apply as direct acceleration */
    UpeVec3 Fc = UpeBallistic_CoriolisAccel(b->vel, 0.0f); /* 0 lat default */

    /* Net acceleration */
    float inv_m = (b->mass_kg > 0.0f) ? 1.0f / b->mass_kg : 0.0f;
    float ax = gravity.x + (Fd.x + Fm.x) * inv_m + Fc.x;
    float ay = gravity.y + (Fd.y + Fm.y) * inv_m + Fc.y;
    float az = gravity.z + (Fd.z + Fm.z) * inv_m + Fc.z;

    /* Symplectic Euler integration */
    b->vel.x += ax * dt;
    b->vel.y += ay * dt;
    b->vel.z += az * dt;
    b->pos.x += b->vel.x * dt;
    b->pos.y += b->vel.y * dt;
    b->pos.z += b->vel.z * dt;

    /* Gyroscopic spin decay (viscous torque) */
    float spin_decay = expf(-0.01f * dt);
    b->spin.x *= spin_decay;
    b->spin.y *= spin_decay;
    b->spin.z *= spin_decay;

    b->time_of_flight_s += dt;
    float dh = sqrtf(b->vel.x*b->vel.x + b->vel.z*b->vel.z) * dt;
    b->range_m += dh;
}

float UpeBallistic_PenetrationDeMarre(float mass_kg, float velocity_ms,
                                       float diameter_m, float brinell_hardness) {
    if (diameter_m <= 0.0f || brinell_hardness <= 0.0f || mass_kg <= 0.0f)
        return 0.0f;
    /* de Marre: depth ∝ m^0.7 * v^(4/3) / (d^2 * H^0.5) */
    float k    = 1.8e-5f;   /* calibration constant */
    float m_p  = powf(mass_kg, 0.7f);
    float v_p  = powf(velocity_ms, 4.0f / 3.0f);
    float d_sq = diameter_m * diameter_m;
    float H_sq = sqrtf(brinell_hardness);
    return k * m_p * v_p / (d_sq * H_sq);
}

float UpeBallistic_GurneyFragVelocity(float gurney_energy_J_kg,
                                       float explosive_mass_kg,
                                       float casing_mass_kg) {
    if (casing_mass_kg <= 0.0f || explosive_mass_kg <= 0.0f) return 0.0f;
    float ratio = explosive_mass_kg / casing_mass_kg;
    float bracket = ratio / (1.0f + ratio * 0.5f);
    return sqrtf(2.0f * gurney_energy_J_kg) * sqrtf(bracket);
}

int UpeBallistic_IsRicochet(float impact_angle_rad, float ricochet_threshold_rad,
                              float hardness_ratio) {
    /* Ricochet more likely on hard targets at shallow angles */
    float effective_threshold = ricochet_threshold_rad * (1.0f + 0.5f * hardness_ratio);
    return (impact_angle_rad < effective_threshold) ? 1 : 0;
}

float UpeBallistic_BulletDrop(float muzzle_velocity_ms, float range_m, float gravity) {
    if (muzzle_velocity_ms <= 0.0f) return 0.0f;
    float t = range_m / muzzle_velocity_ms;
    return 0.5f * gravity * t * t;
}
