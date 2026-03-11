# Build Guide

This guide documents the supported build paths for the Unified Physics Engine and matches the commands used by CI.

## Requirements

- CMake 3.15 or newer
- A C++20-capable compiler
- Ninja or a native platform build toolchain

Supported compilers:

- Windows: MSVC 2019 or newer
- Linux: GCC 10+ or Clang 10+
- macOS: Apple Clang with C++20 support

## Standard Configure

All documented build paths use the same core options:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DUPE_BUILD_TESTS=ON \
  -DUPE_BUILD_EXAMPLES=ON \
  -DUPE_DETERMINISTIC=ON
```

These flags enable the buyer-critical validation path:

- Unit tests via CTest
- Example binaries for smoke tests
- Deterministic mode for replay and hash certification

## Windows

### MSVC from Developer Command Prompt

```bat
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUPE_BUILD_TESTS=ON -DUPE_BUILD_EXAMPLES=ON -DUPE_DETERMINISTIC=ON
cmake --build build --config Release --parallel
```

### Helper Script

```bat
tools\build_windows_msvc.bat
```

Use this when you want the repository-provided wrapper instead of typing the CMake commands directly.

## Linux

### GCC or Clang

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DUPE_BUILD_TESTS=ON \
  -DUPE_BUILD_EXAMPLES=ON \
  -DUPE_DETERMINISTIC=ON
cmake --build build --parallel
```

### Helper Script

```bash
tools/build_linux.sh
```

## macOS

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DUPE_BUILD_TESTS=ON \
  -DUPE_BUILD_EXAMPLES=ON \
  -DUPE_DETERMINISTIC=ON
cmake --build build --parallel
```

The macOS path is intentionally close to Linux because CI and local usage both rely on the same CMake configuration model.

## Alternative Build Trees

The repository also contains specialized build directories for stricter validation runs:

- `build_warn/` for strict warnings checks
- `build_windows/` for Windows-specific verification snapshots

For normal development, prefer `build/` unless you are reproducing a specific CI stage.

## Outputs

Successful builds generate artifacts under `build/`:

- Libraries in `build/lib/`
- Executables in `build/bin/`
- CTest metadata in `build/Testing/`

## Next Validation Steps

After building:

1. Run [TEST_GUIDE.md](TEST_GUIDE.md).
2. Run [DETERMINISM_GUIDE.md](DETERMINISM_GUIDE.md) if you need replay/hash proof.
3. Review [TROUBLESHOOTING.md](TROUBLESHOOTING.md) if configure or build fails.