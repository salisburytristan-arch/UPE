"""
UPE QA Scene: Vehicle Test
Tests vehicle suspension and wheel dynamics
"""

from upe_bindings import UpeBindings

def create_vehicle_scene():
    upe = UpeBindings()
    world = upe.create_world()
    
    # Create vehicle chassis
    chassis = upe.create_body(world, position=(0, 2, 0), mass=500.0)
    
    # Create wheels (simplified as spheres)
    fl_wheel = upe.create_body(world, position=(-1, 1, 1), mass=20.0)
    fr_wheel = upe.create_body(world, position=(1, 1, 1), mass=20.0)
    rl_wheel = upe.create_body(world, position=(-1, 1, -1), mass=20.0)
    rr_wheel = upe.create_body(world, position=(1, 1, -1), mass=20.0)
    
    print("Created vehicle with 4 wheels")
    
    # Simulate vehicle drop and settle
    for step in range(600):
        upe.step_world(world)
        if step % 60 == 0:
            print(f"Vehicle simulation: {upe.get_world_time(world):.2f}s")
    
    upe.destroy_world(world)

if __name__ == "__main__":
    create_vehicle_scene()
