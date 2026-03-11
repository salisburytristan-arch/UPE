#!/usr/bin/env python3
"""Run the full local quality sweep for Unified Physics Engine."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path


@dataclass
class Step:
    name: str
    command: list[str]


def _run_step(step: Step, root: Path) -> tuple[bool, float]:
    print(f"\n=== {step.name} ===")
    print("$", " ".join(step.command))
    start = time.perf_counter()
    result = subprocess.run(step.command, cwd=root)
    elapsed = time.perf_counter() - start
    ok = result.returncode == 0
    status = "PASS" if ok else "FAIL"
    print(f"[{status}] {step.name} ({elapsed:.1f}s)")
    return ok, elapsed


def _platform_steps(
    python_exe: str,
    release_dir: str,
    build_dir_windows: str,
    build_dir_unix: str,
    strict_warnings: bool,
) -> list[Step]:
    steps = [
        Step("Markdown link integrity", [python_exe, "scripts/check_markdown_links.py"]),
        Step("Topic truth audit freshness", [python_exe, "scripts/generate_topic_truth_audit.py", "--check"]),
    ]

    if strict_warnings:
        steps.append(
            Step("Strict warnings-as-errors build", [python_exe, "scripts/run_strict_warnings_gate.py"])
        )

    if os.name == "nt":
        steps.extend(
            [
                Step("Build (Windows)", ["cmd", "/c", "tools\\build_windows_msvc.bat", build_dir_windows]),
                Step("Run tests (Windows)", ["cmd", "/c", "tools\\run_tests.bat", build_dir_windows]),
                Step("Examples smoketest (Windows)", ["cmd", "/c", "tools\\run_examples_smoketest.bat", build_dir_windows]),
                Step("Determinism proof (Windows)", ["cmd", "/c", "tools\\run_determinism_proof.bat", build_dir_windows]),
            ]
        )
    else:
        steps.extend(
            [
                Step("Build (Linux/macOS)", ["bash", "tools/build_linux.sh", build_dir_unix]),
                Step("Run tests (Linux/macOS)", ["bash", "tools/run_tests.sh", build_dir_unix]),
                Step("Examples smoketest (Linux/macOS)", ["bash", "tools/run_examples_smoketest.sh", build_dir_unix]),
                Step("Determinism proof (Linux/macOS)", ["bash", "tools/run_determinism_proof.sh", build_dir_unix]),
            ]
        )

    steps.append(
        Step(
            "Release gate with package creation",
            [python_exe, "scripts/verify_release_gate.py", "--create", "--dir", release_dir],
        )
    )
    return steps


def main() -> int:
    parser = argparse.ArgumentParser(description="Run full local quality verification sweep")
    parser.add_argument(
        "--release-dir",
        default="releases_ci",
        help="Output directory for release package gate",
    )
    parser.add_argument(
        "--build-dir-windows",
        default="build_windows",
        help="Build directory for Windows quality steps",
    )
    parser.add_argument(
        "--build-dir-unix",
        default="build_linux",
        help="Build directory for Linux/macOS quality steps",
    )
    parser.add_argument(
        "--strict-warnings",
        action="store_true",
        help="Enable strict warnings-as-errors gate",
    )
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    python_exe = sys.executable

    steps = _platform_steps(
        python_exe,
        args.release_dir,
        args.build_dir_windows,
        args.build_dir_unix,
        args.strict_warnings,
    )

    failures: list[str] = []
    timings: list[tuple[str, float]] = []

    sweep_start = time.perf_counter()
    for step in steps:
        ok, elapsed = _run_step(step, root)
        timings.append((step.name, elapsed))
        if not ok:
            failures.append(step.name)
            break
    total_elapsed = time.perf_counter() - sweep_start

    print("\n=== Verification Summary ===")
    for name, elapsed in timings:
        print(f"- {name}: {elapsed:.1f}s")
    print(f"- Total: {total_elapsed:.1f}s")

    if failures:
        print("\nFULL SWEEP FAILED")
        for name in failures:
            print(f"- Failed step: {name}")
        return 1

    print("\nFULL SWEEP PASSED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
