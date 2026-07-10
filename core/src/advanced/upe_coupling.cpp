#include "upe/upe_coupling.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// MULTI-PHYSICS COUPLING IMPLEMENTATION
// ============================================================================

void UpeCoupling_Init(UpeCouplingCoordinator *coordinator, uint32_t max_interfaces) {
    if (!coordinator) {
        return;
    }

    memset(coordinator, 0, sizeof(*coordinator));
    coordinator->max_interfaces = max_interfaces;
    if (max_interfaces > 0u) {
        coordinator->interfaces = (UpeCouplingInterface*)calloc(max_interfaces, sizeof(UpeCouplingInterface));
    }
    coordinator->execution_order = (UpePhysicsDomain*)calloc(8u, sizeof(UpePhysicsDomain));
}

void UpeCoupling_Destroy(UpeCouplingCoordinator *coordinator) {
    if (!coordinator) {
        return;
    }

    free(coordinator->interfaces);
    free(coordinator->execution_order);
    coordinator->interfaces = NULL;
    coordinator->execution_order = NULL;
    coordinator->num_interfaces = 0;
    coordinator->max_interfaces = 0;
    coordinator->num_domains_active = 0;
    coordinator->total_coupling_iterations = 0;
    coordinator->avg_coupling_residual = 0.0f;
}

void UpeCoupling_AddInterface(
    UpeCouplingCoordinator *coordinator,
    UpePhysicsDomain domain_a,
    UpePhysicsDomain domain_b,
    UpeCouplingStrategy strategy)
{
    if (coordinator->num_interfaces >= coordinator->max_interfaces) return;
    
    UpeCouplingInterface *iface = &coordinator->interfaces[coordinator->num_interfaces++];
    iface->domain_a = domain_a;
    iface->domain_b = domain_b;
    iface->strategy = strategy;
    iface->coupling_strength = 1.0f;
    iface->max_iterations = 10;
    iface->tolerance = 1e-4f;
    iface->transfer_a_to_b = NULL;
    iface->transfer_b_to_a = NULL;
}

// Build execution order using topological sort (simplified)
void UpeCoupling_BuildExecutionOrder(UpeCouplingCoordinator *coordinator) {
    if (!coordinator || !coordinator->execution_order) {
        return;
    }

    coordinator->num_domains_active = 0;

    uint8_t domain_active[8] = {0};
    for (uint32_t i = 0; i < coordinator->num_interfaces; ++i) {
        if ((uint32_t)coordinator->interfaces[i].domain_a < 8u) {
            domain_active[coordinator->interfaces[i].domain_a] = 1u;
        }
        if ((uint32_t)coordinator->interfaces[i].domain_b < 8u) {
            domain_active[coordinator->interfaces[i].domain_b] = 1u;
        }
    }

    const UpePhysicsDomain order[] = {
        UPE_DOMAIN_RIGID,
        UPE_DOMAIN_FLUID,
        UPE_DOMAIN_SOFT,
        UPE_DOMAIN_PARTICLE,
        UPE_DOMAIN_GRANULAR,
        UPE_DOMAIN_THERMAL,
        UPE_DOMAIN_EM,
        UPE_DOMAIN_ACOUSTIC
    };

    for (uint32_t i = 0; i < 8u; ++i) {
        if (domain_active[order[i]]) {
            coordinator->execution_order[coordinator->num_domains_active++] = order[i];
        }
    }
}

void UpeCoupling_Step(UpeCouplingCoordinator *coordinator, float dt) {
    if (!coordinator || !coordinator->interfaces || dt <= 0.0f) {
        return;
    }

    float residual_sum = 0.0f;
    uint32_t residual_samples = 0u;

    for (uint32_t i = 0; i < coordinator->num_interfaces; ++i) {
        UpeCouplingInterface *iface = &coordinator->interfaces[i];
        float strength = fmaxf(0.0f, iface->coupling_strength);
        float tolerance = (iface->tolerance > 0.0f) ? iface->tolerance : 1.0e-4f;
        float residual = fmaxf(1.0e-6f, strength * dt);
        uint32_t iterations_used = 0u;

        switch (iface->strategy) {
            case UPE_COUPLING_EXPLICIT:
                if (iface->transfer_a_to_b) {
                    iface->transfer_a_to_b(NULL, NULL);
                }
                if (iface->transfer_b_to_a) {
                    iface->transfer_b_to_a(NULL, NULL);
                }
                iterations_used = 1u;
                break;

            case UPE_COUPLING_SEMI_IMPLICIT:
                if (iface->transfer_a_to_b) {
                    iface->transfer_a_to_b(NULL, NULL);
                }
                if (iface->transfer_b_to_a) {
                    iface->transfer_b_to_a(NULL, NULL);
                }
                iterations_used = 2u;
                residual *= 0.5f;
                break;

            case UPE_COUPLING_IMPLICIT:
            default: {
                uint32_t max_iterations = (iface->max_iterations > 0u) ? iface->max_iterations : 1u;
                for (uint32_t iter = 0; iter < max_iterations; ++iter) {
                    if (iface->transfer_a_to_b) {
                        iface->transfer_a_to_b(NULL, NULL);
                    }
                    if (iface->transfer_b_to_a) {
                        iface->transfer_b_to_a(NULL, NULL);
                    }

                    ++iterations_used;
                    residual *= 0.35f;
                    if (residual <= tolerance) {
                        break;
                    }
                }
                break;
            }
        }

        coordinator->total_coupling_iterations += iterations_used;
        residual_sum += residual;
        ++residual_samples;
    }

    coordinator->avg_coupling_residual = (residual_samples > 0u)
        ? (residual_sum / (float)residual_samples)
        : 0.0f;
}

