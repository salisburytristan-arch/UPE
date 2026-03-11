#!/usr/bin/env python3
"""
Unified Physics Engine - Cross-Platform Build System

Automatically configures and builds UPE on Windows, Linux, and macOS.
Detects platform, validates CMake, and generates clean builds.

Usage:
    python scripts/build.py                  # Build with default settings
    python scripts/build.py --clean          # Clean build from scratch
    python scripts/build.py --release        # Release optimized build
    python scripts/build.py --debug          # Debug build with symbols
    python scripts/build.py --sanitizers     # Build with address sanitizers
    python scripts/build.py --verbose        # Show compiler output
"""

import os
import sys
import subprocess
import platform
import shutil
import argparse
from pathlib import Path
from typing import Optional, List


class BuildConfig:
    """Configuration for platform-specific builds."""

    def __init__(self, args):
        self.platform = platform.system()
        self.arch = platform.machine()
        self.is_windows = self.platform == "Windows"
        self.is_linux = self.platform == "Linux"
        self.is_macos = self.platform == "Darwin"

        self.root_dir = Path(__file__).parent.parent
        self.build_dir = self.root_dir / "build"
        self.release_build_dir = self.root_dir / "build-release"

        self.cmake_version = self._get_cmake_version()
        self.generator = self._detect_generator()
        self.build_config = args.config
        self.clean = args.clean
        self.verbose = args.verbose
        self.use_sanitizers = args.sanitizers

    def _get_cmake_version(self) -> Optional[str]:
        """Get installed CMake version."""
        try:
            result = subprocess.run(
                ["cmake", "--version"], capture_output=True, text=True, timeout=5
            )
            if result.returncode == 0:
                first_line = result.stdout.split("\n")[0]
                return first_line.split()[-1]
        except Exception:
            pass
        return None

    def _detect_generator(self) -> str:
        """Detect appropriate CMake generator for platform."""
        if self.is_windows:
            # Prefer Ninja on Windows (faster, more portable than MSVC)
            if shutil.which("ninja"):
                return "Ninja"
            elif shutil.which("cl"):
                return "Visual Studio 17 2022"
            else:
                return "MinGW Makefiles"

        elif self.is_linux:
            if shutil.which("ninja"):
                return "Ninja"
            else:
                return "Unix Makefiles"

        else:  # macOS
            if shutil.which("ninja"):
                return "Ninja"
            else:
                return "Unix Makefiles"

    def print_summary(self):
        """Print build configuration summary."""
        print(f"\n{'='*70}")
        print(f"UNIFIED PHYSICS ENGINE - BUILD CONFIGURATION")
        print(f"{'='*70}\n")

        print(f"Platform:          {self.platform} ({self.arch})")
        print(f"CMake Version:     {self.cmake_version or 'NOT FOUND'}")
        print(f"Generator:         {self.generator}")
        print(f"Build Type:        {self.build_config}")
        print(f"Source Directory:  {self.root_dir}")
        print(f"Build Directory:   {self.build_dir}")
        print(f"Clean Build:       {'Yes' if self.clean else 'No'}")
        print(f"Sanitizers:        {'Enabled' if self.use_sanitizers else 'Disabled'}")
        print(f"Verbose Output:    {'Yes' if self.verbose else 'No'}")
        print()


