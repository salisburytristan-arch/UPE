## Implementation Snapshot — April 5, 2026

The roadmap below is now backed by deeper source-level integration in the engine core, not just planning language.

Already integrated in this pass:
- **Roadmap execution milestone**: a first UPE Arena showcase has been added at `examples/upe_arena/main.cpp` with a matching spec at `docs/UPE_ARENA_DEMO.md`. It prints a deterministic state hash, frame count, and performance telemetry consistent with the roadmap's flagship demo direction.
- **Adaptive runtime control** in `core/src/world/upe_world.cpp` via `UpeAdaptiveRuntimeConfig`, dynamic solver tuning, and adaptive CCD behavior.
- **Scientific runtime configuration** via `UpeScientificRuntimeConfig`, unit-aware scaling metadata, physical constants, error budgets, and stability budgets.
- **Expanded integrator framework** with `UPE_INTEGRATOR_EULER`, `UPE_INTEGRATOR_SEMI_IMPLICIT`, `UPE_INTEGRATOR_VELOCITY_VERLET`, and adaptive integrator resolution.
- **Metrology / dimensional support** through richer `UpeUnits` conversions for SI length, speed, acceleration, force, energy, and momentum.
- **Physical-constants registry** through `UpePhysicalConstants_SI()` for gravity, light speed, gas/thermal constants, and standard air properties.
- **Arithmetic / numerical-analysis core** through interval arithmetic, deterministic reductions, least-squares fitting, barycentric interpolation, conditioning estimation, spectral dominant-frequency analysis, Gershgorin spectral-radius bounds, power-iteration eigen analysis, wave-CFL / phase-velocity / numerical-dispersion diagnostics, Rayleigh damping / Euler buckling estimates, Euler-Bernoulli beam stiffness, bar-chain stiffness assembly, and newer PDE helpers for advection, finite-volume flux/divergence, limiter-based MUSCL reconstruction, wave stepping, tridiagonal solves, and multigrid-style restriction/prolongation in `core/include/upe/upe_math_toolbox.h` and `core/src/math/upe_math_toolbox.cpp`.
- **Sparse linear algebra / ODE / optimization wave** through `UpeSparseMatrix`, Conjugate Gradient, GMRES-style minimal residual solving, RK4 / adaptive RK stepping, gradient descent, Newton-style optimization, L-BFGS fallback, and reference FFT/IFFT implementations.
- **Uncertainty-aware diagnostics** through `UpeScienceErrorBudget` plus derived Courant, Froude, Reynolds, and uncertainty-margin metrics.
- **Verification-facing diagnostics** through richer `UpeWorldStepMetrics` including energy, momentum, stability flags, unit scales, and derived non-dimensional analysis values.
- **Control / verification foundations** through `UpeInvariantAuditReport`, `UpeStateEstimateReport`, `UpeLinearSystem2AuditReport`, `UpeScalarMpcReport`, `UpeAlphaBetaTrackerReport`, `UpeScalarTrajectoryAuditReport`, `UpeTrapezoidalTrajectoryPlan`, and `UpeConstantVelocityKalmanReport` for invariant scoring, scalar estimation, linearized stability/controllability/observability checks, finite-horizon MPC-lite planning, observer diagnostics, scalar trajectory-kinematics audits, motion-profile generation, and 2-state Kalman filtering in `core/src/diagnostic/upe_certification_expanded.cpp`.
- **Coupling-coordination diagnostics** through `UpeCouplingCoordinator` execution-order building plus explicit / semi-implicit / implicit residual tracking in `core/src/advanced/upe_coupling.cpp`, providing non-placeholder convergence bookkeeping for multiphysics interface stepping.
- **Regression coverage** in `tests/cases/test_infrastructure.cpp`, verified with fresh build + test runs.

This means the expansion work is now grounded in runtime APIs and diagnostics that the rest of the roadmap can build on cleanly.

### Current execution status

The roadmap is now being advanced with concrete artifact work rather than only conceptual planning:

- `examples/upe_arena/main.cpp` creates a buyer-facing deterministic stack demo and prints a state hash + runtime metrics.
- `tools/run_benchmark_matrix.py` automates a multi-size benchmark matrix so the engine can be evaluated at more than one scene scale.
- `docs/API_STABILITY_POLICY.md` codifies the supported production compatibility boundary and review expectations.
- `docs/BUYER_INTEGRATION_GUIDE.md` provides the buyer onboarding path required for a commercial evaluation.
- `docs/ROADMAP_COMPLETION_STATUS.md` records the milestone evidence and closes the implementation loop for the roadmap layer.

