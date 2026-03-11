# Fluid Simulation Guide

Status: Fully Implemented (Phase 4-5)
Last verified: 2026-02-18
API: `upe_fluid_sph.h`, `upe_buoyancy.h`, integrated in `UpeEngine_Step()`

## Overview

The Unified Physics Engine includes a comprehensive SPH (Smoothed Particle Hydrodynamics) fluid simulation system with advanced features including:

- **SPH Fluid Dynamics**: Pressure, viscosity, and surface tension forces
- **Buoyancy System**: Bodies floating in fluids with drag
- **Fluid-Body Coupling**: Two-way momentum transfer between fluids and rigid bodies
- **Continuous Collision Detection (CCD)**: Prevents tunneling for fast-moving objects

## SPH Fluid System

### Basic Setup

```c
#include "upe/upe_fluid_sph.h"

// Create fluid system
UpeFluidSph fluid;
UpeFluidSph_Init(&fluid, 1000);  // Max 1000 particles

// Set bounds and gravity
UpeVec3 bounds_min = {-5, 0, -5};
UpeVec3 bounds_max = {5, 10, 5};
UpeFluidSph_SetBounds(&fluid, bounds_min, bounds_max);

UpeVec3 gravity = {0, -9.81f, 0};
UpeFluidSph_SetGravity(&fluid, gravity);

// Add particles
for (int i = 0; i < 100; i++) {
    UpeVec3 pos = {i * 0.1f, 5.0f, 0};
    UpeVec3 vel = {0, 0, 0};
    UpeFluidSph_AddParticle(&fluid, pos, vel);
}

// Simulation loop
float dt = 0.016f;  // 60 FPS
UpeFluidSph_Step(&fluid, dt);

// Cleanup
UpeFluidSph_Destroy(&fluid);
```

### Fluid Parameters

The SPH system has several tunable parameters:

- **rest_density**: Target density of the fluid (default: 1000 kg/m³ for water)
- **gas_constant**: Pressure stiffness (higher = more incompressible)
- **viscosity**: Resistance to flow (default: 0.001 for water)
- **surface_tension**: Cohesion at the surface (default: 0.0728 N/m for water)
- **smoothing_radius**: Interaction radius between particles

### Surface Tension

Surface tension is computed using the color field method:

1. **Color Field Gradient**: Computed at each particle using Poly6 kernel gradient
2. **Curvature**: Calculated from the Laplacian of the color field
3. **Tension Force**: Applied proportional to curvature at the surface

```c
// Adjust surface tension coefficient
fluid.surface_tension = 0.1f;  // Higher = stronger cohesion
```

### Spatial Hashing

The SPH system uses a uniform grid for efficient neighbor queries:

- **Grid Cell Size**: Equal to the smoothing radius
- **Hash Function**: Spatial 3D grid mapping
- **Neighbor Search**: 27-cell stencil (3x3x3)

## Buoyancy System

### Applying Buoyancy to Bodies

```c
#include "upe/upe_buoyancy.h"

// Setup buoyancy parameters
UpeBuoyancyDesc desc = {0};
desc.fluid_density = 1000.0f;  // Water density
desc.fluid_surface_height = 3.0f;
desc.fluid_surface_normal.x = 0;
desc.fluid_surface_normal.y = 1;
desc.fluid_surface_normal.z = 0;
desc.drag_coefficient = 0.8f;
desc.angular_drag = 0.5f;
desc.sample_points = 27;

// Apply to a body
UpeBuoyancyState state;
UpeBuoyancy_ApplyUniform(&body, &desc, &state);

// Access computed values
printf("Submerged Volume: %.3f m³\n", state.submerged_volume);
printf("Buoyancy Force: (%.2f, %.2f, %.2f) N\n",
       state.buoyancy_force.x,
       state.buoyancy_force.y,
       state.buoyancy_force.z);
```

### Buoyancy Calculation

Buoyancy forces are calculated based on:

1. **Submerged Volume Estimation**: Sample points within the body
2. **Archimedes' Principle**: Buoyancy = ρ * V * g
3. **Drag Forces**: Proportional to velocity and submerged area

### Sample Points

The `sample_points` parameter controls accuracy:

