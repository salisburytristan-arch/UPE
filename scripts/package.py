#!/usr/bin/env python3
"""
Unified Physics Engine - Professional Distribution Packager

Creates clean, distribution-ready packages for:
- Indie tier (free/freemium)
- Studio tier (commercial)
- Enterprise tier (with support)

Usage:
    python scripts/package.py --tier indie --output dist/
    python scripts/package.py --tier studio --output dist/
    python scripts/package.py --tier enterprise --output dist/
"""

import os
import sys
import shutil
import argparse
import json
from pathlib import Path
from datetime import datetime

# Configuration
VERSION = "1.0.0"
BUILD_CONFIG = "Release"

PACKAGE_STRUCTURE = {
    "indie": {
        "include": True,
        "lib": True,
        "bin_examples": True,
        "examples_src": True,
        "docs": ["API_REFERENCE.md", "QUICKSTART.md", "LICENSE.md"],
        "include_tests": False,
        "include_bindings": False,
    },
    "studio": {
        "include": True,
        "lib": True,
        "bin_examples": True,
        "examples_src": True,
        "docs": ["API_REFERENCE.md", "QUICKSTART.md", "ADVANCED.md", "LICENSE.md", "CHANGELOG.md"],
        "include_tests": False,
        "include_bindings": True,
    },
    "enterprise": {
        "include": True,
        "lib": True,
        "bin_examples": True,
        "examples_src": True,
        "docs": ["API_REFERENCE.md", "QUICKSTART.md", "ADVANCED.md", "INTERNALS.md", "LICENSE.md", "EULA.md", "CHANGELOG.md"],
        "include_tests": True,
        "include_bindings": True,
    },
}


def clean_directory(path):
    """Remove directory if it exists."""
    if os.path.exists(path):
        shutil.rmtree(path)


def ensure_directory(path):
    """Create directory if it doesn't exist."""
    os.makedirs(path, exist_ok=True)


def copy_recursive(src, dst, exclude=None):
    """Copy directory recursively, excluding certain patterns."""
    if exclude is None:
        exclude = []
    
    if not os.path.exists(src):
        return 0
    
    count = 0
    for root, dirs, files in os.walk(src):
        # Filter directories
        dirs[:] = [d for d in dirs if not any(exc in d for exc in exclude)]
        
        for file in files:
            # Skip excluded files
            if any(file.endswith(exc) for exc in exclude):
                continue
            
            src_file = os.path.join(root, file)
            rel_path = os.path.relpath(src_file, src)
            dst_file = os.path.join(dst, rel_path)
            
            os.makedirs(os.path.dirname(dst_file), exist_ok=True)
            shutil.copy2(src_file, dst_file)
            count += 1
    
    return count


def build_project():
    """Build the project if not already built."""
    print("Building project...")
    build_dir = "cmake-build"
    
    if not os.path.exists(os.path.join(build_dir, "bin")):
        print(f"  Build directory {build_dir} not found. Running CMake...")
        os.system(f'cmake -S . -B {build_dir} -DCMAKE_BUILD_TYPE={BUILD_CONFIG} -DUPE_BUILD_TESTS=OFF -DUPE_BUILD_EXAMPLES=ON')
        os.system(f'cmake --build {build_dir} --config {BUILD_CONFIG}')


def get_public_headers():
    """Get list of public headers to include."""
    headers = []
    core_include = "core/include"
    
    # Recursively find all headers
    for root, dirs, files in os.walk(core_include):
        for file in files:
            if file.endswith(".h"):
                headers.append(os.path.join(root, file))
    
    return headers