These steps are intentionally chosen to move the project from a broad codebase with impressive feature breadth toward a verifiable commercial core.

## Roadmap status: completion of the buyer-readiness layer

The following four roadmap objectives have now been concretized with live artifacts and proofs:

1. Production core clarity and API stability
2. Independent validation via benchmark and determinism proof surfaces
3. A flagship simulation showcase
4. A straightforward buyer integration path

This is the maturity layer required before additional scientific or domain-pack expansion is treated as strategic roadmap value rather than a technical wishlist.

## Still not fully implemented / only partially implemented

The following are **not done yet** or are still only present as **foundations / placeholders / early slices**:

- **Full scientific math stack**: much broader exact arithmetic, affine/rational arithmetic, full decomposition-heavy numerical linear algebra, richer topology/manifold tooling, and a deeper formal verification layer are still pending beyond the currently integrated numerical toolbox.
- **Full continuum / PDE stack**: the repo now has more concrete PDE and FEM-adjacent foundations (diffusion, Poisson with Jacobi/Gauss-Seidel style solving, residual checks, and two-grid correction, advection, wave stepping, finite-volume operators, Crank-Nicolson stepping, tridiagonal solves, limiter-based reconstruction, multigrid-style transfer operators, and 1D bar-element mass/stiffness/load helpers), but end-to-end FEM/MPM/PDE/finite-volume/spectral/multigrid/domain-decomposition workflows are still not production-complete across the engine.
- **Advanced contact / materials / constitutive science**: the repo now has early continuum/material and structural-dynamics foundations (elastic moduli, elastic energy density, plane-stress von Mises / yield-utilization metrics, perfect-plastic stress clamping, Drucker-Prager-style yield evaluation, a Maxwell-style viscoelastic update, and Newmark-beta / bar-internal-force helpers), but tribology, compliant and cohesive contact, wear, anisotropic/plastic constitutive models, and richer fracture/damage mechanics still remain partial.
- **Broad multiphysics domain packs**: thermodynamics, heat-flow coupling, acoustics, electromagnetics, orbital/space operations, biomechanics, CAD/industrial geometry, and digital-twin style applied packs are not yet deeply integrated end-to-end, even though the coupling coordinator now has stronger execution-order and convergence bookkeeping foundations.
- **Robotics/control depth**: control-analysis diagnostics, scalar state-estimation summaries, alpha-beta observer tracking, 2-state constant-velocity Kalman filtering, linearized stability audits, MPC-lite planning foundations, scalar trajectory-kinematics audits, and trapezoidal motion-profile generation are now present, but richer multi-state estimation, deeper trajectory optimization, full-horizon MPC stacks, and contact-rich robotics workflows are still pending.
- **Concrete stub-replacement sweep completed in this pass**:
  - `core/src/collision/upe_warm_start.cpp` now performs actual manifold reuse validation and impulse warm-start application.
  - `core/src/advanced/upe_unified_solver.cpp` now reports adaptive solver work, residuals, conservation/stability bookkeeping, and nonzero error metrics instead of zeroed placeholders.
  - `core/src/world/upe_gpu_compute.cpp` now executes meaningful CPU fallback kernels and tracks upload/compute/download metrics.
  - `core/src/pbd/upe_pbd.cpp` now includes a proper tetrahedral volume-preservation solve path.
  - `core/src/telemetry/upe_telemetry.cpp` now records thread IDs and exports them in telemetry JSON.
  - `core/src/memory/upe_memory_context.cpp` now records actual frame counts instead of static placeholder values.
- **Still genuinely partial after this sweep**:
  - `core/src/diagnostic/upe_certification_expanded.cpp` is improved, but several torture/determinism helpers still use lightweight synthetic harness logic rather than full industrial-scale execution backends.
  - `core/src/world/upe_gpu_compute.cpp` remains a CPU fallback path rather than vendor-specific CUDA/HIP/compute-shader acceleration.
- **Science catalog honesty check**: `core/src/config/upe_science_catalog.cpp` now reflects `CHEM` as **just started**, `ASTRONOMY` as **minimal**, and `SPACE` as **minimal**; these domains still need much deeper end-to-end packs.

