#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Math Toolbox
  
  - Sparse linear solvers (Conjugate Gradient, GMRES)
  - ODE/PDE integration schemes
  - Optimization (Newton, L-BFGS, gradient descent)
  - Geometry processing (mesh operations)
*/

// ========================
// Arithmetic / Numerical Analysis
// ========================

typedef struct UpeIntervalf {
    float lo;
    float hi;
} UpeIntervalf;

typedef struct UpeDeterministicReductionSummary {
    double sum;
    double product;
    double mean;
    double rms;
    float min_value;
    float max_value;
    uint32_t count;
} UpeDeterministicReductionSummary;

typedef struct UpeLeastSquaresFit1D {
    float slope;
    float intercept;
    float rms_error;
    float correlation;
} UpeLeastSquaresFit1D;

typedef struct UpeStatMoments {
    double mean;
    double variance;
    double stddev;
    float min_value;
    float max_value;
    float confidence95_low;
    float confidence95_high;
    uint32_t count;
} UpeStatMoments;

typedef struct UpeBootstrapSummary {
    float mean;
    float resample_stddev;
    float confidence95_low;
    float confidence95_high;
    uint32_t resample_count;
} UpeBootstrapSummary;

UpeIntervalf UpeMath_Interval(float a, float b);
UpeIntervalf UpeMath_IntervalAdd(UpeIntervalf a, UpeIntervalf b);
UpeIntervalf UpeMath_IntervalSub(UpeIntervalf a, UpeIntervalf b);
UpeIntervalf UpeMath_IntervalMul(UpeIntervalf a, UpeIntervalf b);
UpeIntervalf UpeMath_IntervalDiv(UpeIntervalf a, UpeIntervalf b);
int UpeMath_IntervalContains(UpeIntervalf interval, float value);

int UpeMath_DeterministicReduceF32(const float *values, uint32_t count,
                                   UpeDeterministicReductionSummary *out_summary);
int UpeMath_LeastSquaresFitLine(const float *xs, const float *ys, uint32_t count,
                                UpeLeastSquaresFit1D *out_fit);
int UpeMath_BarycentricInterpolate1D(const float *xs, const float *ys, uint32_t count,
                                     float x, float *out_y);
int UpeMath_ComputeMomentsF32(const float *values, uint32_t count,
                              UpeStatMoments *out_moments);
int UpeMath_BootstrapMeanInterval(const float *values, uint32_t count,
                                  uint32_t resamples,
                                  UpeBootstrapSummary *out_summary);
int UpeMath_ConfidenceInterval95(const float *values, uint32_t count,
                                 float *out_low, float *out_high);
int UpeMath_MovingAverageF32(const float *values, uint32_t count,
                             uint32_t window,
                             float *out_values);
int UpeMath_ExponentialMovingAverageF32(const float *values, uint32_t count,
                                        float alpha,
                                        float *out_values);
int UpeMath_DetectOutlierMaskZScoreF32(const float *values, uint32_t count,
                                       float z_threshold,
                                       uint8_t *out_mask);
float UpeMath_TrimmedMeanF32(const float *values, uint32_t count, float trim_fraction);
float UpeMath_WeightedMeanF32(const float *values, const float *weights, uint32_t count);
int UpeMath_WeightedLeastSquaresFitLine(const float *xs, const float *ys, const float *weights,
                                        uint32_t count, UpeLeastSquaresFit1D *out_fit);
float UpeMath_QuantileF32(const float *values, uint32_t count, float q);
int UpeMath_HistogramF32(const float *values, uint32_t count,
                         float min_value, float max_value,
                         uint32_t bin_count, uint32_t *out_bins);
float UpeMath_ShannonEntropyFromHistogram(const uint32_t *bins, uint32_t bin_count);
float UpeMath_CovarianceF32(const float *xs, const float *ys, uint32_t count);
float UpeMath_CorrelationCoefficientF32(const float *xs, const float *ys, uint32_t count);
float UpeMath_MedianAbsoluteDeviationF32(const float *values, uint32_t count);
float UpeMath_RootMeanSquareErrorF32(const float *predicted, const float *observed, uint32_t count);
float UpeMath_NormalizedRMSEF32(const float *predicted, const float *observed, uint32_t count);
float UpeMath_KolmogorovSmirnovDistanceF32(const float *sample_a, uint32_t count_a,
                                           const float *sample_b, uint32_t count_b);
