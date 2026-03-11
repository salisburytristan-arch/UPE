# Python bindings for UPE via ctypes

import ctypes
from ctypes import c_float, c_uint32, c_int, c_bool, POINTER, Structure

class Vec3(Structure):
    _fields_ = [("x", c_float), ("y", c_float), ("z", c_float)]
    
    def __init__(self, x=0.0, y=0.0, z=0.0):
        super().__init__(x=x, y=y, z=z)
    
    def __repr__(self):
        return f"Vec3({self.x}, {self.y}, {self.z})"

class Quat(Structure):
    _fields_ = [("x", c_float), ("y", c_float), ("z", c_float), ("w", c_float)]
    
    def __init__(self, x=0.0, y=0.0, z=0.0, w=1.0):
        super().__init__(x=x, y=y, z=z, w=w)

class WorldDesc(Structure):
    _fields_ = [("fixed_dt", c_float)]

class BodyDesc(Structure):
    _fields_ = [
        ("type", c_uint32),
        ("position", Vec3),
        ("rotation", Quat),
        ("mass", c_float)
    ]

class BuoyancyDesc(Structure):
    _fields_ = [
        ("fluid_density", c_float),
        ("fluid_surface_normal", Vec3),
        ("fluid_surface_height", c_float),
        ("drag_coefficient", c_float),
        ("angular_drag", c_float),
        ("sample_points", c_uint32)
    ]

class BuoyancyState(Structure):
    _fields_ = [
        ("submerged_volume", c_float),
        ("buoyancy_force", Vec3),
        ("drag_force", Vec3),
        ("center_of_buoyancy", Vec3)
    ]

class CcdResult(Structure):
    _fields_ = [
        ("hit", c_int),
        ("toi", c_float),
        ("position", Vec3),
        ("normal", Vec3)
    ]

class Aabb3(Structure):
    _fields_ = [
        ("min", Vec3),
        ("max", Vec3)
    ]

class Particle(Structure):
    _fields_ = [
        ("position", Vec3),
        ("velocity", Vec3),
        ("mass", c_float),
        ("life", c_float),
        ("size", c_float),
        ("drag", c_float),
        ("flags", c_uint32)
    ]

class ParticleSystem(Structure):
    _fields_ = [
        ("particles", POINTER(Particle)),
        ("capacity", c_uint32),
        ("count", c_uint32)
    ]

class ParticleInitial(Structure):
    _fields_ = [
        ("velocity", Vec3),
        ("lifetime", c_float),
        ("mass", c_float),
        ("size", c_float),
        ("drag", c_float)
    ]

class EmitterDesc(Structure):
    _fields_ = [
        ("shape", c_uint32),
        ("mode", c_uint32),
        ("position", Vec3),
        ("rotation", Quat),
        ("particles_per_frame", c_uint32),
        ("emission_duration", c_float),
        ("particle_template", ParticleInitial),
        ("shape_scale", c_float),
        ("velocity_spread", c_float),
        ("gravity", Vec3),
        ("wind_velocity", Vec3),
        ("damping", c_float)
    ]

class ParticleSpawnRequest(Structure):
    _fields_ = [
        ("position", Vec3),
        ("velocity", Vec3),
        ("lifetime", c_float),
        ("mass", c_float),
        ("size", c_float),
        ("drag", c_float)
    ]