1) Mathematical foundation — what absolutely belongs
A. Arithmetic and computer arithmetic

This is bedrock. Your arithmetic material already points beyond school arithmetic into computer arithmetic, binary arithmetic, matrix arithmetic, and interval-style numerical work.

You should add:

exact integer arithmetic
fixed-point arithmetic
deterministic floating-point policy
interval arithmetic
affine arithmetic
rational arithmetic for reference paths
modular arithmetic for hashing / replay verification
saturating arithmetic where safety matters
error propagation models
rounding-mode control
reproducible reduction rules for sums/products across threads and devices

Why it matters:

determinism
replay fidelity
conservative collision checks
trustworthy solver diagnostics
cross-platform reproducibility
B. Linear algebra

This is not optional. The math material makes clear that linear algebra, vector spaces, matrices, and numerical linear algebra are central practical domains. Numerical linear algebra exists specifically to build accurate, efficient computer algorithms under finite precision, with decompositions like QR, LU, SVD, eigendecomposition, iterative solvers, conditioning, and stability analysis.

You should add:

dense and sparse vectors/matrices
block matrices
matrix factorizations: LU, QR, Cholesky, SVD, eigensolvers
Krylov solvers: CG, GMRES, BiCGSTAB, MINRES
preconditioners
least-squares solvers
tensor algebra
dual numbers / autodiff-friendly linear algebra
batched GPU linear algebra
structured matrix support for constraints and FEM systems

Why it matters:

constraints
articulation
FEM/MPM
cloth
fluids
inverse problems
control
optimization
reduced-order simulation
C. Geometry

This is the heart. Geometry is directly about space, size, distance, shape, surface, curve, manifold, metric, dimension, symmetry, and transformation. The uploaded geometry material also explicitly shows the subfields that matter most for an engine: affine, projective, differential, manifold theory, topology, discrete geometry, computational geometry, convex geometry, complex geometry, non-Euclidean geometry, and symmetry/group-based views of geometry.

You should add:

Euclidean geometry
affine geometry
analytic geometry
projective geometry
metric geometry
convex geometry
discrete geometry
computational geometry
differential geometry
manifold theory
transformation geometry
configuration-space geometry
screw geometry
kinematic geometry
geometric measure concepts
symmetry-aware geometry

Concrete engine uses:

collision and intersections
BVHs, convex hulls, Minkowski ops, clipping, support mappings
contact manifolds
surface frames and curvature
articulated configuration spaces
camera/projection/visibility systems
motion planning
robotic pose math
path space and constraint space design
D. Trigonometry and generalized trigonometry

This sounds basic, but it matters more than people think. Your geometry notes mention spherical, hyperbolic, gyrotrigonometric, and generalized trigonometric methods.

You should add:

standard trig
robust angle and phase handling
spherical trig
hyperbolic trig
orientation-safe trig identities
geodesic/angular interpolation utilities

This matters for:

rotations
constraints
navigation on curved spaces
celestial/orbital extensions
special coordinate systems
E. Algebra

The broader math material shows that algebra expands from equations into structures like groups, fields, vector spaces, rings, Lie groups, Lie algebras, and Boolean/logical structures.

You should add:

abstract algebra only where it cashes out into implementation
group theory for symmetries and transforms
Lie groups and Lie algebras for rigid motion, rotations, robotics, continuous symmetries
representation theory where needed for modes / invariants
ring/field-aware exact math utilities
Clifford / geometric algebra as an optional expert system

This matters for:

SO(2), SO(3), SE(2), SE(3)
twist/wrench math
pose interpolation
exponential/log maps
screw motion
invariant-aware solvers
F. Calculus, analysis, and differential equations

The math material explicitly highlights multivariable calculus, ODEs, PDEs, numerical analysis, and functional-analysis-adjacent areas.

You should add:

single and multivariable calculus
vector calculus
ordinary differential equations
partial differential equations
boundary value problems
initial value problems
functional analysis where operator formulation matters
measure/integration concepts where continuum models need rigor

Concrete need:

time stepping
rigid body integration
elasticity
diffusion
fluids
heat
electromagnetics
wave propagation
constraints on manifolds
G. Numerical analysis

This is one of the biggest “you must add this” categories. Your files explicitly point to numerical analysis and numerical linear algebra as fundamental computational disciplines.