float UpeMath_DominantFrequencyEstimate(const float *samples, uint32_t count, float sample_rate_hz);
float UpeMath_ConditionNumber2x2(float a00, float a01, float a10, float a11);
int UpeMath_Laplacian1D(const float *values, uint32_t count, float dx, float *out_laplacian);
int UpeMath_DiffusionStep1D(const float *values, uint32_t count,
                            float diffusivity, float dt, float dx,
                            float *out_values);
int UpeMath_PoissonJacobi1D(const float *rhs, uint32_t count, float dx,
                            uint32_t iterations, float *out_solution);
float UpeMath_CourantNumber1D(float velocity, float dt, float dx);
float UpeMath_DiffusionNumber1D(float diffusivity, float dt, float dx);
int UpeMath_AdvectionStep1D(const float *values, uint32_t count,
                            float velocity, float dt, float dx,
                            float *out_values);
int UpeMath_WaveStep1D(const float *current, const float *previous,
                       uint32_t count, float wave_speed,
                       float dt, float dx, float damping,
                       float *out_next);
int UpeMath_TridiagonalSolve(const float *lower, const float *diag,
                             const float *upper, const float *rhs,
                             uint32_t count, float *out_solution);
int UpeMath_RestrictHalfWeighting1D(const float *fine_values, uint32_t fine_count,
                                    float *out_coarse_values, uint32_t coarse_count);
int UpeMath_ProlongateLinear1D(const float *coarse_values, uint32_t coarse_count,
                               float *out_fine_values, uint32_t fine_count);
float UpeMath_LinearElasticStress1D(float strain, float youngs_modulus);
float UpeMath_ShearModulus(float youngs_modulus, float poisson_ratio);
float UpeMath_BulkModulus(float youngs_modulus, float poisson_ratio);
float UpeMath_VonMisesPlaneStress(float sigma_xx, float sigma_yy, float tau_xy);
float UpeMath_MaxwellStressStep(float stress, float strain_rate,
                                float youngs_modulus, float viscosity, float dt);
float UpeMath_ElasticEnergyDensity1D(float strain, float youngs_modulus);
float UpeMath_YieldUtilizationVonMises(float sigma_xx, float sigma_yy,
                                       float tau_xy, float yield_strength);
float UpeMath_UpwindFlux1D(float left_value, float right_value, float velocity);
float UpeMath_RusanovFlux1D(float left_value, float right_value, float wave_speed);
int UpeMath_FiniteVolumeDivergence1D(const float *interface_flux, uint32_t cell_count,
                                     float dx, float *out_divergence);
int UpeMath_FiniteVolumeAdvectionStep1D(const float *values, uint32_t count,
                                        float velocity, float dt, float dx,
                                        float *out_values);
int UpeMath_CrankNicolsonDiffusion1D(const float *values, uint32_t count,
                                     float diffusivity, float dt, float dx,
                                     float *out_values);
int UpeMath_BarElementStiffness1D(float youngs_modulus, float area, float length,
                                  float *out_matrix4);
int UpeMath_BarElementMass1D(float density, float area, float length,
                             float *out_matrix4);
float UpeMath_MinmodLimiter(float a, float b);
float UpeMath_SuperbeeLimiter(float a, float b);
int UpeMath_MusclReconstruct1D(const float *cell_values, uint32_t count,
                               float *out_left, float *out_right);
float UpeMath_PerfectPlasticStress1D(float trial_stress, float yield_strength);
float UpeMath_DruckerPragerYieldFunction(float mean_stress, float deviatoric_q,
                                         float friction_coeff, float cohesion);
int UpeMath_BarElementInternalForce1D(float youngs_modulus, float area, float length,
                                      float displacement_a, float displacement_b,
                                      float *out_force_a, float *out_force_b);
