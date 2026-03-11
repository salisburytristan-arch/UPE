#!/usr/bin/env python3
"""
Unified Physics Engine - Clean Release Package Creator

Removes build artifacts, .venv, .obj files, and creates distribution-ready zip files.

Usage:
    python scripts/create_release.py
    python scripts/create_release.py --tier buyout
"""

import os
import sys
import shutil
import zipfile
import argparse
import re
from pathlib import Path
from datetime import datetime

VERSION = "1.0.0"
RELEASE_DATE = datetime.now().strftime("%Y-%m-%d")

# Files/directories to exclude from ALL release packages
ALWAYS_EXCLUDE = {
    ".venv",
    "build",
    "build-release",
    "build-debug",
    "build_warn",
    "cmake-build",
    "cmake-build-ninja",
    "bin",
    "lib",
    "CMakeFiles",
    "CMakeCache.txt",
    "compile_commands.json",
    "Testing",
    ".obj",
    ".o",
    ".pdb",
    ".ilk",
    ".exp",
    ".idb",
    ".exe",
    ".git",
    ".github",
    "__pycache__",
    "*.pyc",
    ".DS_Store",
}

BINARY_LIBRARY_EXTENSIONS = {".lib", ".a", ".dll", ".so", ".dylib"}

# Package configuration (single full buyout package)
TIER_CONFIG = {
    "buyout": {
        "name": "Unified Physics Engine (Exclusive IP Buyout Package)",
        "description": "Full proprietary source package for private acquisition diligence",
        "include_dirs": ["core", "examples", "tests", "benchmarks", "bindings", "scripts", "docs", "cmake", "tools", "proof"],
        "include_files": [
            "README.md",
            "LICENSE",
            "CONTRIBUTING.md",
            "DISTRIBUTION_POLICY.md",
            "IP_CHAIN_OF_TITLE_CHECKLIST.md",
            "COMMERCIAL_LICENSE_AGREEMENT.md",
            "QUICK_START.md",
            "API_REFERENCE.md",
            "ADVANCED_INTEGRATION.md",
            "PERFORMANCE_OPTIMIZATION_GUIDE.md",
            "ARCHITECTURE.md",
            "build_msvc.bat",
            "CMakeLists.txt",
            "BUILD_STATUS.md",
            "topic_truth_audit.csv",
        ],
        "include_lib": True,
        "include_tests": True,
        "include_bindings": True,
    },
}


def should_exclude(path_str, tier_config):
    """Check if a path should be excluded."""
    path = Path(path_str)
    path_lower = str(path).lower()
    path_parts = {part.lower() for part in path.parts}

    if path.suffix.lower() in BINARY_LIBRARY_EXTENSIONS and tier_config["include_lib"]:
        return False

    for exclude_pattern in ALWAYS_EXCLUDE:
        if exclude_pattern.startswith("."):
            if exclude_pattern in path_lower or path_lower.endswith(exclude_pattern[1:]):
                return True
        elif exclude_pattern.startswith("*"):
            ext = exclude_pattern[1:]
            if path_lower.endswith(ext):
                return True
        else:
            if exclude_pattern.lower() in path_parts:
                return True
    return False


def copy_directory_tree(src, dst, tier_config):
    """Copy directory tree, excluding unwanted files."""
    src_path = Path(src)
    dst_path = Path(dst)
    dst_path.mkdir(parents=True, exist_ok=True)

    file_count = 0
    for src_file in src_path.rglob("*"):
        if src_file.is_file():
            if should_exclude(src_file, tier_config):
                continue

            rel_path = src_file.relative_to(src_path)
            dst_file = dst_path / rel_path

            dst_file.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src_file, dst_file)
            file_count += 1

    return file_count


def create_metadata(tier):
    """Create metadata.json for the package."""
    config = TIER_CONFIG[tier]
    return {
        "name": config["name"],
        "version": VERSION,
        "date": RELEASE_DATE,
        "tier": tier,
        "description": config["description"],
    }


def count_library_binaries(path):
    """Count library artifacts in a packaged directory."""
    return sum(
        1
        for file_path in Path(path).rglob("*")
        if file_path.is_file() and file_path.suffix.lower() in BINARY_LIBRARY_EXTENSIONS
    )


def count_example_sources(root_dir):
    """Count source examples for high-level package summary."""
    examples_dir = root_dir / "examples"
    if not examples_dir.exists():
        return 0

    source_exts = {".c", ".cc", ".cpp", ".cxx", ".h", ".hpp"}
    return sum(
        1
        for file_path in examples_dir.rglob("*")
        if file_path.is_file() and file_path.suffix.lower() in source_exts
    )