def package_tier(tier, output_dir):
    """Create a distribution package for the specified tier."""
    
    if tier not in PACKAGE_STRUCTURE:
        print(f"ERROR: Unknown tier '{tier}'. Choose from: {', '.join(PACKAGE_STRUCTURE.keys())}")
        return False
    
    config = PACKAGE_STRUCTURE[tier]
    dist_dir = os.path.join(output_dir, f"upe-{tier}-{VERSION}")
    
    print(f"\n{'='*70}")
    print(f"Packaging UPE {VERSION} ({tier.upper()} Tier)")
    print(f"Output: {dist_dir}")
    print(f"{'='*70}\n")
    
    # Clean and create distribution directory
    clean_directory(dist_dir)
    ensure_directory(dist_dir)
    
    build_dir = "cmake-build"
    
    # 1. Copy headers
    if config["include"]:
        print("[1/8] Copying public headers...")
        headers_dst = os.path.join(dist_dir, "include", "upe")
        ensure_directory(headers_dst)
        
        core_include = os.path.join(build_dir, "..", "core", "include", "upe")
        copied = copy_recursive(core_include, headers_dst)
        print(f"  ✓ Copied {copied} header files")
    
    # 2. Copy libraries
    if config["lib"]:
        print("[2/8] Copying libraries...")
        lib_src = os.path.join(build_dir, "lib")
        lib_dst = os.path.join(dist_dir, "lib")
        ensure_directory(lib_dst)
        
        if os.path.exists(lib_src):
            for file in os.listdir(lib_src):
                if file.endswith((".a", ".lib", ".so", ".dylib")):
                    shutil.copy2(os.path.join(lib_src, file), os.path.join(lib_dst, file))
        
        # Verify library exists
        if os.path.exists(lib_dst) and os.listdir(lib_dst):
            print(f"  ✓ Copied library files to {lib_dst}")
        else:
            print(f"  ⚠ No library files found in {lib_src}")
    
    # 3. Copy example binaries
    if config["bin_examples"]:
        print("[3/8] Copying example executables...")
        bin_src = os.path.join(build_dir, "bin", "examples")
        bin_dst = os.path.join(dist_dir, "bin")
        ensure_directory(bin_dst)
        
        if os.path.exists(bin_src):
            for file in os.listdir(bin_src):
                if file.endswith((".exe", "")):  # Windows and Unix executables
                    shutil.copy2(os.path.join(bin_src, file), os.path.join(bin_dst, file))
        
        example_count = len(os.listdir(bin_dst)) if os.path.exists(bin_dst) else 0
        print(f"  ✓ Copied {example_count} example executables")
    
    # 4. Copy example source code
    if config["examples_src"]:
        print("[4/8] Copying example source code...")
        examples_dst = os.path.join(dist_dir, "examples")
        ensure_directory(examples_dst)
        
        copied = copy_recursive("examples", examples_dst, exclude=[".exe", ".obj", ".o"])
        print(f"  ✓ Copied {copied} example source files")
    
    # 5. Copy tests (enterprise only)
    if config["include_tests"]:
        print("[5/8] Copying test suite...")
        tests_dst = os.path.join(dist_dir, "tests")
        ensure_directory(tests_dst)
        
        copied = copy_recursive("tests", tests_dst, exclude=[".exe", ".obj", ".o"])
        print(f"  ✓ Copied {copied} test files")
    else:
        print("[5/8] Skipping tests (not in this tier)")
    
    # 6. Copy Python bindings (studio/enterprise)
    if config["include_bindings"]:
        print("[6/8] Copying Python bindings...")
        bindings_dst = os.path.join(dist_dir, "bindings")
        ensure_directory(bindings_dst)
        
        if os.path.exists("bindings/python"):
            copied = copy_recursive("bindings/python", os.path.join(bindings_dst, "python"))
            print(f"  ✓ Copied {copied} binding files")
        else:
            print(f"  ⚠ Python bindings not found")
    else:
        print("[6/8] Skipping bindings (not in this tier)")
    
    # 7. Copy documentation
    print("[7/8] Creating documentation...")
    docs_dst = os.path.join(dist_dir, "docs")
    ensure_directory(docs_dst)
    
    doc_count = 0
    for doc_file in config["docs"]:
        src_file = os.path.join("docs", doc_file)
        if not os.path.exists(src_file):
            # Check root directory
            src_file = doc_file
        
        if os.path.exists(src_file):
            shutil.copy2(src_file, os.path.join(docs_dst, os.path.basename(src_file)))
            doc_count += 1
        else:
            print(f"  ⚠ Document not found: {doc_file}")
    
    print(f"  ✓ Included {doc_count} documentation files")
    
    # 8. Create manifest and metadata
    print("[8/8] Creating package metadata...")
    metadata = {
        "name": "Unified Physics Engine",
        "version": VERSION,
        "tier": tier,
        "created": datetime.now().isoformat(),
        "files": {
            "include": config["include"],
            "lib": config["lib"],
            "examples_bin": config["bin_examples"],
            "examples_src": config["examples_src"],
            "tests": config["include_tests"],
            "bindings": config["include_bindings"],
        },
    }
    
    metadata_file = os.path.join(dist_dir, "PACKAGE.json")
    with open(metadata_file, "w") as f:
        json.dump(metadata, f, indent=2)
    
    # Copy LICENSE and README
    for file in ["LICENSE", "LICENSE.md", "README.md"]:
        if os.path.exists(file):
            shutil.copy2(file, os.path.join(dist_dir, file))
    
    print(f"  ✓ Created package metadata")
    
    # Summary
    print(f"\n{'='*70}")
    print(f"✅ Package Complete!")
    print(f"{'='*70}")
    print(f"Location: {dist_dir}")
    print(f"Tier:     {tier.upper()}")
    print(f"Version:  {VERSION}")
    
    # Print directory structure
    print(f"\nDirectory Structure:")
    for root, dirs, files in os.walk(dist_dir):
        level = root.replace(dist_dir, "").count(os.sep)
        indent = " " * 2 * level
        print(f"{indent}{os.path.basename(root)}/")
        subindent = " " * 2 * (level + 1)
        for file in files[:5]:  # Show first 5 files
            print(f"{subindent}{file}")
        if len(files) > 5:
            print(f"{subindent}... and {len(files) - 5} more files")
    
    return True


def main():
    parser = argparse.ArgumentParser(
        description="Package Unified Physics Engine for distribution"
    )
    parser.add_argument(
        "--tier",
        choices=["indie", "studio", "enterprise", "all"],
        default="indie",
        help="Package tier (indie/studio/enterprise/all)",
    )
    parser.add_argument(
        "--output",
        default="dist",
        help="Output directory for packages",
    )
    parser.add_argument(
        "--no-build",
        action="store_true",
        help="Skip building the project",
    )
    
    args = parser.parse_args()
    
    # Build project if needed
    if not args.no_build:
        build_project()
    
    # Create output directory
    ensure_directory(args.output)
    
    # Package specified tier(s)
    success = True
    if args.tier == "all":
        for tier in ["indie", "studio", "enterprise"]:
            if not package_tier(tier, args.output):
                success = False
    else:
        if not package_tier(args.tier, args.output):
            success = False
    
    if success:
        print(f"\n✅ All packages created successfully in '{args.output}'")
        return 0
    else:
        print(f"\n❌ Package creation failed")
        return 1


if __name__ == "__main__":
    sys.exit(main())
