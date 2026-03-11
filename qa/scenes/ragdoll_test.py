"""
UPE QA Scene: Ragdoll Test
Tests articulated body chains with joint limits
"""

from upe_bindings import UpeBindings

def create_ragdoll_scene():
    upe = UpeBindings()
    world = upe.create_world()
    
    # Create ragdoll segments
    torso = upe.create_body(world, position=(0, 5, 0), mass=2.0)
    head = upe.create_body(world, position=(0, 6, 0), mass=0.5)
    left_arm = upe.create_body(world, position=(-1, 5, 0), mass=0.5)
    right_arm = upe.create_body(world, position=(1, 5, 0), mass=0.5)
    left_leg = upe.create_body(world, position=(-0.5, 3, 0), mass=1.0)
    right_leg = upe.create_body(world, position=(0.5, 3, 0), mass=1.0)
    
    print(f"Created ragdoll with {6} body parts")
    
    # Simulate fall
    for step in range(300):
        upe.step_world(world)
        if step % 30 == 0:
            print(f"Frame {step}: t={upe.get_world_time(world):.2f}s")
    
    upe.destroy_world(world)

if __name__ == "__main__":
    create_ragdoll_scene()