int UpeMath_NewmarkBetaStep1D(float mass, float damping, float stiffness,
                              float external_force, float dt,
                              float beta, float gamma,
                              float *io_displacement,
                              float *io_velocity,
                              float *io_acceleration);
int UpeMath_PoissonGaussSeidel1D(const float *rhs, uint32_t count, float dx,
                                 uint32_t iterations, float *out_solution);
int UpeMath_PoissonResidual1D(const float *solution, const float *rhs, uint32_t count,
                              float dx, float *out_residual);
float UpeMath_NaturalFrequency1D(float stiffness, float mass);
float UpeMath_DampingRatio1D(float mass, float damping, float stiffness);
int UpeMath_TwoGridPoissonCorrection1D(const float *rhs, uint32_t count, float dx,
                                       uint32_t pre_smooth_iters,
                                       uint32_t post_smooth_iters,
                                       float *out_solution);
int UpeMath_BarElementLoad1D(float body_force_density, float length,
                             float *out_vector2);
float UpeMath_KelvinVoigtStress1D(float strain, float strain_rate,
                                  float youngs_modulus, float viscosity);
float UpeMath_CriticalTimeStepSpringMass(float mass, float stiffness);
float UpeMath_CourantWaveStability1D(float wave_speed, float dt, float dx);
float UpeMath_PhaseVelocityRatio1D(float courant_number, float nondim_wavenumber);
float UpeMath_NumericalDispersion1D(float courant_number, float nondim_wavenumber);
int UpeMath_AssembleBarChainStiffness1D(const float *element_stiffness, uint32_t element_count,
                                        float *out_matrix, uint32_t matrix_dim);
float UpeMath_GershgorinSpectralRadius(const float *matrix, uint32_t dim);
int UpeMath_PowerIterationSymmetric(const float *matrix, uint32_t dim, uint32_t iterations,
                                    float *out_eigenvalue, float *out_eigenvector);
float UpeMath_RayleighDamping1D(float mass_coeff, float stiffness_coeff, float mass, float stiffness);
int UpeMath_BeamElementStiffnessEulerBernoulli(float flexural_rigidity, float length,
                                               float *out_matrix16);
int UpeMath_PowerIterationSymmetric3x3(const float *matrix9, uint32_t iterations,
                                       float *out_eigenvalue, float *out_vector3);
int UpeMath_RayleighDampingCoefficients(float damping_ratio_a, float omega_a,
                                        float damping_ratio_b, float omega_b,
                                        float *out_alpha, float *out_beta);
float UpeMath_EulerBucklingLoadPinnedPinned(float youngs_modulus, float area_moment,
                                            float length);

// ========================
// Sparse Matrix
// ========================

typedef struct UpeSparseMatrix {
    float *values;
    uint32_t *col_indices;
    uint32_t *row_ptr;           // CSR format
    uint32_t rows;
    uint32_t cols;
    uint32_t nnz;                // Non-zero count
} UpeSparseMatrix;

void UpeSparse_Init(UpeSparseMatrix *mat, uint32_t rows, uint32_t cols, uint32_t nnz);
void UpeSparse_Destroy(UpeSparseMatrix *mat);
void UpeSparse_SetValue(UpeSparseMatrix *mat, uint32_t row, uint32_t col, float value);
void UpeSparse_MatVecMul(const UpeSparseMatrix *mat, const float *x, float *result);

// ========================
// Linear Solvers
// ========================

// Conjugate Gradient (for symmetric positive-definite systems)
int UpeMath_ConjugateGradient(const UpeSparseMatrix *A, const float *b, float *x,
                               uint32_t max_iters, float tolerance);

// Generalized Minimal Residual (for general systems)
int UpeMath_GMRES(const UpeSparseMatrix *A, const float *b, float *x,
                  uint32_t max_iters, float tolerance, uint32_t restart);

// Incomplete Cholesky preconditioner
void UpeMath_IncompleteCholesky(const UpeSparseMatrix *A, UpeSparseMatrix *L);

// ========================
// ODE Integration
// ========================

typedef void (*UpeOdeRhsFunc)(float t, const float *y, float *dydt, void *user_data);