You should add:

conditioning analysis
consistency / stability / convergence analysis
truncation error analysis
adaptive stepping
stiff solvers
implicit / explicit / semi-implicit methods
root finding
nonlinear system solves
quadrature / cubature
interpolation / approximation theory
finite difference methods
finite element methods
finite volume methods
spectral methods
meshfree methods
particle methods
multigrid
domain decomposition
model order reduction

This is what separates a toy engine from a scientific runtime.

H. Optimization and variational math

Your files already connect convex geometry to optimization and discrete math to combinatorial optimization.

You should add:

convex optimization
nonlinear optimization
constrained optimization
least squares
interior-point methods
SQP
projected methods
proximal methods
LCP / NCP / complementarity methods
variational calculus
energy minimization
variational inequalities
duality
inverse problem formulations
differentiable optimization pipelines

Why:

constraint solvers
contact/friction
inverse kinematics
parameter fitting
control
shape matching
system identification
material calibration
I. Discrete math, combinatorics, and graph theory

The uploaded math material explicitly calls out combinatorics, graph theory, discrete geometry, discrete optimization, and algorithms as central to discrete mathematics.

You should add:

graph theory
hypergraphs
combinatorics
discrete geometry
integer programming where needed
constraint graphs
dependency graphs
contact graphs
island graphs
mesh adjacency
topological connectivity graphs
scheduling graphs
sparse graph algorithms

Why:

broadphase / scene graphs
articulation trees
solver ordering
connected components
constraint partitioning
fracture graphs
procedural assembly systems
J. Probability, statistics, and uncertainty

The broader math material includes probability and statistics as core fields.

You should add:

probability theory
statistics
random processes
stochastic simulation
Monte Carlo methods
Bayesian estimation
filtering/smoothing
uncertainty quantification
sensitivity analysis
error bars / confidence intervals
probabilistic collision margins
stochastic material fields
ensemble simulation

This matters for:

sensor modeling
robotics
AI integration
noisy environments
uncertainty-aware prediction
calibration
validation
K. Topology

Your geometry/math material makes this one very clear: topology studies properties under continuous deformation; it branches into general, algebraic, differential, and geometric topology; manifolds, connectedness, compactness, homology/cohomology, and topological data analysis all matter in different contexts.

You should add selectively:

topological connectivity
manifold checks
orientability
mesh validity
homology-aware tools
persistent homology / TDA for debugging and data analysis
knot/link reasoning for cables, ropes, folded structures
contact topology support where useful

Do not let abstract topology bloat the core unless it solves a real engine problem.

L. Logic, formal methods, and computability

The math material also explicitly includes logic, computability, proof theory, type theory, formal verification, and compiler/program-analysis relevance.

You should add:

invariant checking
formal spec for deterministic stepping
proof-oriented test or theorem-assisted validation where feasible
dimensional/unit correctness systems
contract programming
symbolic sanity checks
verifiable serialization / deserialization constraints
reproducibility proofs for selected subsystems

This does not make the engine “academic.” It makes it trustworthy.

M. Harmonic/Fourier/spectral analysis

Your uploaded geometry/mathematics text explicitly references harmonic analysis.

You should add:

Fourier transforms
spectral decomposition
modal analysis
frequency-domain tools
wavelet-ready pipelines if you want advanced signal workflows

Why:

vibration
modal simulation
acoustics
signal-based diagnostics
reduced-order physics
deformation modes
N. Geometric algebra and geometric calculus

Your uploaded notes explicitly call out geometric algebra and geometric calculus.

You should add this as:

an optional expert backend
an internal research layer
possibly a future unification layer for transforms and advanced geometry

Good uses:

bivectors
reflections
rotors
screw-like motion handling
frame calculus
cleaner contact/constraint math

But it should not replace your mainstream API at first.

2) Physics and science foundation — what the engine should scientifically know

This is the part people often miss. A great engine is not only “math-rich.” It is a structured scientific model of the world.

A. Kinematics and rigid-body mechanics

Core now:

particle mechanics
rigid body kinematics
rigid body dynamics
translational and rotational dynamics
inertia tensors
center of mass theory
impulses and momentum
work/energy/power
frames and coordinate systems
screw motion / twists / wrenches
articulated rigid bodies
B. Analytical mechanics

You should add:

