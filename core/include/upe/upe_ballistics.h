#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  External Ballistics (Vehicle/Machinery — Phase 5)

  Models projectile motion beyond the muzzle:
    - 6-DOF trajectory integration (position + orientation)
    - Drag (quadratic & subsonic/supersonic Cd curves)
    - Spin stabilization & gyroscopic precession
    - Magnus effect (spinning projectile lateral drift)
    - Wind drift & crosswind correction
    - Coriolis deflection (long-range sniper / artillery)
    - Muzzle velocity accounting for temperature & propellant burn
    - Terminal ballistics: penetration depth (de Marre / modified Poncelet)
    - Fragmentation spray cone (Gurney energy model)
    - Ricochet angle threshold
*/

#define UPE_BALLISTICS_GRAVITY_DEFAULT 9.81f

/* ---- Projectile descriptor ---- */
typedef struct UpeBallistic {
    /* State */
    UpeVec3  pos;               /* world position (m) */
    UpeVec3  vel;               /* world velocity (m/s) */
    UpeVec3  spin;              /* angular velocity (rad/s) — body frame */

    /* Physical properties */
    float    mass_kg;
    float    diameter_m;        /* reference diameter for drag */
    float    Cd;                /* drag coefficient at subsonic speed */
    float    Cl_magnus;         /* Magnus lift coefficient */
    float    Ix;                /* axial moment of inertia */
    float    Iy;                /* transverse moment of inertia */

    /* Propellant */
    float    muzzle_velocity_ms;
    float    barrel_twist_rps;  /* spin rate at muzzle (rotations per second) */

    /* Integration */
    float    time_of_flight_s;
    float    range_m;           /* accumulated horizontal range */
    int      in_flight;
} UpeBallistic;

/* ---- Wind state ---- */
typedef struct UpeBallisticWind {
    UpeVec3  velocity_ms;       /* world wind velocity */
    float    density_kg_m3;     /* air density (default 1.225) */
    float    sound_speed_ms;    /* speed of sound (default 343) */
} UpeBallisticWind;

/* ---- Terminal ballistics result ---- */
typedef struct UpeBallisticImpact {
    UpeVec3  pos;
    UpeVec3  normal;
    float    impact_velocity_ms;
    float    impact_angle_rad;      /* angle to surface normal */
    float    penetration_depth_m;   /* de Marre result */
    int      ricochet;              /* 1 if shallow angle causes ricochet */
} UpeBallisticImpact;

/* --- Init / reset --- */
void UpeBallistic_Init(UpeBallistic *b);
void UpeBallistic_Fire(UpeBallistic *b, UpeVec3 muzzle_pos, UpeVec3 aim_dir,
                       float muzzle_velocity_ms, float latitude_rad);

/* --- Integration step (full 6-DOF) --- */
void UpeBallistic_Step(UpeBallistic *b, const UpeBallisticWind *wind,
                       UpeVec3 gravity, float dt);

/* --- Drag force (velocity-squared, Cd multiplied by dynamic pressure × area) --- */
UpeVec3 UpeBallistic_DragForce(const UpeBallistic *b, UpeVec3 rel_vel, float air_density);

/* --- Magnus force: F = Cl * rho * A * (omega × v) / |v| --- */
UpeVec3 UpeBallistic_MagnusForce(const UpeBallistic *b, float air_density);

/* --- Coriolis acceleration: a = -2 * omega_earth × v ---
   latitude_rad: shooter latitude.  omega_earth = 7.292e-5 rad/s */
UpeVec3 UpeBallistic_CoriolisAccel(UpeVec3 vel, float latitude_rad);

/* --- Supersonic drag multiplier (Prandtl-Glauert divergence near Mach 1) ---
   Returns Cd_total / Cd_subsonic.  Mach = speed / sound_speed. */
float UpeBallistic_DragMachFactor(float mach);

/* --- de Marre penetration: depth = k * m^0.7 * v^(4/3) / (d^2 * H^0.5)  ---
   Returns penetration depth in meters. */
float UpeBallistic_PenetrationDeMarre(float mass_kg, float velocity_ms,
                                       float diameter_m, float brinell_hardness);

/* --- Gurney fragmentation velocity: v = sqrt(2*E) * sqrt(C/M / (1 + C/(2M)))
   C = explosive mass (kg), M = casing mass (kg), E = Gurney energy (J/kg). */
float UpeBallistic_GurneyFragVelocity(float gurney_energy_J_kg,
                                       float explosive_mass_kg,
                                       float casing_mass_kg);

/* --- Ricochet check: returns 1 if ricochet, 0 if penetration ---
   Based on impact angle vs. material threshold. */
int UpeBallistic_IsRicochet(float impact_angle_rad, float ricochet_threshold_rad,
                              float hardness_ratio);

/* --- Point-mass trajectory estimate (sight correction in meters at range) ---
   Returns drop due to gravity only. */
float UpeBallistic_BulletDrop(float muzzle_velocity_ms, float range_m, float gravity);

#ifdef __cplusplus
}
#endif