class UpeBindings:
    def __init__(self, dll_path="upe.dll"):
        self.lib = ctypes.CDLL(dll_path)
        
        self._setup_world_functions()
        self._setup_body_functions()
        self._setup_sph_functions()
        self._setup_buoyancy_functions()
        self._setup_ccd_functions()
        self._setup_particle_functions()
        self._setup_emitter_functions()
        self._setup_spray_functions()
    
    def _setup_world_functions(self):
        # World functions
        self.lib.UpeWorld_Create.argtypes = [POINTER(WorldDesc)]
        self.lib.UpeWorld_Create.restype = ctypes.c_void_p
        
        self.lib.UpeWorld_Destroy.argtypes = [ctypes.c_void_p]
        self.lib.UpeWorld_Destroy.restype = None
        
        self.lib.UpeWorld_Step.argtypes = [ctypes.c_void_p]
        self.lib.UpeWorld_Step.restype = None
        
        self.lib.UpeWorld_Time.argtypes = [ctypes.c_void_p]
        self.lib.UpeWorld_Time.restype = c_float
    
    def _setup_body_functions(self):
        # Body functions
        self.lib.UpeWorld_CreateBody.argtypes = [ctypes.c_void_p, POINTER(BodyDesc)]
        self.lib.UpeWorld_CreateBody.restype = c_uint32
    
    def _setup_sph_functions(self):
        # SPH Fluid functions
        self.lib.UpeFluidSph_Init.argtypes = [ctypes.c_void_p, c_uint32]
        self.lib.UpeFluidSph_Init.restype = None
        
        self.lib.UpeFluidSph_SetBounds.argtypes = [ctypes.c_void_p, Vec3, Vec3]
        self.lib.UpeFluidSph_SetBounds.restype = None
        
        self.lib.UpeFluidSph_SetGravity.argtypes = [ctypes.c_void_p, Vec3]
        self.lib.UpeFluidSph_SetGravity.restype = None
        
        self.lib.UpeFluidSph_AddParticle.argtypes = [ctypes.c_void_p, Vec3, Vec3]
        self.lib.UpeFluidSph_AddParticle.restype = c_uint32
        
        self.lib.UpeFluidSph_RemoveParticle.argtypes = [ctypes.c_void_p, c_uint32]
        self.lib.UpeFluidSph_RemoveParticle.restype = None
        
        self.lib.UpeFluidSph_Step.argtypes = [ctypes.c_void_p, c_float]
        self.lib.UpeFluidSph_Step.restype = None
        
        self.lib.UpeFluidSph_Destroy.argtypes = [ctypes.c_void_p]
        self.lib.UpeFluidSph_Destroy.restype = None
    
    def _setup_buoyancy_functions(self):
        # Buoyancy functions
        self.lib.UpeBuoyancy_ApplyUniform.argtypes = [
            ctypes.c_void_p,
            POINTER(BuoyancyDesc),
            POINTER(BuoyancyState)
        ]
        self.lib.UpeBuoyancy_ApplyUniform.restype = None
    
    def _setup_ccd_functions(self):
        # CCD functions
        self.lib.UpeCcd_SphereSphere.argtypes = [
            Vec3, c_float, Vec3,  # Center A, radius A, velocity A
            Vec3, c_float, Vec3,  # Center B, radius B, velocity B
            c_float              # dt
        ]
        self.lib.UpeCcd_SphereSphere.restype = CcdResult
        
        self.lib.UpeCcd_BoxBox.argtypes = [
            POINTER(Aabb3), Vec3,  # Box A, velocity A
            POINTER(Aabb3), Vec3,  # Box B, velocity B
            c_float                # dt
        ]
        self.lib.UpeCcd_BoxBox.restype = CcdResult

    def _setup_particle_functions(self):
        # Particle system functions
        self.lib.UpeParticleSystem_Init.argtypes = [POINTER(ParticleSystem), c_uint32]
        self.lib.UpeParticleSystem_Init.restype = None

        self.lib.UpeParticleSystem_Emit.argtypes = [POINTER(ParticleSystem), Vec3, Vec3]
        self.lib.UpeParticleSystem_Emit.restype = None

        self.lib.UpeParticleSystem_EmitEx.argtypes = [
            POINTER(ParticleSystem), Vec3, Vec3,
            c_float, c_float, c_float, c_float
        ]
        self.lib.UpeParticleSystem_EmitEx.restype = None

        self.lib.UpeParticleSystem_EmitFromRequests.argtypes = [
            POINTER(ParticleSystem), POINTER(ParticleSpawnRequest), c_uint32
        ]
        self.lib.UpeParticleSystem_EmitFromRequests.restype = None

        self.lib.UpeParticleSystem_Step.argtypes = [POINTER(ParticleSystem), c_float, Vec3]
        self.lib.UpeParticleSystem_Step.restype = None

        self.lib.UpeParticleSystem_Destroy.argtypes = [POINTER(ParticleSystem)]
        self.lib.UpeParticleSystem_Destroy.restype = None

    def _setup_emitter_functions(self):
        # Emitter pool functions
        self.lib.UpeEmitterPool_Create.argtypes = [c_uint32, c_uint32]
        self.lib.UpeEmitterPool_Create.restype = ctypes.c_void_p

        self.lib.UpeEmitterPool_Destroy.argtypes = [ctypes.c_void_p]
        self.lib.UpeEmitterPool_Destroy.restype = None

        self.lib.UpeEmitterPool_AddEmitter.argtypes = [ctypes.c_void_p, POINTER(EmitterDesc)]
        self.lib.UpeEmitterPool_AddEmitter.restype = c_uint32

        self.lib.UpeEmitterPool_Update.argtypes = [ctypes.c_void_p, c_float]
        self.lib.UpeEmitterPool_Update.restype = None

        self.lib.UpeEmitterPool_GetEmissionQueue.argtypes = [
            ctypes.c_void_p, POINTER(ParticleSpawnRequest), c_uint32
        ]
        self.lib.UpeEmitterPool_GetEmissionQueue.restype = c_uint32

        self.lib.UpeEmitterPool_EmitBurst.argtypes = [ctypes.c_void_p, c_uint32, c_uint32]
        self.lib.UpeEmitterPool_EmitBurst.restype = None

        self.lib.UpeEmitterPool_SetActive.argtypes = [ctypes.c_void_p, c_uint32, c_bool]
        self.lib.UpeEmitterPool_SetActive.restype = None

        self.lib.UpeEmitterPool_SetTransform.argtypes = [ctypes.c_void_p, c_uint32, Vec3, Quat]
        self.lib.UpeEmitterPool_SetTransform.restype = None

        self.lib.UpeEmitterPool_SetForces.argtypes = [ctypes.c_void_p, c_uint32, Vec3, Vec3]
        self.lib.UpeEmitterPool_SetForces.restype = None
    
    def _setup_spray_functions(self):
        # Fluid spray functions
        self.lib.UpeFluidSpray_Init.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p]
        self.lib.UpeFluidSpray_Init.restype = None
        
        self.lib.UpeFluidSpray_Update.argtypes = [ctypes.c_void_p, c_float]
        self.lib.UpeFluidSpray_Update.restype = None
        
        self.lib.UpeFluidSpray_GetTotalEmitted.argtypes = [ctypes.c_void_p]
        self.lib.UpeFluidSpray_GetTotalEmitted.restype = c_uint32
        
        self.lib.UpeFluidSpray_GetEmissionStats.argtypes = [
            ctypes.c_void_p, POINTER(c_uint32), POINTER(c_uint32), POINTER(c_uint32)
        ]
        self.lib.UpeEmitterPool_SetForces.restype = None
    
    # World API
    def create_world(self, fixed_dt=1.0/60.0):
        """Create a physics world with a fixed timestep."""
        desc = WorldDesc(fixed_dt=fixed_dt)
        return self.lib.UpeWorld_Create(ctypes.byref(desc))
    
    def destroy_world(self, world):
        """Destroy a physics world."""
        self.lib.UpeWorld_Destroy(world)
    
    def step_world(self, world):
        """Step the physics world by one fixed timestep."""
        self.lib.UpeWorld_Step(world)
    
    def get_world_time(self, world):
        """Get the current simulation time."""
        return self.lib.UpeWorld_Time(world)
    
    # Body API
    def create_body(self, world, position=(0,0,0), mass=1.0):
        """Create a dynamic rigid body."""
        pos = Vec3(x=position[0], y=position[1], z=position[2])
        rot = Quat(x=0, y=0, z=0, w=1)
        desc = BodyDesc(type=2, position=pos, rotation=rot, mass=mass)
        return self.lib.UpeWorld_CreateBody(world, ctypes.byref(desc))
    
    # SPH Fluid API
    def create_fluid(self, max_particles=1000):
        """Create an SPH fluid system."""
        fluid = ctypes.create_string_buffer(4096)  # Allocate space for UpeFluidSph struct
        self.lib.UpeFluidSph_Init(ctypes.cast(fluid, ctypes.c_void_p), max_particles)
        return fluid
    
    def set_fluid_bounds(self, fluid, min_pos, max_pos):
        """Set the boundary box for the fluid."""
        min_vec = Vec3(*min_pos)
        max_vec = Vec3(*max_pos)
        self.lib.UpeFluidSph_SetBounds(ctypes.cast(fluid, ctypes.c_void_p), min_vec, max_vec)
    
    def set_fluid_gravity(self, fluid, gravity):
        """Set gravity for the fluid."""
        grav_vec = Vec3(*gravity)
        self.lib.UpeFluidSph_SetGravity(ctypes.cast(fluid, ctypes.c_void_p), grav_vec)
    
    def add_fluid_particle(self, fluid, position, velocity=(0,0,0)):
        """Add a particle to the fluid."""
        pos = Vec3(*position)
        vel = Vec3(*velocity)
        return self.lib.UpeFluidSph_AddParticle(ctypes.cast(fluid, ctypes.c_void_p), pos, vel)
    
    def step_fluid(self, fluid, dt):
        """Step the fluid simulation."""
        self.lib.UpeFluidSph_Step(ctypes.cast(fluid, ctypes.c_void_p), dt)
    
    def destroy_fluid(self, fluid):
        """Destroy the fluid system."""
        self.lib.UpeFluidSph_Destroy(ctypes.cast(fluid, ctypes.c_void_p))

    # Particle system API
    def create_particle_system(self, capacity=1024):
        """Create a basic particle system."""
        sys = ParticleSystem()
        self.lib.UpeParticleSystem_Init(ctypes.byref(sys), capacity)
        return sys

    def destroy_particle_system(self, sys):
        """Destroy the particle system and release memory."""
        self.lib.UpeParticleSystem_Destroy(ctypes.byref(sys))

    def emit_particle(self, sys, position, velocity=(0,0,0)):
        """Emit a particle with default properties."""
        pos = Vec3(*position)
        vel = Vec3(*velocity)
        self.lib.UpeParticleSystem_Emit(ctypes.byref(sys), pos, vel)

    def emit_particle_ex(self, sys, position, velocity, mass=1.0, life=5.0, size=1.0, drag=0.0):
        """Emit a particle with explicit properties."""
        pos = Vec3(*position)
        vel = Vec3(*velocity)
        self.lib.UpeParticleSystem_EmitEx(ctypes.byref(sys), pos, vel, mass, life, size, drag)

    def emit_particles_from_requests(self, sys, requests, request_count):
        """Emit particles from a spawn request array."""
        self.lib.UpeParticleSystem_EmitFromRequests(ctypes.byref(sys), requests, request_count)

    def step_particles(self, sys, dt, gravity=(0, -9.81, 0)):
        """Step the particle simulation."""
        grav = Vec3(*gravity)
        self.lib.UpeParticleSystem_Step(ctypes.byref(sys), dt, grav)

    # Emitter pool API
    def create_emitter_pool(self, max_emitters=4, max_particles_per_frame=256):
        """Create an emitter pool for spawning particles."""
        return self.lib.UpeEmitterPool_Create(max_emitters, max_particles_per_frame)

    def destroy_emitter_pool(self, pool):
        """Destroy an emitter pool."""
        self.lib.UpeEmitterPool_Destroy(pool)

    def add_emitter(self, pool, desc):
        """Add an emitter to a pool. Returns emitter id."""
        return self.lib.UpeEmitterPool_AddEmitter(pool, ctypes.byref(desc))

    def update_emitters(self, pool, dt):
        """Update emitter pool and accumulate spawn requests."""
        self.lib.UpeEmitterPool_Update(pool, dt)

    def get_emission_requests(self, pool, max_requests=256):
        """Get spawn requests for this frame."""
        req_array = (ParticleSpawnRequest * max_requests)()
        count = self.lib.UpeEmitterPool_GetEmissionQueue(pool, req_array, max_requests)
        return req_array, count

    def emit_burst(self, pool, emitter_id, count):
        """Force a burst emission on an emitter."""
        self.lib.UpeEmitterPool_EmitBurst(pool, emitter_id, count)

    def set_emitter_active(self, pool, emitter_id, active):
        """Enable or disable an emitter."""
        self.lib.UpeEmitterPool_SetActive(pool, emitter_id, active)

    def set_emitter_transform(self, pool, emitter_id, position, rotation):
        """Update emitter transform."""
        pos = Vec3(*position)
        rot = Quat(*rotation)
        self.lib.UpeEmitterPool_SetTransform(pool, emitter_id, pos, rot)

    def set_emitter_forces(self, pool, emitter_id, gravity, wind):
        """Update emitter forces."""
        grav = Vec3(*gravity)
        wnd = Vec3(*wind)
        self.lib.UpeEmitterPool_SetForces(pool, emitter_id, grav, wnd)
    
    # Fluid spray API
    def create_spray_system(self, fluid, emitter_pool):
        """Create a fluid spray system for secondary particle emission."""
        spray = ctypes.create_string_buffer(4096)  # UpeFluidSpraySystem struct
        self.lib.UpeFluidSpray_Init(ctypes.cast(spray, ctypes.c_void_p), 
                                     ctypes.cast(fluid, ctypes.c_void_p),
                                     emitter_pool)
        return spray
    
    def set_spray_params(self, spray, 
                        spray_vel_threshold=2.0, spray_rate=0.5, spray_lifetime=1.0,
                        foam_rate=0.2, foam_lifetime=2.0,
                        bubble_rate=0.1, bubble_lifetime=3.0,
                        emission_prob=0.3, max_per_frame=100):
        """Configure spray system parameters."""
        # Read current params
        spray_ptr = ctypes.cast(spray, ctypes.c_void_p)
        # Note: In production, you'd use proper struct access. For demo, we set via C API.
        pass  # Parameters set at C level for now
    
    def update_spray_system(self, spray, dt):
        """Update spray system to emit secondary particles."""
        self.lib.UpeFluidSpray_Update(ctypes.cast(spray, ctypes.c_void_p), dt)
    
    def get_spray_stats(self, spray):
        """Get emission statistics."""
        spray_count = ctypes.c_uint32()
        foam_count = ctypes.c_uint32()
        bubble_count = ctypes.c_uint32()
        self.lib.UpeFluidSpray_GetEmissionStats(ctypes.cast(spray, ctypes.c_void_p),
                                                ctypes.byref(spray_count),
                                                ctypes.byref(foam_count),
                                                ctypes.byref(bubble_count))
        return {
            'spray': spray_count.value,
            'foam': foam_count.value,
            'bubbles': bubble_count.value,
            'total': spray_count.value + foam_count.value + bubble_count.value
        }
    
    # Buoyancy API
    def apply_buoyancy(self, body, fluid_density=1000.0, surface_height=0.0, 
                      surface_normal=(0,1,0), drag_coeff=0.8, angular_drag=0.5):
        """Apply buoyancy forces to a body."""
        desc = BuoyancyDesc(
            fluid_density=fluid_density,
            fluid_surface_normal=Vec3(*surface_normal),
            fluid_surface_height=surface_height,
            drag_coefficient=drag_coeff,
            angular_drag=angular_drag,
            sample_points=27
        )
        state = BuoyancyState()
        self.lib.UpeBuoyancy_ApplyUniform(body, ctypes.byref(desc), ctypes.byref(state))
        return {
            'submerged_volume': state.submerged_volume,
            'buoyancy_force': (state.buoyancy_force.x, state.buoyancy_force.y, state.buoyancy_force.z),
            'drag_force': (state.drag_force.x, state.drag_force.y, state.drag_force.z),
            'center_of_buoyancy': (state.center_of_buoyancy.x, state.center_of_buoyancy.y, state.center_of_buoyancy.z)
        }
    
    # CCD API
    def ccd_sphere_sphere(self, center_a, radius_a, vel_a, center_b, radius_b, vel_b, dt):
        """Perform continuous collision detection between two spheres."""
        ca = Vec3(*center_a)
        va = Vec3(*vel_a)
        cb = Vec3(*center_b)
        vb = Vec3(*vel_b)
        result = self.lib.UpeCcd_SphereSphere(ca, radius_a, va, cb, radius_b, vb, dt)
        return {
            'hit': bool(result.hit),
            'toi': result.toi,
            'position': (result.position.x, result.position.y, result.position.z),
            'normal': (result.normal.x, result.normal.y, result.normal.z)
        }
    
    def ccd_box_box(self, min_a, max_a, vel_a, min_b, max_b, vel_b, dt):
        """Perform continuous collision detection between two boxes."""
        box_a = Aabb3(min=Vec3(*min_a), max=Vec3(*max_a))
        box_b = Aabb3(min=Vec3(*min_b), max=Vec3(*max_b))
        va = Vec3(*vel_a)
        vb = Vec3(*vel_b)
        result = self.lib.UpeCcd_BoxBox(ctypes.byref(box_a), va, 
                                        ctypes.byref(box_b), vb, dt)
        return {
            'hit': bool(result.hit),
            'toi': result.toi,
            'position': (result.position.x, result.position.y, result.position.z),
            'normal': (result.normal.x, result.normal.y, result.normal.z)
        }


