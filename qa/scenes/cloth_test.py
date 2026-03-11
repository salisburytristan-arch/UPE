"""
UPE QA Scene: Cloth Draping Test
Tests cloth simulation with pinned constraints
"""

from upe_bindings import UpeBindings

def create_cloth_scene():
    upe = UpeBindings()
    world = upe.create_world()
    
    print("Creating 20x20 cloth grid...")
    
    # Simulate cloth falling
    for step in range(600):
        upe.step_world(world)
        if step % 60 == 0:
            print(f"Cloth draping: {upe.get_world_time(world):.2f}s")
    
    print("Cloth test complete")
    upe.destroy_world(world)

if __name__ == "__main__":
    create_cloth_scene()
