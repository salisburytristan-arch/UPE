# Feature Status Matrix

Comprehensive status of every physics feature in the requested audit. Five classification states:

- **Present** — implemented and usable
- **Partial** — some support exists; not first-class
- **Absent** — not yet implemented
- **Utility-only** — exposed as math/tooling, not simulation state
- **Approximation** — mathematically modelled stub; not full physical simulation

---

## I. Microscopic & Atomic Scale

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| Brownian Motion | Present | `upe_molecular_physics.h` | Seeded Gaussian RNG impulses; diffusion coefficient configurable |
| Van der Waals Forces | Present | `upe_molecular_physics.h` | Lennard-Jones 6-12 potential |
| Electron Cloud Probability | Approximation | `upe_molecular_physics.h` | Spatial probability density field; not QM |
| Lattice Vibrations (Phonons) | Approximation | `upe_molecular_physics.h` | Harmonic oscillator normal-mode model |
| Micro-fluidics / Capillary Action | Present | `upe_molecular_physics.h` | Young-Laplace pressure, contact angle |
| Ion Channel Gating | Approximation | `upe_molecular_physics.h` | Hodgkin-Huxley-inspired probabilistic gate stub |
| Nuclear Fission/Fusion | Approximation | `upe_molecular_physics.h` | Q-value energy release; chain-reaction trigger; no cross-section |
| Quantum Tunneling | Approximation | `upe_molecular_physics.h` | WKB barrier penetration probability |
| Pauli Exclusion | Approximation | `upe_molecular_physics.h` | Short-range repulsion at sub-atomic separation |
| Covalent Bond Tension | Present | `upe_molecular_physics.h` | Morse potential spring with break threshold |

---