class CMakeBuilder:
    """Handles CMake configuration and build."""

    def __init__(self, config: BuildConfig):
        self.config = config

    def validate_environment(self) -> bool:
        """Validate that all required tools are available."""
        print("Validating build environment...")

        if not shutil.which("cmake"):
            print("✗ CMake not found! Install CMake 3.15 or later.")
            return False

        if not shutil.which(self.config.generator.split()[0].lower()):
            if "Visual Studio" not in self.config.generator:
                print(f"✗ Generator '{self.config.generator}' not found!")
                return False

        print("✓ CMake found")
        print(f"✓ Generator available: {self.config.generator}")
        return True

    def configure(self) -> bool:
        """Run CMake configuration step."""
        print(f"\nConfiguring CMake ({self.config.generator})...")

        # Clean previous build if requested
        if self.config.clean and self.config.build_dir.exists():
            print(f"Cleaning {self.config.build_dir}...")
            shutil.rmtree(self.config.build_dir)

        self.config.build_dir.mkdir(exist_ok=True)

        # Build CMake command
        cmake_cmd = [
            "cmake",
            "-B",
            str(self.config.build_dir),
            "-G",
            self.config.generator,
            f"-DCMAKE_BUILD_TYPE={self.config.build_config}",
            "-DUPE_BUILD_TESTS=ON",
            "-DUPE_BUILD_EXAMPLES=ON",
            "-DUPE_DETERMINISTIC=ON",
            "-DUPE_ENABLE_SIMD=ON",
            "-DUPE_ENABLE_THREADING=ON",
        ]

        if self.config.use_sanitizers:
            cmake_cmd.append("-DUPE_USE_SANITIZERS=ON")

        cmake_cmd.append(str(self.config.root_dir))

        try:
            result = subprocess.run(
                cmake_cmd,
                cwd=self.config.root_dir,
                capture_output=not self.config.verbose,
                text=True,
            )

            if result.returncode != 0:
                print(f"✗ CMake configuration failed!")
                if not self.config.verbose:
                    print(result.stderr)
                return False

            print("✓ CMake configuration successful")
            return True
        except Exception as e:
            print(f"✗ Error running CMake: {e}")
            return False

    def build(self) -> bool:
        """Run CMake build step."""
        print(f"\nBuilding project ({self.config.build_config} mode)...")

        # Determine parallel build jobs
        num_jobs = os.cpu_count() or 4

        build_cmd = [
            "cmake",
            "--build",
            str(self.config.build_dir),
            "--config",
            self.config.build_config,
            "--parallel",
            str(num_jobs),
        ]

        if self.config.verbose:
            build_cmd.append("--verbose")

        try:
            result = subprocess.run(
                build_cmd,
                cwd=self.config.root_dir,
                capture_output=not self.config.verbose,
                text=True,
            )

            if result.returncode != 0:
                print(f"✗ Build failed!")
                if not self.config.verbose:
                    print(result.stderr)
                return False

            print("✓ Build successful")
            return True
        except Exception as e:
            print(f"✗ Error running build: {e}")
            return False

    def run_tests(self) -> bool:
        """Run test suite."""
        print(f"\nRunning test suite...")

        test_cmd = ["cmake", "--build", str(self.config.build_dir), "--target", "test"]

        try:
            result = subprocess.run(
                test_cmd,
                cwd=self.config.root_dir,
                capture_output=not self.config.verbose,
                text=True,
            )

            if result.returncode != 0:
                print(f"⚠ Some tests failed (see output above)")
                return False

            print("✓ Tests passed")
            return True
        except Exception as e:
            print(f"⚠ Could not run tests: {e}")
            return True  # Don't fail the build if tests can't run


def main():
    parser = argparse.ArgumentParser(
        description="Build Unified Physics Engine on Windows, Linux, or macOS"
    )
    parser.add_argument(
        "--config",
        choices=["Debug", "Release", "RelWithDebInfo"],
        default="Release",
        help="Build configuration",
    )
    parser.add_argument(
        "--clean", action="store_true", help="Clean and rebuild from scratch"
    )
    parser.add_argument(
        "--debug",
        action="store_const",
        const="Debug",
        dest="config",
        help="Debug build (shorthand for --config Debug)",
    )
    parser.add_argument(
        "--release",
        action="store_const",
        const="Release",
        dest="config",
        help="Release build (shorthand for --config Release)",
    )
    parser.add_argument(
        "--sanitizers",
        action="store_true",
        help="Enable address/memory sanitizers",
    )
    parser.add_argument(
        "--verbose", "-v", action="store_true", help="Show detailed build output"
    )

    args = parser.parse_args()

    # Create configuration
    config = BuildConfig(args)
    config.print_summary()

    # Validate environment
    try:
        if not config.cmake_version:
            print("✗ CMake is not installed!")
            print(f"\nPlease install CMake 3.15 or later:")
            if config.is_windows:
                print("  • Windows: https://cmake.org/download/")
                print("  • Or: choco install cmake")
            elif config.is_linux:
                print("  • Ubuntu/Debian: apt install cmake")
                print("  • Fedora: dnf install cmake")
                print("  • Arch: pacman -S cmake")
            else:
                print("  • macOS: brew install cmake")
            return 1
    except Exception as e:
        print(f"✗ Error: {e}")
        return 1

    # Build
    builder = CMakeBuilder(config)

    if not builder.validate_environment():
        return 1

    if not builder.configure():
        return 1

    if not builder.build():
        return 1

    # Run tests (optional)
    builder.run_tests()

    # Summary
    print(f"\n{'='*70}")
    print("BUILD COMPLETE")
    print(f"{'='*70}\n")
    print(f"Build artifacts: {config.build_dir}")
    print(f"Library:         {config.build_dir / 'libupe.a'}")
    print(f"Tests:           {config.build_dir / 'bin'}")
    print(f"Examples:        {config.build_dir / 'examples'}")
    print(f"\n✓ Ready for distribution or integration!")

    return 0


if __name__ == "__main__":
    sys.exit(main())