def validate_package_baseline(temp_dir, config):
    """Validate that release package has baseline buyer-facing files."""
    required_files = [
        "README.md",
        "LICENSE",
        "CONTRIBUTING.md",
        "DISTRIBUTION_POLICY.md",
        "docs/BUYER_START_HERE.md",
        "docs/INDEX.md",
        "docs/DETERMINISM_GUIDE.md",
        "docs/INTEGRATION_GUIDE.md",
        "docs/API_REFERENCE.md",
        "docs/BINDINGS_STATUS.md",
        "docs/TROUBLESHOOTING.md",
        "docs/KNOWN_LIMITATIONS.md",
        "docs/SECURITY_AND_SAFETY.md",
        "docs/ACCEPTANCE_CHECKLIST.md",
        "docs/ARCHITECTURE.md",
        "docs/IMPLEMENTATION_STATUS.md",
        "docs/RELEASE_NOTES_v1.0.0.md",
        "docs/GETTING_STARTED.md",
        "docs/FLUID_SIMULATION_GUIDE.md",
        "tools/build_windows_msvc.bat",
        "tools/build_linux.sh",
        "tools/run_determinism_proof.bat",
        "tools/run_determinism_proof.sh",
        "tools/run_examples_smoketest.bat",
        "tools/run_examples_smoketest.sh",
        "tools/diagnose_environment.bat",
        "tools/diagnose_environment.sh",
        "tools/hash_package.bat",
        "tools/hash_package.sh",
        "tools/hash_package.py",
        "proof/CLOSING_HASH_RECORD.txt",
        "proof/EXPECTED_OUTPUTS/build_success_windows.txt",
        "proof/EXPECTED_OUTPUTS/build_success_linux.txt",
        "proof/EXPECTED_OUTPUTS/tests_pass.txt",
        "proof/EXPECTED_OUTPUTS/determinism_hash_expected.txt",
        "proof/EXPECTED_OUTPUTS/examples_smoketest_expected.txt",
        "proof/REFERENCE_RUNS/windows_reference_run_YYYYMMDD.log",
        "proof/REFERENCE_RUNS/linux_reference_run_YYYYMMDD.log",
        "proof/REFERENCE_RUNS/hashes_reference_run_YYYYMMDD.json",
        "QUICK_START.md",
        "API_REFERENCE.md",
        "CMakeLists.txt",
        "cmake/UpeConfig.cmake.in",
        "build_msvc.bat",
        "topic_truth_audit.csv",
    ]

    def required_exists(required_path: str) -> bool:
        if (temp_dir / required_path).exists():
            return True
        if "YYYYMMDD" not in required_path:
            return False

        pattern = re.escape(required_path).replace("YYYYMMDD", r"\d{8}")
        regex = re.compile(rf"^{pattern}$")
        for candidate in temp_dir.rglob("*"):
            if not candidate.is_file():
                continue
            rel = candidate.relative_to(temp_dir).as_posix()
            if regex.match(rel):
                return True
        return False

    missing = [path for path in required_files if not required_exists(path)]

    if config["include_bindings"] and not (temp_dir / "bindings").exists():
        missing.append("bindings/")

    if missing:
        missing_str = ", ".join(missing)
        raise FileNotFoundError(f"Release baseline validation failed; missing: {missing_str}")


