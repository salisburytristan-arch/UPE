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

// Mesh operations
void UpeMesh_ComputeNormals(UpeMesh *mesh, float *normals);
void UpeMesh_Subdivide(const UpeMesh *input, UpeMesh *output);  // Loop subdivision
void UpeMesh_Simplify(const UpeMesh *input, UpeMesh *output, float target_ratio);
float UpeMesh_ComputeVolume(const UpeMesh *mesh);
void UpeMesh_ComputeBoundingBox(const UpeMesh *mesh, UpeVec3 *min_bounds, UpeVec3 *max_bounds);

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
