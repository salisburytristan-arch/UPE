#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ACOUSTICS HEADER
//
// - Acoustic wave equation solver (FDTD finite differences)
// - Sound propagation with geometric and atmospheric attenuation
// - Modal synthesis for impact sounds (IIR resonant filters)
// - Sound pressure level (dB SPL) and frequency analysis
// - Acoustic intensity and power computation
// ============================================================================

// Acoustic grid for wave equation solver
typedef struct UpeAcousticsGrid {
    float *pressure;         // Pressure field p(x,y,z,t)
    float *pressure_vel;     // Pressure time derivative ∂p/∂t
    float *pressure_accel;   // Pressure acceleration ∂²p/∂t²
    float *absorption;       // Material absorption (0=reflective, 1=absorbing)
    
    uint32_t res_x, res_y, res_z;
    float cell_size;
    float speed_of_sound;
} UpeAcousticsGrid;

// Modal filter for synthesizing resonant sounds
typedef struct UpeAcousticsModalFilter {
    float frequency;        // Resonant frequency (Hz)
    float damping;          // Damping factor (0-1)
    float peak_amplitude;   // Peak response amplitude
    
    // Biquad filter coefficients
    float b0, b1, b2;
    float a1, a2;
    
    // Filter state (z-transform)
    float x1, x2;
    float y1, y2;
} UpeAcousticsModalFilter;

// Grid management
void UpeAcoustics_GridInit(UpeAcousticsGrid *grid, uint32_t res_x, uint32_t res_y, uint32_t res_z, float cell_size);
void UpeAcoustics_GridDestroy(UpeAcousticsGrid *grid);

// Grid control
void UpeAcoustics_SetAbsorption(UpeAcousticsGrid *grid, float absorption);
void UpeAcoustics_ExcitePulse(UpeAcousticsGrid *grid, UpeVec3 pos, float amplitude);

// Wave equation solver
void UpeAcoustics_StepFDTD(UpeAcousticsGrid *grid, float dt);

// Modal synthesis
void UpeAcoustics_InitModalFilter(UpeAcousticsModalFilter *filter, float frequency, float damping, float amplitude);
float UpeAcoustics_ModalFilterStep(UpeAcousticsModalFilter *filter, float input);

// Frequency domain
float UpeAcoustics_ComputeSPL(float pressure);
float UpeAcoustics_ComputeRMSPressure(const UpeAcousticsGrid *grid);
float UpeAcoustics_EstimateDominantFrequency(const UpeAcousticsModalFilter *filters, uint32_t num_filters);

// Propagation and attenuation
float UpeAcoustics_GeometricAttenuation(float distance);
float UpeAcoustics_AtmosphericAbsorption(float frequency, float distance, float humidity);
float UpeAcoustics_ComputeAttenuation(float distance, float frequency, float humidity);

// Energy and power
float UpeAcoustics_ComputeIntensity(float pressure);
float UpeAcoustics_ComputeTotalEnergy(const UpeAcousticsGrid *grid);
float UpeAcoustics_ComputeSoundPower(const UpeAcousticsGrid *grid, uint32_t axis);

/* Doppler-shifted frequency  f' = f0 * (v_sound + v_observer) / (v_sound + v_source)  */
float UpeAcoustics_DopplerFrequency(float f0, float v_sound,
                                     float v_source, float v_observer);

/* Whitham-based sonic boom overpressure estimate [Pa] */
float UpeAcoustics_SonicBoomOverpressure(float mach, float altitude_m, float wingspan_m);

/* Single-edge Kirchhoff diffraction attenuation [dB] */
float UpeAcoustics_DiffractionAttenuation(float wavelength, float aperture, float angle_rad);

#ifdef __cplusplus
}
#endif