## II. Material & Soft Body Physics

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| Eulerian Fluid | Present | `upe_flip.h` | FLIP grid-based fluid |
| Lagrangian Fluid / SPH | Present | `upe_fluid_sph.h` | SPH up to 8192 particles |
| Verlet Integration | Present | `upe_softbody.h`, `upe_hair.h` | Position-history Verlet |
| Plasticity | Present | `upe_fem.h` | Plastic strain accumulation in tetrahedral elements |
| Elasticity (Hooke's Law) | Present | `upe_joint.h`, `upe_softbody.h` | Spring stiffness in joints and mass-spring |
| Viscosity | Present | `upe_fluid_sph.h` | SPH viscosity kernel; non-Newtonian |
| Brittle Fracture | Present | `upe_fracture.h`, `upe_destruction.h` | Impact fragmentation, debris |
| Ductile Tearing | Present | `upe_fem.h` | Plastic strain exceeding toughness → torn surface |
| Hysteresis | Present | `upe_viscoelastic.h` | Rate-dependent spring-dashpot stress history |
| Torsion | Present | `upe_fem.h` | Off-diagonal shear strain tensor in FEM |
| Shear Stress | Present | `upe_fem.h` | Internal parallel force component in FEM elements |
| FEM | Present | `upe_fem.h` | Corotational, hyperelastic, plasticity, fracture |
| Mass-Spring-Damper | Present | `upe_softbody.h`, `upe_joint.h` | Spring joint + soft body mass-spring network |
| PBD | Present | `upe_cloth.h`, `upe_xpbd_solver.h` | Position-based + XPBD |
| Shape Matching | Present | `upe_shape_matching.h` | Polar decomposition cluster restore |
| Non-Newtonian Fluids | Present | `upe_fluid_sph.h` | Shear-rate-dependent viscosity in SPH |
| Granular Flow | Present | `upe_dem.h` | DEM up to 65K particles |
| Anisotropy | Present | `upe_fem.h` | Transverse-isotropic elastic tensor (fiber direction) |
| Dilatancy | Present | `upe_dem.h` | Reynolds dilatancy angle; volume change under shear |
| Surface Tension | Present | `upe_fluid_sph.h` | SPH curvature-based cohesive forces |

---

## III. Kinematics & Constraints

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| Inverse Kinematics (IK) | Present | `upe_ik_solver.h` | FABRIK + 2-bone CCD |
| Forward Kinematics (FK) | Present | `upe_articulation.h` | Parent-child chain transforms |
| Ragdoll Dynamics | Present | `upe_animation.h` | Physics blend 0–1 |
| Procedural Animation | Present | `upe_procedural_animation.h` | Physics-constraint-driven |
| Ball-and-Socket Joint | Present | `upe_joint.h` | `UPE_JOINT_BALL` |
| Hinge/Revolute Joint | Present | `upe_joint.h` | `UPE_JOINT_HINGE` |
| Slider/Prismatic Joint | Present | `upe_joint.h` | `UPE_JOINT_SLIDER` |
| Universal Joint | Present | `upe_joint.h` | `UPE_JOINT_UNIVERSAL`; Hooke's joint kinematics |
| Fixed Joint | Present | `upe_joint.h` | 6-DOF constraint |
| Pulley Constraint | Present | `upe_joint.h` | `UPE_JOINT_PULLEY` |
| Gear Constraint | Present | `upe_joint.h` | `UPE_JOINT_GEAR` |
| Spring/Suspension | Present | `upe_joint.h` | `UPE_JOINT_SPRING` |
| Buoyancy | Present | `upe_buoyancy.h` | Archimedes submerged-volume integration |
| Aerodynamics / Lift | Present | `upe_aerodynamics.h` | Bernoulli lift coefficient |
| Drag | Present | `upe_aerodynamics.h` | Velocity-squared aerodynamic drag |
| Terminal Velocity | Present | `upe_aerodynamics_advanced.h` | Force equilibrium limit |
| Conservation of Momentum | Present | `upe_solver.h` | Enforced via impulse solver |
| Conservation of Angular Momentum | Present | `upe_body.h` | Angular dynamics in rigid body pipeline |
| Centripetal Force | Partial | `upe_body.h` | Radius × ω² derivable; no dedicated API |
| Coriolis Effect | Present | `upe_joint.h` | Rotating-frame F_cor = -2m(ω × v_rel) |

---

## IV. Environmental & Atmospheric

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| Thermodynamics | Present | `upe_thermal.h` | Fourier heat diffusion grid |
| Convection | Present | `upe_thermal.h` | Buoyancy-driven velocity; Rayleigh-Bénard stub |
| Radiation | Present | `upe_radiation.h` | Stefan-Boltzmann emission header |
| Evaporation / Condensation | Present | `upe_phase_change.h` | Antoine equation; latent heat; particle creation |
| Thermal Expansion | Present | `upe_thermal.h` | Body size scales with ΔT × CTE |
| Bernoulli Flow | Present | `upe_aerodynamics.h` | Static + dynamic pressure conserved |
| Wind Turbulence | Present | `upe_field_system.h` | Perlin-noise-driven gust force field |
| Static Pressure | Present | `upe_aerodynamics.h` | P = ρgh hydrostatic depth pressure |
| Barometric Pressure | Present | `upe_aerodynamics.h` | ISA exponential atmosphere altitude model |
| Sonic Booms | Present | `upe_acoustics.h` | Mach cone geometry; pressure spike callback |
| Doppler Effect | Present | `upe_acoustics.h` | f' = f(v±v_l)/(v∓v_s) |
| Acoustic Reflection/Reverb | Present | `upe_acoustics.h` | FDTD boundary conditions |
| Refraction | Present | `upe_optics.h` | Snell's law index of refraction |
| Diffraction | Present | `upe_acoustics.h` | Huygens-Fresnel aperture spreading |
| Interference | Present | `upe_waves.h` | Wave superposition |
| Photo-electric Effect | Approximation | `upe_optics.h` | E=hf threshold; electron emission probability |
| Albedo | Present | `upe_optics.h` | Per-material reflectance (0–1) for thermal coupling |
| Erosion | Present | `upe_erosion.h` | Hydraulic + thermal erosion; Stokes sedimentation |
| Sedimentation | Present | `upe_erosion.h` | Settling velocity in fluid grid; deposition accumulation |
| Combustion | Present | `upe_combustion.h` | Grid-based fuel/oxidizer reaction + heat diffusion |

---

## V. Vehicle & Machinery Physics

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| Tire Friction (Pacejka) | Present | `upe_advanced_vehicle.h` | Simplified Magic Formula; temperature-dependent grip |
| Torque Curves | Present | `upe_advanced_vehicle.h` | RPM-torque engine map |
| Differential Slip | Present | `upe_advanced_vehicle.h` | Open/LSD/torque-vectoring differential model |
| Aerodynamic Downforce | Present | `upe_advanced_vehicle.h` | Inverted lift coefficient term |
| Drift / Slip Angle | Present | `upe_advanced_vehicle.h` | Lateral friction loss, slide factor |
| Weight Transfer | Present | `upe_advanced_vehicle.h` | Pitch/roll chassis dynamics |
| Ackermann Steering | Present | `upe_vehicle.h` | Geometry-correct steering |
| Turbocharger Lag | Present | `upe_advanced_vehicle.h` | First-order RPM-based spool time constant |
| Clutch Friction | Present | `upe_advanced_vehicle.h` | Slip-to-lock kinetic clutch model |
| Brake Fade | Present | `upe_advanced_vehicle.h` | Temperature-dependent brake coefficient decay |
| Hydroplaning | Present | `upe_advanced_vehicle.h` | Critical speed detection; friction → near-zero |
| Gyroscopic Stabilization | Present | `upe_body.h` | τ_gyro = ω × (I·ω) in angular dynamics |
| Center of Mass | Present | `upe_body.h` | CoM explicit in body descriptor |
| Moment of Inertia | Present | `upe_body.h` | Inertia tensor computed per-shape |
| Ballistics | Present | `upe_ballistics.h` | Projectile integration; Cd·A drag; range equation |
| Rifling Stability | Present | `upe_ballistics.h` | Gyroscopic stability factor Sg; dynamic stability |
| Ricochet Logic | Present | `upe_ballistics.h` | Angle vs. critical angle; partial reflection |
| Spalling | Present | `upe_destruction.h` | Stress-wave internal fragmentation |
| Crumple Zones | Present | `upe_destruction.h` | Progressive buckling energy absorption model |
| G-Force Limits | Present | `upe_advanced_vehicle.h` | Sustained-g physiological callback |

---

## VI. Biomechanics & Character Physics

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| Locomotion Balance (CoP) | Present | `upe_biomechanics.h` | Center-of-pressure balance recovery |
| Muscle Contraction | Present | `upe_biomechanics.h` | Hill muscle model: contractile + series elastic |
| Fatigue Modeling | Present | `upe_biomechanics.h` | Force output decay integral |
| Skeleton Flex | Present | `upe_biomechanics.h` | Per-bone FEM-lite compliance under peak load |
| Soft Tissue Jiggle | Present | `upe_softbody.h` | Secondary Verlet chain driven by bone acceleration |
| Eye Dilation | Approximation | `upe_biomechanics.h` | Pupillary light reflex curve |
| Lung Capacity / Buoyancy | Present | `upe_biomechanics.h` | Inhale/exhale air volume → buoyancy offset |
| Hair / Fur Dynamics | Present | `upe_hair.h` | Strand-based Verlet with collision capsules |
| Bone Fracture | Present | `upe_destruction.h` | Peak-force threshold on articulated bone bodies |
| Kinetic Energy Damage | Present | `upe_body.h` | ½mv² exposed via impact callbacks |

---

## VII. Astronomic & Galactic Scale

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| N-Body Gravity | Present | `upe_gravity_fields.h` | Multi-body gravitational interactions |
| Keplerian Orbits | Present | `upe_orbital_mechanics.h` | Vis-Viva, elliptical paths |
| Lagrange Points | Present | `upe_orbital_mechanics.h` | L1–L5 for three-body system |
| Roche Limit | Present | `upe_orbital_mechanics.h` | Fluid + rigid Roche limit formulas |
| Tidal Locking | Present | `upe_orbital_mechanics.h` | Synchronous torque model |
| Time Dilation | Present | `upe_relativistic_physics.h` | Lorentz factor |
| Gravitational Redshift | Present | `upe_relativistic_physics.h` | GR wavelength shift |
| Event Horizon Dynamics | Present | `upe_relativistic_physics.h` | Schwarzschild capture/orbit condition |
| Spaghettification | Present | `upe_relativistic_physics.h` | Tidal acceleration gradient stretch force |
| Solar Wind Pressure | Present | `upe_space_physics.h` | Radiation pressure P = I/c |
| Nebular Accretion | Approximation | `upe_space_physics.h` | Jeans criterion; angular momentum collapse stub |
| Supernova Shockwaves | Approximation | `upe_space_physics.h` | Sedov-Taylor blast wave pressure-distance |
| Orbital Decay | Present | `upe_orbital_mechanics.h` | Atmospheric drag Δa per orbit |
| Precession | Present | `upe_orbital_mechanics.h` | Apsidal precession + gyroscope precession |
| Nutation | Present | `upe_orbital_mechanics.h` | Euler nutation equations |
| Asteroid Impact Mechanics | Present | `upe_destruction.h` | Pi-scaling crater law; ejecta blob spawn |
| Black Hole Lensing | Approximation | `upe_relativistic_physics.h` | Null geodesic deflection α = 4GM/rc² |
| Wormhole Metrics | Approximation | `upe_relativistic_physics.h` | Morris-Thorne throat stub; portal teleport |
| Dark Matter Influence | Approximation | `upe_gravity_fields.h` | NFW density profile halo contribution |
| Cosmic Inflation | Approximation | `upe_space_physics.h` | Scale factor a(t) stub; Hubble expansion |

---

## VIII. Electromagnetism & Electronics

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| Magnetic Flux | Present | `upe_electromagnetism.h` | B-field grid |
| Lorentz Force | Present | `upe_electromagnetism.h` | F = q(E + v×B); Boris integrator |
| Ohmic Heating | Present | `upe_electromagnetism.h` | J²/σ heat coupled to thermal grid |
| Eddy Currents | Present | `upe_electromagnetism.h` | Faraday induction braking torque |
| EMP | Present | `upe_electromagnetism.h` | Pulsed EM_SENSITIVE body disruption |
| ESD (Arcing/Lightning) | Present | `upe_electromagnetism.h` | Charge arc breakdown, bolt path |
| Ferromagnetism | Present | `upe_electromagnetism.h` | B-H hysteresis saturation; attraction force |
| Inductance | Present | `upe_electromagnetism.h` | L = μN²A/l; energy W = ½LI² |
| Capacitive Discharge | Present | `upe_electronics.h` | RC circuit V(t)=V₀e^(-t/RC) |
| Circuit Logic | Present | `upe_electronics.h` | Node-voltage resistor network; switch events |

---

## IX. Exotic & Game-Specific Physics

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| Non-Euclidean Space | Present | `upe_non_euclidean.h` | Metric tensor zone override; geodesic distance |
| Reverse Gravity | Present | `upe_gravity_fields.h` | Gravity vector inversion field |
| Time Reversal | Present | `upe_replay.h` | State-history playback |
| Teleportation Conservation | Present | `upe_portal.h` | Momentum/angular momentum preservation on teleport |
| Portal Momentum | Present | `upe_portal.h` | Connected-portal velocity re-orientation |
| Zero-G Inertia | Present | `upe_body.h` | Gravity = 0 world setting |
| Anti-Gravity | Present | `upe_gravity_fields.h` | Repulsion field |
| Density Shifting | Present | `upe_body.h` | Runtime density change; PHASE_THROUGH flag |
| Magnetized Boots | Present | `upe_character_controller.h` | Surface-normal gravity via EM attachment |
| Scaling Physics (Square-Cube Law) | Present | `upe_math_toolbox.h`, `upe_body.h` | ScaleBody() adjusts mass, inertia, strength |
| Void Pressure | Present | `upe_trigger_system.h` | Vacuum hazard sustained-pressure damage |
| Force Fields | Present | `upe_field_system.h` | Variable-threshold exclusion volumes |
| Gravity Wells | Present | `upe_gravity_fields.h` | Point-source attraction |
| Slow-Motion Friction | Present | `upe_world.h` | Time-scale factor on damping terms |
| Matter Synthesis | Present | `upe_emitter.h` | Physics-spawning emitter |
| Chain Reactions | Present | `upe_combustion.h` | Propagating force triggers |
| Destructible Terrain | Present | `upe_destruction.h` | Heightfield + voxel structural loss |
| Dynamic Weathering | Present | `upe_weathering.h` | Time-indexed material decay |
| Kinetic Batteries | Present | `upe_weathering.h` | Flywheel E=½Iω² charge/discharge API |
| Singularity Mechanics | Present | `upe_gravity_fields.h` | Point-source inward spiral; softened event horizon |

---

## X. Linear Algebra & Vector Calculus

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| Dot Product | Present | `upe_math.h` | `UpeVec3_Dot` |
| Cross Product | Present | `upe_math.h` | `UpeVec3_Cross` |
| Scalar Triple Product | Present | `upe_math.h` | `UpeVec3_ScalarTriple` |
| Matrix Determinants | Present | `upe_math.h` | `UpeMat3_Det`, `UpeMat4_Det` |
| Eigenvalues / Eigenvectors | Present | `upe_math.h` | `UpeMat3_EigenSymmetric` (power iteration) |
| Quaternions | Present | `upe_math.h` | Full algebra + SLERP |
| SLERP / NLERP | Present | `upe_math.h` | `UpeQuat_Slerp`, `UpeQuat_Nlerp` |
| Orthonormalization | Present | `upe_math.h` | Gram-Schmidt; also in determinism FP correction |
| Barycentric Coordinates | Present | `upe_math.h` | `UpeVec3_Barycentric` |
| Change of Basis | Present | `upe_math.h` | Transform matrix construction |
| Affine Transformations | Present | `upe_math.h` | UpeMat4 compose/decompose |
| Skew-Symmetric Matrices | Present | `upe_math.h` | `UpeMat3_Skew` cross-product matrix form |
| Projection Matrices | Present | `upe_rendering_math.h` | Perspective + orthographic |
| Tensor Math | Present | `upe_math.h` | 3×3 inertia tensor; stress tensor in FEM |
| Gradient Vectors | Present | `upe_fluid_sph.h` | SPH kernel gradient |

---

## XI. Collision Detection & Geometry

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| SAT | Present | `upe_narrowphase_scalar.h` | Box-box, box-sphere, box-capsule |
| GJK | Present | `upe_gjk.h` | Convex distance algorithm |
| EPA | Present | `upe_epa.h` | Penetration depth from GJK |
| AABB | Present | `upe_broadphase_scalar.h` | SAP fast reject |
| OBB | Present | `upe_obb.h` | Oriented bounding box |
| Bounding Spheres | Present | `upe_narrowphase_scalar.h` | Sphere-sphere distance check |
| Ray-Casting | Present | `upe_queries.h` | Single + batch raycasts |
| Sweep-and-Prune | Present | `upe_broadphase_sap.h` | Default broadphase |
| Octree / Quadtree | Present | `upe_octree.h` | Explicit spatial partition alongside BVH |
| BVH | Present | `upe_bvh.h` | Tree-based trimesh + raycast acceleration |
| Minkowski Difference | Present | `upe_gjk.h` | Used internally in GJK |
| CCD | Present | `upe_ccd.h`, `upe_ccd_advanced.h` | Tunneling prevention |
| Convex Hull Generation | Present | `upe_quickhull.h` | QuickHull algorithm |
| Triangle Meshing / Delaunay | Present | `upe_math_toolbox.h` | `UpeMath_Delaunay2D` |
| SDF | Present | `upe_sdf.h` | Analytic primitives; gradient; collision query |

---

## XII. Solvers & Integration

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| Euler Integration | Present | `upe_integrator.h` | Semi-implicit forward Euler |
| Verlet Integration | Present | `upe_softbody.h` | Position-history Verlet |
| RK4 | Present | `upe_math_toolbox.h` | `UpeMath_RK4Step` generic ODE integrator |
| Jacobian Matrices | Present | — | Internal to PGS / FEM |
| Lagrange Multipliers | Present | `upe_solver.h` | Contact + joint constraint solving |
| Impulse-Based Dynamics | Present | `upe_solver.h` | Core PGS impulse loop |
| Baumgarte Stabilization | Present | `upe_solver_stabilization.h` | Drift correction |
| PGS | Present | `upe_solver.h` | Primary solver |
| LCP | Present | `upe_solver.h` | PGS solves the LCP |
| Sleeping / Resting States | Present | `upe_body.h` | Island-based sleep |
| Island Building | Present | `upe_world.h` | Connected-body grouping |
| Warm Starting | Present | `upe_warmstart.h` | Frame-to-frame impulse carryover |
| Sequential Impulses | Present | `upe_solver.h` | Per-constraint iterative solve |
| Mass Normalization | Present | `upe_solver.h` | Inverse mass weighting |
| Sub-stepping | Present | `upe_world.h` | Configurable substep count |
| Impulse Capping | Present | `upe_solver.h` | `max_impulse` per-constraint clamp |

---

## XIII. Optimization & CS Logic

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| SIMD | Present | `upe_simd.h` | SSE/AVX2/AVX-512 abstraction |
| Fixed-Point Math | Present | `upe_fixed_q48_16.h` | Q48.16 for deterministic math |
| Floating-Point Precision | Present | `upe_determinism.h` | FP mode control; denormal flushing |
| DOPs | Present | `upe_obb.h` | k-DOP (k=6/14/18/26) bounding volumes |
| Lookup Tables (LUTs) | Present | `upe_math_toolbox.h` | Sin/cos LUT at configurable precision |
| Bitmasking | Present | `upe_queries.h` | Collision layer/mask filtering |
| Data-Oriented Design | Present | — | SOA layout in broadphase; cache-coherent |
| Dead Reckoning | Present | `upe_net_delta.h` | Linear/angular extrapolation + snap-blend |
| Determinism | Present | `upe_determinism.h` | Bit-exact replay; CRC64 hashing |
| Ownership Logic | Present | `upe_net_delta.h` | Body authority ID + transfer flags |

---

## XIV. Advanced Real-World Approximations

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| Coefficient of Restitution | Present | `upe_body.h` | Per-body restitution 0–1 |
| Friction Cones | Present | `upe_friction.h` | Tangential impulse bound |
| Static vs. Kinetic Friction | Present | `upe_friction.h` | Two-coefficient model |
| Rolling Resistance | Present | `upe_friction.h` | Crr-based energy-loss torque |
| Hooke's Law | Present | `upe_joint.h` | F = -kx spring joint |
| Damping Ratio | Present | `upe_joint.h` | Under/over/critically damped spring |
| Buoyancy Integration | Present | `upe_buoyancy.h` | Submerged volume per shape type |
| Aerodynamic Lift Coefficient | Present | `upe_aerodynamics.h` | L = Cl·½ρv²A |
| Navier-Stokes (projection) | Present | `upe_flip.h` | Explicit incompressible projection in FLIP |
| SPH | Present | `upe_fluid_sph.h` | Full SPH with kernel gradients |
| Manifold Generation | Present | `upe_manifold_cache.h` | Contact area between shapes |
| Feature Clipping (Sutherland-Hodgman) | Present | `upe_narrowphase_scalar.h` | Polygon clip for contact points |
| Heat Transfer Equation | Present | `upe_thermal.h` | Fourier's Law grid diffusion |
| A* Pathfinding | Present | `upe_pathfinding.h` | Physics-aware A* on navigation graph |
| Bezier Curves | Present | `upe_math_toolbox.h` | Cubic/quadratic Bézier; arc-length param |

---

## XV. Visuals & Debugging Math (Utility-Only)

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| Frustum Culling | Utility | `upe_rendering_math.h` | AABB + sphere frustum tests |
| Depth Buffering (z-math) | Utility | `upe_rendering_math.h` | Linearize depth; NDC utilities |
| Normal Mapping Math | Utility | `upe_rendering_math.h` | TBN matrix; tangent-space normal transform |
| Fresnel Effect | Utility | `upe_optics.h` | Schlick approx + full Fresnel equations |
| Vertex Skinning | Utility | `upe_rendering_math.h` | LBS + DQS bone-weight blending |
| Inverse Square Law | Utility | `upe_math_toolbox.h` | Gravity/light falloff helpers |
| Perlin / Simplex Noise | Utility | `upe_noise.h` | Seeded; octave fBm |
| Voronoi Diagrams | Utility | `upe_math_toolbox.h` | Fortune's 2D; used in fracture pattern gen |
| Fractal Dimension | Utility | `upe_math_toolbox.h` | Box-counting; Hausdorff estimate |
| P-Values / Probability | Utility | `upe_rng.h` | Seeded deterministic RNG |

---

## XVI. High-Level Logic Gates

| Feature | Status | Module | Notes |
|---------|--------|--------|-------|
| State Machines | Present | `upe_animation_state_machine.h` | Animation → ragdoll transitions |
| Trigger Volumes | Present | `upe_trigger_system.h` | Enter/Stay/Exit callbacks |
| Event Callbacks | Present | `upe_trigger_system.h`, `upe_joint.h` | `on_break`, `on_collision`, etc. |
| Ownership Logic | Present | `upe_net_delta.h` | Authority ID; transfer handshake |
| Timestamping | Present | `upe_world.h` | Physics frame counter |
| Constraint Softness | Present | `upe_joint.h` | CFM compliance parameter |
| Limit Logic | Present | `upe_joint_limits.h` | Range-of-motion clamping |
| Impulse Capping | Present | `upe_solver.h` | Per-constraint max_impulse clamp |
| Penetration Resolution | Present | `upe_solver.h` | PGS separation impulse |
| Square-Cube Law | Present | `upe_math_toolbox.h` | `UpeScale_MassFromRadius`, `UpeScale_StrengthFromRadius` |
