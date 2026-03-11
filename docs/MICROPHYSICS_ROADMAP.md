# Microphysics Roadmap

This document describes the design and implementation plan for the `microphysics/` domain in UPE — covering atomic, molecular, and sub-microscopic physical phenomena.

> **Approximation note:** All features in this domain are **scientific-approximation** tier. Real-time sub-atomic simulation is not physically realistic. Models here use correct mathematical formulas as close-form approximations that produce physically plausible results for gameplay and visualization purposes.

---

## Scope

The microphysics domain provides:

- Stochastic Brownian motion for particle systems
- Molecular inter-particle forces (Van der Waals, covalent bonds, Pauli repulsion)
- Quantum-mechanical probability approximations (tunneling, electron cloud)
- Lattice vibrational modes (phonons)
- Micro-fluidic capillary behavior
- Ion channel gating for biological simulation
- Nuclear energy release approximations

---

## Module

**Header:** `core/include/upe/upe_molecular_physics.h`  
**Implementation:** `core/src/world/upe_molecular_physics.cpp`

---

## Feature Breakdown

### 1. Brownian Motion
**Model:** Stochastic velocity kicks from the deterministic RNG at each step.  
**Formula:** Random force per particle: $F_i = \sqrt{2 k_B T \gamma / \Delta t} \cdot N(0,1)$  
**Determinism class:** Seeded stochastic — deterministic per seed.

### 2. Van der Waals Forces
**Model:** Lennard-Jones 6-12 potential.  
**Formula:** $V(r) = 4\varepsilon \left[(\sigma/r)^{12} - (\sigma/r)^6\right]$  
**Use:** Pairwise inter-particle attraction/repulsion. Short-range repulsion, long-range attraction.

### 3. Electron Cloud Probability
**Model:** Spatial probability density field over a grid.  
**Formula:** Simplified hydrogen-like radial distribution $P(r) \propto r^2 e^{-2r/a_0}$  
**Use:** Modulates collision density for particle-scale interaction.

### 4. Lattice Vibrations (Phonons)
**Model:** Coupled harmonic oscillator chain.  
**Formula:** Dispersion relation $\omega(k) = 2\sqrt{K/m} \cdot |\sin(ka/2)|$  
**Use:** Normal mode oscillation in solid lattices; heat conduction proxy.

### 5. Micro-fluidics / Capillary Action
**Model:** Young-Laplace pressure with contact angle.  
**Formula:** $\Delta P = \frac{2\gamma \cos\theta}{r}$ where $\gamma$ is surface tension, $\theta$ is contact angle, $r$ is channel radius.  
**Use:** Pressure-driven flow in narrow channels; extends SPH surface tension system.

### 6. Ion Channel Gating
**Model:** Hodgkin-Huxley-inspired probabilistic gate.  
**Formula:** Gate probability transitions as a function of membrane voltage:  
$\alpha_n(V) = 0.01(V+55)/(1-e^{-(V+55)/10})$  
**Use:** Biological cell membrane ion passage stub.

### 7. Nuclear Fission/Fusion
**Model:** Q-value energy release approximation.  
**Formula (fission):** $E = (m_{reactants} - m_{products}) c^2 = Q$  
**Use:** Energy burst trigger; chain reaction propagation event. No cross-section modeling.

### 8. Quantum Tunneling
**Model:** WKB (Wentzel-Kramers-Brillouin) barrier penetration.  
**Formula:** Transmission probability $T \approx e^{-2\kappa d}$ where $\kappa = \sqrt{2m(V_0-E)}/\hbar$  
**Use:** Returns a stochastic bypass probability; applied as a collision phase-through event.

### 9. Pauli Exclusion (Hard Repulsion)
**Model:** Very short-range exponential repulsion below atomic separation.  
**Formula:** $F(r) = A \cdot e^{-r/\rho}$ where $\rho$ is repulsion range (~0.03 Å equivalent).  
**Use:** Prevents overlap in molecular particle simulations; complement to Lennard-Jones.

### 10. Covalent Bond Tension
**Model:** Morse potential between bonded particles.  
**Formula:** $V(r) = D_e (1 - e^{-a(r-r_e)})^2$ where $D_e$ is bond dissociation energy.  
**Use:** Bond stretching with realistic break threshold; creates snap event on dissociation.

---

## Determinism

All stochastic processes (Brownian, ion gating, tunneling, nuclear) use `upe_rng` seeded RNG. Deterministic per-seed across repeated runs (Level 1 determinism). Not cross-platform bit-exact (Level 3/4 not claimed).

---

## Integration with Other Domains

| Link | Description |
|------|-------------|
| SPH fluids | Micro-fluidic capillary pressure can feed into SPH surface tension coefficient |
| Thermal | Lattice vibration energy can contribute to thermal grid temperature |
| Combustion | Nuclear Q-value energy can trigger combustion chain reaction |
| Destruction | Covalent bond breakage can trigger fracture events |
| EM | Ionic charge carriers couple to EM field grid |

---

## Validation

These items are **not** part of the buyer proof path. They are tested for basic numerical stability but not for physical accuracy against experiment. The expected behavior is:

- Brownian motion produces visible diffusion in particle sims with correct diffusion coefficient scaling
- Lennard-Jones produces stable crystal lattice spacing at equilibrium
- Morse potential breaks bonds at the correct energy threshold
- WKB tunneling probabilities are in the correct order of magnitude for test cases