// Explicit methods
void UpeMath_EulerStep(UpeOdeRhsFunc rhs, float t, float *y, uint32_t n, float dt, void *user_data);
void UpeMath_RK4Step(UpeOdeRhsFunc rhs, float t, float *y, uint32_t n, float dt, void *user_data);

// Adaptive stepping
int UpeMath_RK45AdaptiveStep(UpeOdeRhsFunc rhs, float *t, float *y, uint32_t n,
                             float *dt, float tolerance, void *user_data);

// ========================
// Optimization
// ========================

typedef float (*UpeObjectiveFunc)(const float *x, float *gradient, uint32_t n, void *user_data);

// Gradient descent
int UpeMath_GradientDescent(UpeObjectiveFunc func, float *x, uint32_t n,
                            float learning_rate, uint32_t max_iters, float tolerance, void *user_data);

// Newton's method (requires Hessian)
int UpeMath_Newton(UpeObjectiveFunc func, float *x, uint32_t n,
                   uint32_t max_iters, float tolerance, void *user_data);

// L-BFGS (limited-memory quasi-Newton)
int UpeMath_LBFGS(UpeObjectiveFunc func, float *x, uint32_t n,
                  uint32_t max_iters, float tolerance, uint32_t history_size, void *user_data);

// ========================
// Geometry Processing
// ========================

typedef struct UpeMesh {
    float *vertices;             // x,y,z interleaved
    uint32_t *indices;           // Triangle indices
    uint32_t vertex_count;
    uint32_t triangle_count;
} UpeMesh;

typedef struct UpeMeshTopologySummary {
    uint32_t vertex_count;
    uint32_t triangle_count;
    uint32_t edge_count;
    uint32_t boundary_edge_count;
    uint32_t nonmanifold_edge_count;
    uint32_t connected_components;
    int is_watertight;
    int is_orientable;
    int has_degenerate_faces;
    int euler_characteristic;
} UpeMeshTopologySummary;

// Mesh operations
void UpeMesh_ComputeNormals(UpeMesh *mesh, float *normals);
void UpeMesh_Subdivide(const UpeMesh *input, UpeMesh *output);  // Loop subdivision
void UpeMesh_Simplify(const UpeMesh *input, UpeMesh *output, float target_ratio);
float UpeMesh_ComputeVolume(const UpeMesh *mesh);
void UpeMesh_ComputeBoundingBox(const UpeMesh *mesh, UpeVec3 *min_bounds, UpeVec3 *max_bounds);
int UpeMesh_AnalyzeTopology(const UpeMesh *mesh, UpeMeshTopologySummary *out_summary);

// Convex hull
void UpeMesh_ConvexHull(const float *points, uint32_t point_count, UpeMesh *hull);

// Delaunay triangulation (2D)
void UpeMath_Delaunay2D(const float *points, uint32_t point_count, uint32_t **triangles, uint32_t *triangle_count);

// ========================
// Fast Fourier Transform
// ========================

void UpeMath_FFT(const float *real, const float *imag, float *out_real, float *out_imag, uint32_t n);
void UpeMath_IFFT(const float *real, const float *imag, float *out_real, float *out_imag, uint32_t n);

/* Cubic Bezier curve and tangent */
UpeVec3 UpeMath_BezierCubic(UpeVec3 p0, UpeVec3 p1, UpeVec3 p2, UpeVec3 p3, float t);
UpeVec3 UpeMath_BezierCubicTangent(UpeVec3 p0, UpeVec3 p1, UpeVec3 p2, UpeVec3 p3, float t);

/* Fractal box-counting dimension estimate */
float   UpeMath_FractalBoxDimension(float count_small, float count_large, float scale_ratio);

/* Fast LUT-based sin/cos using 256-entry table (approx, periodic) */
float   UpeMath_LUTSin(float angle_rad);
float   UpeMath_LUTCos(float angle_rad);

/* Square-cube law: area scales as linear_scale^2, volume as linear_scale^3.
  Returns surface-area / volume ratio relative to unit scale. */
float   UpeMath_SquareCubeLaw(float linear_scale);

#ifdef __cplusplus
}
#endif
