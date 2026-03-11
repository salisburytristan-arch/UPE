#!/usr/bin/env python3
"""One-command deterministic proof path for buyers."""

from __future__ import annotations

import argparse
import glob
import os
import subprocess
import sys
from pathlib import Path


def run(command: list[str], cwd: Path) -> int:
    print("[UPE]", " ".join(command))
    return subprocess.run(command, cwd=str(cwd)).returncode


def latest_log(root: Path) -> Path | None:
    candidates = sorted(glob.glob(str(root / "proof" / "RUNS" / "determinism_*.log")))
    if not candidates:
        return None
    return Path(candidates[-1])


def main() -> int:
    parser = argparse.ArgumentParser(description="Run one-command deterministic proof")
    parser.add_argument("--build-dir", default="build_proof", help="Build directory for proof run")
    parser.add_argument("--config", default="Release", choices=["Debug", "Release"], help="Build configuration")
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    build_dir = args.build_dir

    configure = [
        "cmake",
        "-S",
        ".",
        "-B",
        build_dir,
        f"-DCMAKE_BUILD_TYPE={args.config}",
        "-DUPE_BUILD_TESTS=ON",
        "-DUPE_BUILD_EXAMPLES=OFF",
        "-DUPE_DETERMINISTIC=ON",
    ]
    if run(configure, root) != 0:
        return 1

    build = ["cmake", "--build", build_dir, "--parallel"]
    if run(build, root) != 0:
        return 1

    if os.name == "nt":
        proof = ["cmd", "/c", "tools\\run_determinism_proof.bat", build_dir]
    else:
        proof = ["bash", "tools/run_determinism_proof.sh", build_dir]

    if run(proof, root) != 0:
        return 1

    log_path = latest_log(root)
    print("\n[UPE] Determinism proof completed")
    if log_path:
        print(f"[UPE] Inspect log: {log_path}")
    print("[UPE] PASS condition: hash_run_1 == hash_run_2 and MATCH=true")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
