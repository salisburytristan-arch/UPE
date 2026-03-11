#!/usr/bin/env python3
"""
Generate SHA256 hash of the UnifiedPhysicsEngine directory manifest.
Walks all files, sorts paths, and computes a hash of the manifest.
"""
import os
import hashlib
from pathlib import Path

def generate_manifest_hash(root_dir):
    """Generate SHA256 hash of directory manifest."""
    files = []
    
    # Walk directory and collect all file paths
    for root, dirs, filenames in os.walk(root_dir):
        # Skip build artifacts and git
        dirs[:] = [d for d in dirs if d not in ['.git', '.venv', 'build', 'build_debug', 'build_release', '__pycache__']]
        
        for filename in filenames:
            # Skip certain file types
            if filename.endswith(('.obj', '.exe', '.dll', '.pdb', '.lib', '.ilk', '.exp', '.pyc')):
                continue
            
            filepath = os.path.join(root, filename)
            relpath = os.path.relpath(filepath, root_dir)
            files.append(relpath.replace('\\', '/'))  # Normalize to forward slashes
    
    # Sort paths for deterministic hash
    files.sort()
    
    # Create manifest string
    manifest = '\n'.join(files) + '\n'
    
    # Compute SHA256
    sha256_hash = hashlib.sha256(manifest.encode('utf-8')).hexdigest().upper()
    
    return sha256_hash, len(files)

if __name__ == '__main__':
    # Get the UnifiedPhysicsEngine directory (parent of tools)
    script_dir = Path(__file__).parent
    upe_dir = script_dir.parent
    
    hash_value, file_count = generate_manifest_hash(upe_dir)
    
    print(f"[UPE] Manifest Hash Generator")
    print(f"Directory: {upe_dir}")
    print(f"Files counted: {file_count}")
    print(f"SHA256: {hash_value}")