- **8 points**: Fast, low accuracy (corners only)
- **27 points**: Good balance (corners + edges + center)
- **64 points**: High accuracy, slower

## Fluid-Body Coupling

Two-way momentum transfer between fluids and rigid bodies:

```c
#include "upe/upe_fluid_coupling.h"

// Initialize coupling system
UpeFluidCoupling coupling;
UpeFluidCoupling_Init(&coupling, &fluid, &world);

// Update each frame
float dt = 0.016f;
UpeFluidCoupling_Update(&coupling, dt);
```

### Coupling Algorithm

1. **Body-to-Fluid**: Bodies repel nearby particles based on penetration depth
2. **Fluid-to-Body**: Reaction forces applied to bodies (momentum conservation)
3. **Wetness Tracking**: Bodies track fluid contact for rendering effects

### Interaction Radius

The coupling radius is computed as:

```
interaction_radius = body_radius + fluid_smoothing_radius
```

### Force Application

- **Repulsion Force**: `k * penetration_depth * normal`
- **Reaction Force**: Equal and opposite to repulsion (Newton's 3rd law)
- **Impulse Scaling**: `force / body_mass` for proper dynamics

## Continuous Collision Detection (CCD)

Prevents fast-moving objects from tunneling through each other.

### Basic CCD

```c
#include "upe/upe_ccd.h"

// Sphere-Sphere CCD
UpeVec3 center_a = {0, 0, 0};
UpeVec3 center_b = {5, 0, 0};
UpeVec3 vel_a = {10, 0, 0};
UpeVec3 vel_b = {-10, 0, 0};
float radius_a = 1.0f;
float radius_b = 1.0f;
float dt = 0.1f;

UpeCcdResult result = UpeCcd_SphereSphere(
    center_a, radius_a, vel_a,
    center_b, radius_b, vel_b,
    dt
);

if (result.hit) {
    printf("Collision at t=%.3f\n", result.toi);
    printf("Position: (%.2f, %.2f, %.2f)\n",
           result.position.x, result.position.y, result.position.z);
    printf("Normal: (%.2f, %.2f, %.2f)\n",
           result.normal.x, result.normal.y, result.normal.z);
}
```

### Advanced CCD

For complex shapes, use the advanced CCD with conservative advancement:

```c
#include "upe/upe_ccd_advanced.h"

// Box-Box CCD
UpeAabb3 box_a = {{-1, -1, -1}, {1, 1, 1}};
UpeAabb3 box_b = {{3, -1, -1}, {5, 1, 1}};
UpeVec3 vel_a = {5, 0, 0};
UpeVec3 vel_b = {-5, 0, 0};

UpeCcdResult result = UpeCcd_BoxBox(&box_a, vel_a, &box_b, vel_b, dt);

// Convex-Convex CCD (using GJK)
UpeCcdResult result = UpeCcd_ConvexConvex(
    shape_a, support_fn_a, vel_a,
    shape_b, support_fn_b, vel_b,
    dt
);
```

### Conservative Advancement

The algorithm uses binary search to find the exact time of impact (TOI):

1. **Initial Check**: Test if shapes overlap at t=0 or t=dt
2. **Binary Search**: Narrow down TOI to within tolerance (0.001)
3. **Contact Normal**: Computed from witness points or center directions
4. **Iteration Limit**: Maximum 20 iterations for convergence

### Supported Shape Pairs

| Shape A     | Shape B     | Function                  |
|-------------|-------------|---------------------------|
| Sphere      | Sphere      | `UpeCcd_SphereSphere`     |
| Sphere      | Capsule     | `UpeCcd_SphereCapsule`    |
| Sphere      | Box         | `UpeCcd_SphereBox`        |
| Box         | Box         | `UpeCcd_BoxBox`           |
| Convex      | Convex      | `UpeCcd_ConvexConvex`     |

## Python Bindings

```python
from bindings.python.upe_bindings import UpeBindings

upe = UpeBindings()

# Create fluid
fluid = upe.create_fluid(max_particles=1000)
upe.set_fluid_bounds(fluid, (-5, 0, -5), (5, 10, 5))
upe.set_fluid_gravity(fluid, (0, -9.81, 0))

# Add particles
for i in range(10):
    for j in range(10):
        upe.add_fluid_particle(fluid, (i * 0.1, j * 0.1 + 1, 0))

# Simulate
for i in range(100):
    upe.step_fluid(fluid, 0.016)

# CCD
result = upe.ccd_sphere_sphere(
    center_a=(0, 0, 0), radius_a=1.0, vel_a=(10, 0, 0),
    center_b=(5, 0, 0), radius_b=1.0, vel_b=(-10, 0, 0),
    dt=1.0
)
print(f"Collision: {result['hit']}, TOI: {result['toi']}")

upe.destroy_fluid(fluid)
```

## C# Bindings

```csharp
using UpeSharp;

// Create fluid
using (var fluid = new Upe.FluidSph(maxParticles: 1000))
{
    fluid.SetBounds(new Vec3(-5, 0, -5), new Vec3(5, 10, 5));
    fluid.SetGravity(new Vec3(0, -9.81f, 0));

    // Add particles
    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            fluid.AddParticle(new Vec3(i * 0.1f, j * 0.1f + 1, 0));
        }
    }

    // Simulate
    for (int i = 0; i < 100; i++)
    {
        fluid.Step(0.016f);
    }
}

// CCD
var result = Upe.Ccd.SphereSphere(
    new Vec3(0, 0, 0), 1.0f, new Vec3(10, 0, 0),
    new Vec3(5, 0, 0), 1.0f, new Vec3(-10, 0, 0),
    1.0f
);
Console.WriteLine($"Collision: {result.hit != 0}, TOI: {result.toi}");
```

## Performance Optimization

### Spatial Hashing

- Grid size affects performance: smaller = more overhead, larger = more checks
- Optimal grid size ≈ 2 × smoothing_radius
- Max neighbors per cell: 64 (UPE_SPH_MAX_NEIGHBORS)

### Particle Count

Performance scales roughly as O(N log N) due to spatial hashing:

| Particles | Frame Time (60 FPS) | Realistic? |
|-----------|---------------------|------------|
| 100       | < 1 ms              | ✓          |
| 1,000     | ~5 ms               | ✓          |
| 10,000    | ~50 ms              | ✗          |

### Optimization Tips

1. **Reduce Smoothing Radius**: Smaller = fewer neighbors
2. **Lower Surface Tension**: Disable if not needed
3. **Coarser Time Steps**: dt = 0.033f (30 FPS) vs 0.016f (60 FPS)
4. **Boundary Culling**: Remove particles outside bounds
5. **Adaptive Resolution**: Use fewer particles in low-detail areas

## Common Issues

### Particles Exploding

**Cause**: Gas constant too high or time step too large

**Solution**:
```c
fluid.gas_constant = 1000.0f;  // Lower value
dt = 0.008f;  // Smaller time step
```

### Particles Clumping

**Cause**: Viscosity too high or surface tension too strong

**Solution**:
```c
fluid.viscosity = 0.0001f;  // Lower viscosity
fluid.surface_tension = 0.01f;  // Reduce surface tension
```

### Bodies Floating Too High

**Cause**: Body mass too low or drag too high

**Solution**:
```c
body.mass = 50.0f;  // Increase mass
desc.drag_coefficient = 0.2f;  // Reduce drag
```

### CCD Missing Collisions

**Cause**: Velocities too high or tolerance too coarse

**Solution**:
```c
// Use smaller time steps
dt = 0.008f;  // Instead of 0.016f

// Or adjust CCD tolerance in source:
const float tolerance = 0.0001f;  // More precise (default: 0.001)
```

## Next Steps

- **Visualization**: Render particles as spheres or metaballs
- **Multi-Phase Fluids**: Multiple fluid types with different densities
- **Turbulence**: Add vorticity confinement for swirling effects
- **GPU Acceleration**: Port SPH kernels to compute shaders
- **Fluid Emitters**: Continuous particle spawning from sources

## References

- Müller et al. "Particle-Based Fluid Simulation" (SIGGRAPH 2003)
- Macklin & Müller "Position Based Fluids" (SIGGRAPH 2013)
- Van den Bergen "Ray Casting against General Convex Objects with Application to Continuous Collision Detection" (GDC 2005)