def create_release_package(tier, output_dir="releases"):
    """Create a clean release package for the specified tier."""
    print(f"\n{'='*60}")
    print(f"Creating {tier.upper()} Package")
    print(f"{'='*60}\n")

    config = TIER_CONFIG[tier]
    root_dir = Path.cwd()
    temp_dir = root_dir / f".temp_release_{tier}"
    output_path = Path(output_dir)
    output_path.mkdir(exist_ok=True)

    zip_filename = (
        output_path / f"UnifiedPhysicsEngine-{VERSION}-{tier}-{RELEASE_DATE}.zip"
    )

    try:
        # Clean temp directory
        if temp_dir.exists():
            shutil.rmtree(temp_dir)
        temp_dir.mkdir()

        print(f"Stage 1: Copying source files...")
        total_files = 0

        # Copy directories
        for dir_name in config["include_dirs"]:
            src_dir = root_dir / dir_name
            if src_dir.exists():
                dst_dir = temp_dir / dir_name
                count = copy_directory_tree(src_dir, dst_dir, config)
                total_files += count
                print(f"  ✓ {dir_name}: {count} files")
            else:
                print(f"  ⚠ {dir_name}: not found")

        # Copy documentation files
        print(f"\nStage 2: Copying documentation...")
        for doc_file in config["include_files"]:
            src_file = root_dir / doc_file
            if src_file.exists():
                dst_file = temp_dir / doc_file
                shutil.copy2(src_file, dst_file)
                total_files += 1
                print(f"  ✓ {doc_file}")
            else:
                print(f"  ⚠ {doc_file}: not found")

        # Add metadata
        metadata = create_metadata(tier)
        metadata_path = temp_dir / "PACKAGE_INFO.json"
        import json
        with open(metadata_path, "w") as f:
            json.dump(metadata, f, indent=2)
        total_files += 1
        print(f"  ✓ PACKAGE_INFO.json")

        # Build tier-specific README
        included_binary_count = count_library_binaries(temp_dir)
        readme_content = f"""# {config['name']}

**Version:** {VERSION}  
**Date:** {RELEASE_DATE}  
**Package:** {tier.upper()}

{config['description']}

## Quick Start

1. Extract this package
2. Review [QUICK_START.md](QUICK_START.md) for setup instructions
3. Check [API_REFERENCE.md](API_REFERENCE.md) for available features

## What's Included

- **Core Headers:** Complete physics API definitions
"""
        if config["include_lib"] and included_binary_count > 0:
            readme_content += (
                f"- **Pre-built Library:** Included ({included_binary_count} binary artifact(s))\n"
            )
        elif config["include_lib"]:
            readme_content += "- **Pre-built Library:** Not included; build from source in this package\n"
        if config["include_tests"]:
            readme_content += "- **Test Suite:** Test sources included (run locally for current pass rate)\n"
        if config["include_bindings"]:
            readme_content += "- **Language Bindings:** Included (see bindings directory)\n"

        readme_content += f"\n- **Examples:** {count_example_sources(root_dir)} source files\n"
        readme_content += "- **Documentation:** Complete API and integration guides\n"

        readme_path = temp_dir / "README_RELEASE.md"
        with open(readme_path, "w") as f:
            f.write(readme_content)
        total_files += 1

        validate_package_baseline(temp_dir, config)

        # Create the zip file
        print(f"\nStage 3: Creating zip package...")
        with zipfile.ZipFile(zip_filename, "w", zipfile.ZIP_DEFLATED) as zipf:
            for file_path in temp_dir.rglob("*"):
                if file_path.is_file():
                    arcname = f"UnifiedPhysicsEngine-{VERSION}-{tier}/" + str(
                        file_path.relative_to(temp_dir)
                    )
                    zipf.write(file_path, arcname)

        zip_size_mb = zip_filename.stat().st_size / (1024 * 1024)
        print(f"\n✓ Package created: {zip_filename.name}")
        print(f"  Size: {zip_size_mb:.2f} MB")
        print(f"  Files: {total_files}")

        return str(zip_filename)

    finally:
        # Clean up temp directory
        if temp_dir.exists():
            shutil.rmtree(temp_dir)


def main():
    parser = argparse.ArgumentParser(
        description="Create clean release packages for Unified Physics Engine"
    )
    parser.add_argument(
        "--tier",
        choices=["buyout"],
        default="buyout",
        help="Package profile (buyout only)",
    )
    parser.add_argument(
        "--output", default="releases", help="Output directory for packages"
    )
    parser.add_argument(
        "--confirm-private-distribution",
        action="store_true",
        help="Required acknowledgment that generated packages are for private confidential distribution only",
    )

    args = parser.parse_args()

    if not args.confirm_private_distribution:
        print(
            "\n✗ Refusing to create release packages without --confirm-private-distribution\n"
            "  This project is in proprietary/exclusive-acquisition mode and package generation\n"
            "  is restricted to private confidential distribution.\n",
            file=sys.stderr,
        )
        return 1

    tiers = [args.tier]

    created_files = []
    for tier in tiers:
        try:
            package_path = create_release_package(tier, args.output)
            created_files.append(package_path)
        except Exception as e:
            print(f"\n✗ Error creating {tier} package: {e}", file=sys.stderr)
            return 1

    print(f"\n{'='*60}")
    print("RELEASE PACKAGE CREATION COMPLETE")
    print(f"{'='*60}")
    print(f"\nCreated {len(created_files)} package(s):")
    for pkg in created_files:
        print(f"  • {Path(pkg).name}")

    print(f"\n✓ All packages are ready for distribution!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
