#!/usr/bin/env python3
"""Run the full buyer-readiness evidence suite for the UPE roadmap.

This command is intentionally simple and deterministic:
1. configure the project,
2. build the example and test target,
3. run the UPE Arena showcase,
4. run the benchmark matrix,
5. print a concise summary suitable for buyer review.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def run(command: list[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    print("[UPE roadmap evidence]", " ".join(command))
    return subprocess.run(command, cwd=str(cwd), text=True, capture_output=True)


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    build_dir = root / "build"

    configure = run(
        [
            "cmake",
            "-S",
            str(root),
            "-B",
            str(build_dir),
            "-G",
            "Ninja",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DUPE_BUILD_EXAMPLES=ON",
            "-DUPE_BUILD_TESTS=ON",
            "-DUPE_DETERMINISTIC=ON",
        ],
        root,
    )
    if configure.returncode != 0:
        print(configure.stdout)
        print(configure.stderr, file=sys.stderr)
        return configure.returncode

    build = run(["cmake", "--build", str(build_dir), "--target", "upe_arena", "--parallel"], root)
    if build.returncode != 0:
        print(build.stdout)
        print(build.stderr, file=sys.stderr)
        return build.returncode

    test = run(["ctest", "--test-dir", str(build_dir), "--output-on-failure"], root)
    if test.returncode != 0:
        print(test.stdout)
        print(test.stderr, file=sys.stderr)
        return test.returncode

    demo = run([str(build_dir / "bin" / "examples" / "upe_arena.exe")], root)
    if demo.returncode != 0:
        print(demo.stdout)
        print(demo.stderr, file=sys.stderr)
        return demo.returncode

    benchmark = run(
        [
            sys.executable,
            str(root / "tools" / "run_benchmark_matrix.py"),
            "--build-dir",
            str(build_dir),
            "--grid-sizes",
            "2,3,4,5",
            "--frames",
            "60",
        ],
        root,
    )
    if benchmark.returncode != 0:
        print(benchmark.stdout)
        print(benchmark.stderr, file=sys.stderr)
        return benchmark.returncode

    print("\n[UPE roadmap evidence] SUMMARY")
    print("- configure: PASS")
    print("- build: PASS")
    print("- tests: PASS")
    print("- demo output: PASS")
    print("- benchmark matrix: PASS")
    print("- evidence_path: proof/RUNS/upe_arena_benchmark_matrix.csv")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
