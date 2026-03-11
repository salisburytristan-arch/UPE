#include "upe/upe_dem.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
   DEM dilatancy extension implementations
   ----------------------------------------------------------------------- */

/* Volume change rate from shear due to dilatancy (Rowe stress-dilatancy).
   dV/V ≈ tan(dilatancy_angle) * shear_strain_rate
*/
float UpeDem_DilatancyVolumeChange(float shear_strain_rate, float dilatancy_angle_rad) {
    return tanf(dilatancy_angle_rad) * shear_strain_rate;
}

/* Apply dilatancy: expands particle radii slightly proportional to volumetric strain. */
void UpeDem_ApplyDilatancy(UpeDemSystem *dem, float shear_strain_rate) {
    if (!dem || dem->particle_count == 0) return;
    /* Representative dilatancy angle for current material: derive from friction */
    float phi_dil = atanf(dem->default_friction) * 0.5f;  /* Simplified: half friction angle */
    float dV_over_V = UpeDem_DilatancyVolumeChange(shear_strain_rate, phi_dil);
    /* Distribute volumetric strain as radius change: dV/V = 3*(dr/r) for sphere */
    float dr_over_r = dV_over_V / 3.0f;
    if (dr_over_r < 0.0f) dr_over_r = 0.0f;  /* Dilation only (positive) */
    for (uint32_t i = 0; i < dem->particle_count; i++) {
        float new_r = dem->particles[i].radius * (1.0f + dr_over_r);
        dem->particles[i].radius = new_r;
    }
}
