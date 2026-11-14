#!/usr/bin/env python3
"""Small integration check for UPE buyer onboarding.

This script validates the key roadmap milestones in a compact way:
1. configuration can be generated,
2. the UPE Arena demo builds,
3. the demo executes and prints a deterministic state hash.
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path


def run(command: list[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    print("[UPE integration check]", " ".join(command))
    return subprocess.run(command, cwd=str(cwd), text=True, capture_output=True)


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    build_dir = root / "build"
    if build_dir.exists():
        print("[UPE integration check] Reusing existing build directory")
    else:
        result = run([
            "cmake",
            "-S",
            str(root),
            "-B",
            str(build_dir),
            "-DCMAKE_BUILD_TYPE=Release",
            "-DUPE_BUILD_EXAMPLES=ON",
            "-DUPE_BUILD_TESTS=ON",
            "-DUPE_DETERMINISTIC=ON",
        ], root)
        if result.returncode != 0:
            print(result.stdout)
            print(result.stderr, file=sys.stderr)
            return result.returncode

    binary = build_dir / "bin" / "examples" / "upe_arena.exe"
    if not binary.exists():
        binary = build_dir / "bin" / "examples" / "upe_arena"
    if not binary.exists():
        result = run(["cmake", "--build", str(build_dir), "--parallel"], root)
        if result.returncode != 0:
            print(result.stdout)
            print(result.stderr, file=sys.stderr)
            return result.returncode

    if not binary.exists():
        print("[UPE integration check] ERROR: UPE Arena binary was not produced.", file=sys.stderr)
        return 1

    result = run([str(binary)], root)
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr, file=sys.stderr)
        return result.returncode

    output = result.stdout.strip()
    print(output)
    if "Deterministic:" not in output or "State Hash:" not in output:
        print("[UPE integration check] ERROR: output did not include expected validation markers.", file=sys.stderr)
        return 1

    print("[UPE integration check] PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