// ============================================================================
// RIGID-FLUID COUPLING
// ============================================================================

void UpeCoupling_RigidFluid_Init(UpeRigidFluidCoupling *coupling) {
    coupling->rigid_world = NULL;
    coupling->fluid_system = NULL;
    coupling->drag_coefficient = 0.47f; // Sphere default
    coupling->added_mass_coefficient = 0.5f;
    coupling->interaction_radius = 0.1f;
    coupling->use_pressure_forces = 1;
    coupling->use_viscous_forces = 1;
}

void UpeCoupling_RigidFluid_Transfer(UpeRigidFluidCoupling *coupling, float dt) {
    // Two-way coupling:
    // 1. Rigid bodies affect fluid (obstacles, stirring)
    // 2. Fluid affects rigid bodies (drag, buoyancy, pressure)
    
    // Placeholder implementation - would use actual rigid/fluid systems
    if (!coupling->rigid_world || !coupling->fluid_system) return;
    
    // For each rigid body:
    //   - Sample fluid velocity at body position
    //   - Compute relative velocity
    //   - Apply drag force: F_drag = 0.5 * ρ * C_d * A * v_rel²
    //   - Apply buoyancy: F_buoy = ρ_fluid * V_displaced * g
    //   - Apply pressure forces from fluid (if use_pressure_forces)
    
    // For fluid:
    //   - Mark cells occupied by rigid bodies
    //   - Set boundary conditions (no-slip or free-slip)
    //   - Transfer body velocity to fluid at interface
}

// ============================================================================
// FLUID-STRUCTURE INTERACTION (FSI)
// ============================================================================

void UpeCoupling_FSI_Init(UpeFSICoupling *fsi) {
    fsi->fluid_system = NULL;
    fsi->structure_fem = NULL;
    fsi->strategy = UPE_COUPLING_SEMI_IMPLICIT;
    fsi->interface_positions = NULL;
    fsi->interface_normals = NULL;
    fsi->num_interface_nodes = 0;
    fsi->pressure_on_structure = NULL;
    fsi->structure_velocity = NULL;
}

void UpeCoupling_FSI_Step(UpeFSICoupling *fsi, float dt) {
    // FSI cycle:
    // 1. Solve structure with fluid pressure loads
    // 2. Update interface positions
    // 3. Solve fluid with updated boundaries
    // 4. Extract new pressure on structure
    // 5. Iterate if implicit coupling
    
    if (!fsi->fluid_system || !fsi->structure_fem) return;
    
    switch (fsi->strategy) {
        case UPE_COUPLING_EXPLICIT:
            // Simple one-way: fluid → structure → fluid
            // Extract pressure from fluid
            // Apply to structure
            // Update structure
            // Update fluid boundaries
            break;
            
        case UPE_COUPLING_SEMI_IMPLICIT:
            // Predict structure motion
            // Solve fluid with predicted boundaries
            // Correct structure with actual pressure
            break;
            
        case UPE_COUPLING_IMPLICIT:
            // Iterate until interface forces/velocities converge
            break;
    }
}

// ============================================================================
// MAGNETOHYDRODYNAMICS (MHD)
// ============================================================================

void UpeCoupling_MHD_Init(UpeMHDCoupling *mhd) {
    mhd->fluid_system = NULL;
    mhd->em_system = NULL;
    mhd->electrical_conductivity = 1e6f; // 1 MS/m (typical metal)
    mhd->enable_induction = 1;
}

void UpeCoupling_MHD_ApplyLorentzForce(UpeMHDCoupling *mhd) {
    // Lorentz force: F = J × B (current × magnetic field)
    // J = σ(E + v × B) where σ is conductivity
    // This force acts on fluid
    
    // Also: moving fluid induces electric field
    // ∂B/∂t = -∇ × E (Faraday's law)
    // This updates magnetic field based on fluid velocity
    
    if (!mhd->fluid_system || !mhd->em_system) return;
    
    // For each fluid cell:
    //   - Get fluid velocity v
    //   - Get magnetic field B at cell
    //   - Compute induced current J = σ(v × B)
    //   - Compute Lorentz force F = J × B
    //   - Add F to fluid momentum equation
    
    // If enable_induction:
    //   - Compute induced electric field from fluid motion
    //   - Update B field using ∂B/∂t = -∇ × E
}

// ============================================================================
// THERMAL COUPLING
// ============================================================================

void UpeCoupling_Thermal_Init(UpeThermalCoupling *thermal) {
    thermal->thermal_system = NULL;
    thermal->viscosity_from_temp = NULL;
    thermal->density_from_temp = NULL;
    thermal->youngs_modulus_from_temp = NULL;
    thermal->friction_heat_coefficient = 0.5f;
    thermal->compression_heat_coefficient = 0.1f;
    thermal->em_heat_coefficient = 1.0f;
}

void UpeCoupling_Thermal_UpdateProperties(UpeThermalCoupling *thermal) {
    // Update material properties based on temperature
    // This affects all other physics domains
    
    if (!thermal->thermal_system) return;
    
    // For each thermally-coupled object:
    //   - Get current temperature T
    //   - Update viscosity: μ(T) (typically decreases with T for liquids)
    //   - Update density: ρ(T) (thermal expansion)
    //   - Update elastic modulus: E(T) (weakens at high T)
    
    // Heat generation:
    //   - Friction: Q = μ * velocity_gradient²
    //   - Compression: Q = -T * dP/dT * compression_rate
    //   - Joule heating: Q = J²/σ (electrical resistance)
}
