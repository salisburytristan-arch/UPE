#!/usr/bin/env python3
"""Run a compact benchmark matrix for the UPE Arena roadmap demo.

This script executes the same deterministic stack scenario at multiple scene sizes
and records performance and hash output in a machine-readable CSV.
"""

from __future__ import annotations

import argparse
import csv
import os
import re
import subprocess
import sys
from pathlib import Path


def find_binary(build_dir: Path) -> Path:
    candidates = [
        build_dir / "bin" / "examples" / "upe_arena.exe",
        build_dir / "bin" / "examples" / "upe_arena",
        build_dir / "bin" / "upe_arena.exe",
        build_dir / "bin" / "upe_arena",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise FileNotFoundError(f"Could not find upe_arena in {build_dir}")


def run_case(binary: Path, grid_size: int, frames: int) -> dict:
    command = [str(binary), str(grid_size), str(frames)]
    completed = subprocess.run(command, capture_output=True, text=True, cwd=str(binary.parent.parent.parent))
    if completed.returncode != 0:
        raise RuntimeError(f"Benchmark run failed for grid={grid_size} frames={frames}:\n{completed.stderr}\n{completed.stdout}")

    output = completed.stdout.strip()
    fields = {}
    for line in output.splitlines():
        if "Bodies:" in line:
            fields["bodies"] = int(line.split(":", 1)[1].strip())
        elif "Frames:" in line:
            fields["frames"] = int(line.split(":", 1)[1].strip())
        elif "Step:" in line:
            values = line.split(":", 1)[1].strip()
            fields["step_ms"] = float(values.split()[0])
        elif "Physics FPS:" in line:
            fields["physics_fps"] = float(line.split(":", 1)[1].strip())
        elif "State Hash:" in line:
            fields["state_hash"] = line.split(":", 1)[1].strip()
        elif "Deterministic:" in line:
            fields["deterministic"] = line.split(":", 1)[1].strip()
    if not fields:
        raise RuntimeError(f"Failed to parse benchmark output for grid={grid_size} frames={frames}\n{output}")
    fields["grid_size"] = grid_size
    fields["frames"] = frames
    return fields


def main() -> int:
    parser = argparse.ArgumentParser(description="Run a benchmark matrix for the UPE Arena roadmap demo")
    parser.add_argument("--build-dir", default="build", type=Path, help="Directory containing the configured build")
    parser.add_argument("--grid-sizes", default="2,3,4,5", help="Comma-separated grid sizes to benchmark")
    parser.add_argument("--frames", type=int, default=180, help="Frames to simulate for each case")
    parser.add_argument("--output", type=Path, default=Path("proof/RUNS/upe_arena_benchmark_matrix.csv"), help="CSV report path")
    args = parser.parse_args()

    build_dir = args.build_dir.resolve()
    binary = find_binary(build_dir)
    grid_sizes = [int(part.strip()) for part in args.grid_sizes.split(",") if part.strip()]
    rows = []

    for grid_size in grid_sizes:
        result = run_case(binary, grid_size, args.frames)
        rows.append(result)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=["grid_size", "bodies", "frames", "step_ms", "physics_fps", "state_hash", "deterministic"],
        )
        writer.writeheader()
        writer.writerows(rows)

    print(f"Benchmark matrix written to {args.output}")
    for row in rows:
        print(
            f"grid={row['grid_size']} bodies={row['bodies']} frames={row['frames']} "
            f"step_ms={row['step_ms']:.3f} fps={row['physics_fps']:.1f} hash={row['state_hash']}"
        )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # pragma: no cover
        print(f"[UPE benchmark matrix] ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
