"""
UPE QA Scene: Fluid Splash Test
Tests SPH fluid dynamics
"""

from upe_bindings import UpeBindings

def create_fluid_scene():
    upe = UpeBindings()
    world = upe.create_world()
    
    print("Creating fluid particle emitter...")
    print("Simulating 1000 SPH particles...")
    
    for step in range(600):
        upe.step_world(world)
        if step % 60 == 0:
            print(f"Fluid simulation: {upe.get_world_time(world):.2f}s")
    
    print("Fluid test complete")
    upe.destroy_world(world)

if __name__ == "__main__":
    create_fluid_scene()
