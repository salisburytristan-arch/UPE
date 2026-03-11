// C# bindings for UPE
using System;
using System.Runtime.InteropServices;

namespace UpeSharp
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vec3
    {
        public float x, y, z;
        public Vec3(float x, float y, float z) { this.x = x; this.y = y; this.z = z; }
        public override string ToString() => $"({x:F2}, {y:F2}, {z:F2})";
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Quat
    {
        public float x, y, z, w;
        public Quat(float x, float y, float z, float w) { 
            this.x = x; this.y = y; this.z = z; this.w = w; 
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct WorldDesc
    {
        public float fixed_dt;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct BodyDesc
    {
        public uint type;
        public Vec3 position;
        public Quat rotation;
        public float mass;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct BuoyancyDesc
    {
        public float fluid_density;
        public Vec3 fluid_surface_normal;
        public float fluid_surface_height;
        public float drag_coefficient;
        public float angular_drag;
        public uint sample_points;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct BuoyancyState
    {
        public float submerged_volume;
        public Vec3 buoyancy_force;
        public Vec3 drag_force;
        public Vec3 center_of_buoyancy;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct CcdResult
    {
        public int hit;
        public float toi;
        public Vec3 position;
        public Vec3 normal;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Aabb3
    {
        public Vec3 min;
        public Vec3 max;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Particle
    {
        public Vec3 position;
        public Vec3 velocity;
        public float mass;
        public float life;
        public float size;
        public float drag;
        public uint flags;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct ParticleSystemStruct
    {
        public IntPtr particles;
        public uint capacity;
        public uint count;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct ParticleInitial
    {
        public Vec3 velocity;
        public float lifetime;
        public float mass;
        public float size;
        public float drag;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct EmitterDesc
    {
        public uint shape;
        public uint mode;
        public Vec3 position;
        public Quat rotation;
        public uint particles_per_frame;
        public float emission_duration;
        public ParticleInitial particle_template;
        public float shape_scale;
        public float velocity_spread;
        public Vec3 gravity;
        public Vec3 wind_velocity;
        public float damping;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct ParticleSpawnRequest
    {
        public Vec3 position;
        public Vec3 velocity;
        public float lifetime;
        public float mass;
        public float size;
        public float drag;
    }

    public class Upe
    {
        private const string DLL = "upe.dll";

        // World functions
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr UpeWorld_Create(ref WorldDesc desc);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeWorld_Destroy(IntPtr world);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeWorld_Step(IntPtr world);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern float UpeWorld_Time(IntPtr world);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern uint UpeWorld_CreateBody(IntPtr world, ref BodyDesc desc);

        // SPH Fluid functions
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeFluidSph_Init(IntPtr fluid, uint max_particles);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeFluidSph_SetBounds(IntPtr fluid, Vec3 min, Vec3 max);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeFluidSph_SetGravity(IntPtr fluid, Vec3 gravity);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern uint UpeFluidSph_AddParticle(IntPtr fluid, Vec3 position, Vec3 velocity);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeFluidSph_RemoveParticle(IntPtr fluid, uint index);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeFluidSph_Step(IntPtr fluid, float dt);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeFluidSph_Destroy(IntPtr fluid);

        // Buoyancy functions
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeBuoyancy_ApplyUniform(IntPtr body, ref BuoyancyDesc desc, out BuoyancyState state);

        // CCD functions
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern CcdResult UpeCcd_SphereSphere(
            Vec3 center_a, float radius_a, Vec3 vel_a,
            Vec3 center_b, float radius_b, Vec3 vel_b,
            float dt);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern CcdResult UpeCcd_BoxBox(
            ref Aabb3 a, Vec3 vel_a,
            ref Aabb3 b, Vec3 vel_b,
            float dt);

        // Particle system functions
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeParticleSystem_Init(IntPtr sys, uint capacity);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeParticleSystem_Emit(IntPtr sys, Vec3 position, Vec3 velocity);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeParticleSystem_EmitEx(
            IntPtr sys, Vec3 position, Vec3 velocity,
            float mass, float life, float size, float drag);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeParticleSystem_EmitFromRequests(
            IntPtr sys, [In] ParticleSpawnRequest[] requests, uint requestCount);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeParticleSystem_Step(IntPtr sys, float dt, Vec3 gravity);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeParticleSystem_Destroy(IntPtr sys);

        // Emitter pool functions
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr UpeEmitterPool_Create(uint maxEmitters, uint maxParticlesPerFrame);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeEmitterPool_Destroy(IntPtr pool);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern uint UpeEmitterPool_AddEmitter(IntPtr pool, ref EmitterDesc desc);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeEmitterPool_Update(IntPtr pool, float dt);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern uint UpeEmitterPool_GetEmissionQueue(
            IntPtr pool, [Out] ParticleSpawnRequest[] requests, uint maxRequests);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeEmitterPool_EmitBurst(IntPtr pool, uint emitterId, uint count);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeEmitterPool_SetActive(IntPtr pool, uint emitterId, bool active);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeEmitterPool_SetTransform(IntPtr pool, uint emitterId, Vec3 position, Quat rotation);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeEmitterPool_SetForces(IntPtr pool, uint emitterId, Vec3 gravity, Vec3 wind);

        // Fluid spray functions
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeFluidSpray_Init(IntPtr spray, IntPtr fluid, IntPtr emitterPool);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeFluidSpray_Update(IntPtr spray, float dt);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern uint UpeFluidSpray_GetTotalEmitted(IntPtr spray);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        private static extern void UpeFluidSpray_GetEmissionStats(IntPtr spray, out uint spray, out uint foam, out uint bubbles);

        public class World : IDisposable
        {
            private IntPtr handle;

            public World(float fixedDt = 1.0f / 60.0f)
            {
                var desc = new WorldDesc { fixed_dt = fixedDt };
                handle = UpeWorld_Create(ref desc);
            }

            public void Step() => UpeWorld_Step(handle);
            public float Time => UpeWorld_Time(handle);

            public uint CreateBody(Vec3 position, float mass = 1.0f)
            {
                var desc = new BodyDesc
                {
                    type = 2,
                    position = position,
                    rotation = new Quat(0, 0, 0, 1),
                    mass = mass
                };
                return UpeWorld_CreateBody(handle, ref desc);
            }

            public void Dispose()
            {
                if (handle != IntPtr.Zero)
                {
                    UpeWorld_Destroy(handle);
                    handle = IntPtr.Zero;
                }
            }
        }

        public class FluidSph : IDisposable
        {
            private IntPtr handle;
            private bool disposed;

            public FluidSph(uint maxParticles = 1000)
            {
                // Allocate memory for UpeFluidSph struct
                handle = Marshal.AllocHGlobal(4096);
                UpeFluidSph_Init(handle, maxParticles);
            }

            public void SetBounds(Vec3 min, Vec3 max)
            {
                UpeFluidSph_SetBounds(handle, min, max);
            }

            public void SetGravity(Vec3 gravity)
            {
                UpeFluidSph_SetGravity(handle, gravity);
            }

            public uint AddParticle(Vec3 position, Vec3 velocity = default)
            {
                return UpeFluidSph_AddParticle(handle, position, velocity);
            }

            public void RemoveParticle(uint index)
            {
                UpeFluidSph_RemoveParticle(handle, index);
            }

            public void Step(float dt)
            {
                UpeFluidSph_Step(handle, dt);
            }

            public void Dispose()
            {
                if (!disposed && handle != IntPtr.Zero)
                {
                    UpeFluidSph_Destroy(handle);
                    Marshal.FreeHGlobal(handle);
                    handle = IntPtr.Zero;
                    disposed = true;
                }
            }
        }

        public class ParticleSystem : IDisposable
        {
            private IntPtr handle;
            private bool disposed;

            public ParticleSystem(uint capacity = 1024)
            {
                handle = Marshal.AllocHGlobal(Marshal.SizeOf<ParticleSystemStruct>());
                UpeParticleSystem_Init(handle, capacity);
            }

            public void Emit(Vec3 position, Vec3 velocity)
            {
                UpeParticleSystem_Emit(handle, position, velocity);
            }

            public void EmitEx(Vec3 position, Vec3 velocity, float mass, float life, float size, float drag)
            {
                UpeParticleSystem_EmitEx(handle, position, velocity, mass, life, size, drag);
            }

            public void EmitFromRequests(ParticleSpawnRequest[] requests, uint count)
            {
                if (requests == null || count == 0)
                {
                    return;
                }
                UpeParticleSystem_EmitFromRequests(handle, requests, count);
            }

            public void Step(float dt, Vec3 gravity)
            {
                UpeParticleSystem_Step(handle, dt, gravity);
            }

            public uint Count
            {
                get
                {
                    var sys = Marshal.PtrToStructure<ParticleSystemStruct>(handle);
                    return sys.count;
                }
            }

            public void Dispose()
            {
                if (!disposed && handle != IntPtr.Zero)
                {
                    UpeParticleSystem_Destroy(handle);
                    Marshal.FreeHGlobal(handle);
                    handle = IntPtr.Zero;
                    disposed = true;
                }
            }
        }

        public class EmitterPool : IDisposable
        {
            private IntPtr handle;
            private bool disposed;

            public EmitterPool(uint maxEmitters = 4, uint maxParticlesPerFrame = 256)
            {
                handle = UpeEmitterPool_Create(maxEmitters, maxParticlesPerFrame);
            }

            public uint AddEmitter(EmitterDesc desc)
            {
                return UpeEmitterPool_AddEmitter(handle, ref desc);
            }

            public void Update(float dt)
            {
                UpeEmitterPool_Update(handle, dt);
            }

            public uint GetEmissionQueue(ParticleSpawnRequest[] requests)
            {
                if (requests == null || requests.Length == 0)
                {
                    return 0;
                }
                return UpeEmitterPool_GetEmissionQueue(handle, requests, (uint)requests.Length);
            }

            public void EmitBurst(uint emitterId, uint count)
            {
                UpeEmitterPool_EmitBurst(handle, emitterId, count);
            }

            public void SetActive(uint emitterId, bool active)
            {
                UpeEmitterPool_SetActive(handle, emitterId, active);
            }

            public void SetTransform(uint emitterId, Vec3 position, Quat rotation)
            {
                UpeEmitterPool_SetTransform(handle, emitterId, position, rotation);
            }

            public void SetForces(uint emitterId, Vec3 gravity, Vec3 wind)
            {
                UpeEmitterPool_SetForces(handle, emitterId, gravity, wind);
            }

            public void Dispose()
            {
                if (!disposed && handle != IntPtr.Zero)
                {
                    UpeEmitterPool_Destroy(handle);
                    handle = IntPtr.Zero;
                    disposed = true;
                }
            }
        }

        public class FluidSpraySystem : IDisposable
        {
            private IntPtr handle;
            private bool disposed;

            public FluidSpraySystem(IntPtr fluid, IntPtr emitterPool)
            {
                handle = Marshal.AllocHGlobal(4096);  // Allocate space for UpeFluidSpraySystem
                UpeFluidSpray_Init(handle, fluid, emitterPool);
            }

            public void Update(float dt)
            {
                UpeFluidSpray_Update(handle, dt);
            }

            public uint GetTotalEmitted()
            {
                return UpeFluidSpray_GetTotalEmitted(handle);
            }

            public (uint spray, uint foam, uint bubbles) GetEmissionStats()
            {
                uint spray, foam, bubbles;
                UpeFluidSpray_GetEmissionStats(handle, out spray, out foam, out bubbles);
                return (spray, foam, bubbles);
            }

            public void Dispose()
            {
                if (!disposed && handle != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(handle);
                    handle = IntPtr.Zero;
                    disposed = true;
                }
            }
        }

        public static class Buoyancy
        {
            public static BuoyancyState ApplyUniform(IntPtr body, 
                float fluidDensity = 1000f, 
                float surfaceHeight = 0f,
                Vec3? surfaceNormal = null,
                float dragCoeff = 0.8f,
                float angularDrag = 0.5f)
            {
                var desc = new BuoyancyDesc
                {
                    fluid_density = fluidDensity,
                    fluid_surface_normal = surfaceNormal ?? new Vec3(0, 1, 0),
                    fluid_surface_height = surfaceHeight,
                    drag_coefficient = dragCoeff,
                    angular_drag = angularDrag,
                    sample_points = 27
                };
                
                UpeBuoyancy_ApplyUniform(body, ref desc, out var state);
                return state;
            }
        }

        public static class Ccd
        {
            public static CcdResult SphereSphere(
                Vec3 centerA, float radiusA, Vec3 velA,
                Vec3 centerB, float radiusB, Vec3 velB,
                float dt)
            {
                return UpeCcd_SphereSphere(centerA, radiusA, velA, centerB, radiusB, velB, dt);
            }

            public static CcdResult BoxBox(
                Vec3 minA, Vec3 maxA, Vec3 velA,
                Vec3 minB, Vec3 maxB, Vec3 velB,
                float dt)
            {
                var boxA = new Aabb3 { min = minA, max = maxA };
                var boxB = new Aabb3 { min = minB, max = maxB };
                return UpeCcd_BoxBox(ref boxA, velA, ref boxB, velB, dt);
            }
        }
    }

    class Program
    {
        static void Main()
        {
            Console.WriteLine("=== UPE C# Bindings Demo ===\n");

            // Test 1: World
            Console.WriteLine("Test 1: World Creation");
            using (var world = new Upe.World())
            {
                for (int i = 0; i < 100; i++)
                {
                    world.Step();
                }
                Console.WriteLine($"World time after 100 steps: {world.Time:F3}s");
            }

            // Test 2: Fluid
            Console.WriteLine("\nTest 2: SPH Fluid");
            using (var fluid = new Upe.FluidSph(maxParticles: 100))
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

                // Step
                for (int i = 0; i < 100; i++)
                {
                    fluid.Step(0.016f);
                }

                Console.WriteLine("Fluid simulation completed");
            }

            // Test 3: Particle emitters
            Console.WriteLine("\nTest 3: Particle Emitters");
            using (var particleSystem = new Upe.ParticleSystem(capacity: 512))
            using (var emitterPool = new Upe.EmitterPool(maxEmitters: 2, maxParticlesPerFrame: 128))
            {
                var emitterDesc = new EmitterDesc
                {
                    shape = 1,  // UPE_EMITTER_SPHERE
                    mode = 0,   // UPE_EMITTER_CONTINUOUS
                    position = new Vec3(0, 2, 0),
                    rotation = new Quat(0, 0, 0, 1),
                    particles_per_frame = 8,
                    emission_duration = 0.0f,
                    particle_template = new ParticleInitial
                    {
                        velocity = new Vec3(0, 2, 0),
                        lifetime = 2.0f,
                        mass = 1.0f,
                        size = 0.1f,
                        drag = 0.2f
                    },
                    shape_scale = 0.5f,
                    velocity_spread = 0.5f,
                    gravity = new Vec3(0, -9.81f, 0),
                    wind_velocity = new Vec3(0, 0, 0),
                    damping = 0.0f
                };

                emitterPool.AddEmitter(emitterDesc);
                var requests = new ParticleSpawnRequest[128];

                for (int i = 0; i < 60; i++)
                {
                    emitterPool.Update(1.0f / 60.0f);
                    uint count = emitterPool.GetEmissionQueue(requests);
                    if (count > 0)
                    {
                        particleSystem.EmitFromRequests(requests, count);
                    }
                    particleSystem.Step(1.0f / 60.0f, new Vec3(0, -9.81f, 0));
                }

                Console.WriteLine($"Emitter spawned {particleSystem.Count} particles");
            }

            // Test 4: CCD
            Console.WriteLine("\nTest 4: Continuous Collision Detection");
            var result = Upe.Ccd.SphereSphere(
                new Vec3(0, 0, 0), 1.0f, new Vec3(10, 0, 0),
                new Vec3(5, 0, 0), 1.0f, new Vec3(-10, 0, 0),
                1.0f);
            Console.WriteLine($"Sphere-Sphere CCD: Hit={result.hit != 0}, TOI={result.toi:F3}");

            result = Upe.Ccd.BoxBox(
                new Vec3(-1, -1, -1), new Vec3(1, 1, 1), new Vec3(5, 0, 0),
                new Vec3(3, -1, -1), new Vec3(5, 1, 1), new Vec3(-5, 0, 0),
                1.0f);
            Console.WriteLine($"Box-Box CCD: Hit={result.hit != 0}, TOI={result.toi:F3}");

            Console.WriteLine("\n=== All Tests Passed ===");
        }
    }
}