Lagrangian mechanics
Hamiltonian mechanics
variational mechanics
generalized coordinates
constrained mechanics
symplectic structure awareness
geometric mechanics

Why:

cleaner formulation of constrained systems
better long-term integrators
robotics relevance
serious scientific credibility
C. Contact mechanics and tribology

This is a huge value area that many engines treat too crudely.

Add:

Hertzian contact
compliant contact models
rigid complementarity contact models
friction cones
stick-slip transitions
rolling resistance
torsional friction
adhesion/cohesion options
wear/abrasion models
rough-surface contact approximations
impact laws with restitution policies

This is where a lot of “feels fake” physics comes from if handled badly.

D. Continuum mechanics

Massive must-have if you want to move beyond rigid bodies.

Add:

continuum kinematics
stress/strain theory
constitutive modeling
hyperelasticity
viscoelasticity
plasticity
elastoplasticity
anisotropy
damage mechanics
fracture mechanics
porous media
shell theory
beam/rod theory
Cosserat/micropolar style models if you go deep
E. Solid mechanics / structural mechanics

Add:

FEM-ready stress analysis
linear elasticity
nonlinear elasticity
beam/shell/plate elements
modal vibration
buckling
fatigue approximations
fracture propagation
deformation gradients
corotational formulations
F. Material science / constitutive science

You need a material model framework, not just “density + friction”.

Add:

isotropic and anisotropic materials
orthotropic materials
composites
foams
rubber-like materials
metals
brittle materials
granular materials
viscoelastic and viscoplastic models
temperature-dependent materials
strain-rate dependence
hysteresis
creep
relaxation
G. Fluid mechanics

This is an entire family.

Add:

incompressible flow
weakly compressible flow
free-surface flow
multiphase flow
turbulence approximations
viscosity models
buoyancy
vorticity methods
boundary layers
fluid-structure interaction
shallow water models
porous flow
non-Newtonian fluids
granular-fluid hybrids
particle and grid methods: SPH, FLIP/PIC/APIC, MPM, finite volume CFD paths
H. Thermodynamics and heat transfer

This is one of the most overlooked “missing sciences.”

Add:

thermodynamic state variables
energy accounting
heat conduction
convection
radiation approximations
latent heat / phase change
thermal expansion
thermoelasticity
thermoviscoelasticity
coupled heat-flow-solid systems

Why:

realistic materials
industrial simulation
melting/freezing/burning paths
higher buyer value outside games
I. Waves, acoustics, and vibration

Add:

wave equations
elastic waves
acoustics
damping models
resonance
modal analysis
coupled vibration systems
impact sound proxies
structural acoustics if you want advanced packs
J. Electromagnetism

Not core for a general rigid-body engine, but extremely valuable as an advanced pack.

Add if you want broader science reach:

electrostatics
magnetostatics
Lorentz forces
charged particle motion
field coupling
electromagnetic actuator approximations
induction/heating approximations where relevant
K. Orbital / celestial mechanics

Usually not core, but very valuable if included as a specialized pack.

Add:

N-body gravity
orbital elements
perturbation methods
symplectic orbit integrators
attitude dynamics for spacecraft
reference frames and ephemeris support
L. Granular / particulate / powder science

Very useful and commercially interesting.

Add:

DEM
cohesion and packing models
angle of repose modeling
particle fracture/aggregation
granular-fluid coupling
bulk handling and compaction
M. Fracture, damage, and destruction science

Add:

crack initiation criteria
crack propagation
damage accumulation
fragmentation
brittle vs ductile fracture modes
peridynamics or other nonlocal approaches as advanced paths
N. Robotics and control science

This is one of the best “make it more wanted” categories.

Add:

forward/inverse kinematics
inverse dynamics
trajectory generation
model predictive control hooks
PID/state-space/controller interfaces
state estimation
actuator/sensor models
contact-rich manipulation support
trajectory optimization
motion planning hooks
O. Biomechanics / soft tissue / muscle-like actuation

Advanced, but powerful for medical/sports/robotics use cases.

Add:

muscle/tendon-style actuators
soft tissue approximations
joint constraints
gait / locomotion-oriented abstractions
compliant-body biomechanical packs
P. Metrology and dimensional science

This is unbelievably important for a “serious” engine.

Add:

compile-time / runtime units and dimensions
SI coherence
physical constants registry
uncertainty propagation
calibration models
measurement metadata
tolerances and accepted error envelopes