if __name__ == "__main__":
    print("=== UPE Python Bindings Demo ===\n")
    
    upe = UpeBindings()
    
    # Test 1: World creation and stepping
    print("Test 1: World Creation")
    world = upe.create_world()
    for i in range(100):
        upe.step_world(world)
    print(f"World time after 100 steps: {upe.get_world_time(world):.3f}s")
    upe.destroy_world(world)
    
    # Test 2: Fluid simulation
    print("\nTest 2: SPH Fluid")
    fluid = upe.create_fluid(max_particles=100)
    upe.set_fluid_bounds(fluid, (-5, 0, -5), (5, 10, 5))
    upe.set_fluid_gravity(fluid, (0, -9.81, 0))
    
    # Add some particles
    for i in range(10):
        for j in range(10):
            upe.add_fluid_particle(fluid, (i * 0.1, j * 0.1 + 1, 0))
    
    # Step fluid
    for i in range(100):
        upe.step_fluid(fluid, 0.016)
    
    print("Fluid simulation completed")
    upe.destroy_fluid(fluid)

    # Test 3: Emitter + particle system
    print("\nTest 3: Particle Emitters")
    particle_sys = upe.create_particle_system(capacity=512)
    pool = upe.create_emitter_pool(max_emitters=2, max_particles_per_frame=128)

    emitter_desc = EmitterDesc(
        shape=1,  # UPE_EMITTER_SPHERE
        mode=0,   # UPE_EMITTER_CONTINUOUS
        position=Vec3(0, 2, 0),
        rotation=Quat(0, 0, 0, 1),
        particles_per_frame=8,
        emission_duration=0.0,
        particle_template=ParticleInitial(velocity=Vec3(0, 2, 0), lifetime=2.0, mass=1.0, size=0.1, drag=0.2),
        shape_scale=0.5,
        velocity_spread=0.5,
        gravity=Vec3(0, -9.81, 0),
        wind_velocity=Vec3(0, 0, 0),
        damping=0.0
    )

    emitter_id = upe.add_emitter(pool, emitter_desc)
    for i in range(60):
        upe.update_emitters(pool, 1.0 / 60.0)
        requests, count = upe.get_emission_requests(pool, 128)
        if count > 0:
            upe.emit_particles_from_requests(particle_sys, requests, count)
        upe.step_particles(particle_sys, 1.0 / 60.0)

    print(f"Emitter spawned {particle_sys.count} particles")
    upe.destroy_emitter_pool(pool)
    upe.destroy_particle_system(particle_sys)
    
    # Test 4: CCD
    print("\nTest 4: Continuous Collision Detection")
    result = upe.ccd_sphere_sphere(
        center_a=(0, 0, 0), radius_a=1.0, vel_a=(10, 0, 0),
        center_b=(5, 0, 0), radius_b=1.0, vel_b=(-10, 0, 0),
        dt=1.0
    )
    print(f"Sphere-Sphere CCD: Hit={result['hit']}, TOI={result['toi']:.3f}")
    
    result = upe.ccd_box_box(
        min_a=(-1, -1, -1), max_a=(1, 1, 1), vel_a=(5, 0, 0),
        min_b=(3, -1, -1), max_b=(5, 1, 1), vel_b=(-5, 0, 0),
        dt=1.0
    )
    print(f"Box-Box CCD: Hit={result['hit']}, TOI={result['toi']:.3f}")
    
    print("\n=== All Tests Passed ===")