This one alone makes you look much more enterprise/scientific.

3) Numerical methods and computational science — the “real engine” layer

This is where the science becomes software.

A. Integrators

Add:

explicit Euler only as a baseline
semi-implicit Euler
Verlet / velocity Verlet
Runge-Kutta family
symplectic integrators
implicit integrators
backward Euler
Newmark-style structural methods
generalized-alpha / BDF-style methods for stiff systems
adaptive time stepping
multi-rate integrators
B. Solver framework

Add:

direct solvers
iterative solvers
nonlinear solvers
complementarity solvers
projected Gauss-Seidel
Jacobi / block solvers
Newton / quasi-Newton
trust-region / line-search infrastructure
warm-starting
preconditioning
convergence diagnostics
solver fallback hierarchy
C. Discretization framework

Add:

finite difference
finite element
finite volume
boundary element
particle/grid hybrid
meshfree approximations
adaptive remeshing / refinement hooks
multiresolution representations
D. Approximation and interpolation

Add:

polynomial interpolation
barycentric interpolation
spline systems
Bézier / NURBS support where CAD matters
radial basis functions
approximation error analysis
surrogate models / reduced-order models
E. Scientific computing infrastructure

Add:

sparse matrix kernels
parallel reduction policies
GPU kernels
SIMD/vectorization
batched simulation
deterministic parallel execution modes
out-of-core / chunked world simulation
mixed-precision pathways
exact/reference mode vs fast mode
4) Verification, validation, and uncertainty — this is where most engines are weak

This is the part that makes UPE look like a real scientific product.

You should add:

reference solutions
analytic benchmark suite
conservation-law tests
invariance tests
convergence tests
mesh/time-step refinement studies
conditioning reports
energy drift reports
determinism certification tests
cross-platform replay tests
parameter sweep tools
uncertainty quantification
sensitivity analysis
calibration workflows
residual/error dashboards
dimensional/unit audits
physical plausibility gates

That is how you move from “engine” to “trusted simulation runtime.”

5) Domain packs — how to organize all this without destroying the core
UPE Core

These should be first-class:

arithmetic/computer arithmetic
deterministic numeric policy
linear algebra / tensor basics
Euclidean + affine + analytic + computational geometry
convex geometry
differential geometry essentials
ODE solvers
rigid body mechanics
collision / contact
constraint solving
basic continuum hooks
units / dimensions
verification harness
numerical linear algebra
graph-based engine infrastructure
UPE Scientific Core Extensions

These should come soon after:

FEM / MPM / cloth / softbody backbone
PDE framework
optimization framework
uncertainty framework
topology-aware mesh/connectivity support
robotics/control layer
thermodynamics and heat coupling
UPE Applied Packs

These should be modular:

vehicles
robotics
cloth
softbody
fracture/destruction
fluids
granular
orbital/celestial
acoustic/vibration
thermal / multiphysics
biomechanics
CAD / industrial geometry
digital-twin / batch-sim pack
UPE Labs

These can be real and valuable, but must stay honestly labeled:

non-Euclidean / hyperbolic packs
geometric algebra backend
advanced topological analytics
exotic manifold simulation
relativistic toy packs
plasma / EM research packs
symbolic mechanics / theorem-aided subsystems
6) Things people usually miss

These are the “you forgot this” items that matter a lot:

dimensional analysis
units and constants
uncertainty quantification
conditioning/stability analysis
approximation theory
numerical linear algebra as its own discipline
convex analysis
variational methods
Lie groups / Lie algebras
screw theory
contact science / tribology
constitutive modeling
heat and thermodynamics
signal / frequency / modal analysis
formal invariants and verification
calibration / system identification
topology for mesh validity and data analysis
parallel determinism policy
reduced-order modeling
error budgeting and metrology

If you add those well, UPE becomes much deeper than a normal physics engine.

7) Things that should stay out of the engine core

Your uploaded materials are very clear that pseudoscience is incompatible with scientific standards, and numerology and astrology are explicitly treated as pseudoscience / pseudomathematics rather than valid scientific foundations.

So these stay out of core engine science:

numerology
astrology
mystical number claims
occult geometry claims
unfalsifiable physical assertions
“special constants” chosen for symbolic reasons rather than empirical or mathematical justification

You can use symbolic motifs for branding or fictional demo packs, but not for engine truth.