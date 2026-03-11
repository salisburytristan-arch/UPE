#!/usr/bin/env python3
"""Strict warnings-as-errors build gate."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path


def _run(cmd: list[str], cwd: Path) -> int:
    print("$", " ".join(cmd))
    return subprocess.run(cmd, cwd=cwd).returncode


def _configure_command(build_dir: str) -> list[str]:
    cmd = [
        "cmake",
        "-S",
        ".",
        "-B",
        build_dir,
        "-DCMAKE_BUILD_TYPE=Release",
        "-DUPE_DETERMINISTIC=ON",
        "-DUPE_BUILD_TESTS=ON",
        "-DUPE_BUILD_EXAMPLES=ON",
        "-DUPE_WARNINGS_AS_ERRORS=ON",
    ]

    if shutil.which("ninja"):
        cmd.extend(["-G", "Ninja"])

    if os.name == "nt" and shutil.which("cl") is None:
        if shutil.which("g++"):
            cmd.append("-DCMAKE_CXX_COMPILER=g++")
        elif shutil.which("clang++"):
            cmd.append("-DCMAKE_CXX_COMPILER=clang++")

    return cmd


def main() -> int:
    parser = argparse.ArgumentParser(description="Run strict warnings-as-errors build gate")
    parser.add_argument("--build-dir", default="build_warn", help="Strict gate build directory")
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]

    configure_cmd = _configure_command(args.build_dir)
    if _run(configure_cmd, root) != 0:
        return 1

    build_cmd = ["cmake", "--build", args.build_dir, "--config", "Release", "--parallel"]
    if _run(build_cmd, root) != 0:
        return 1

    print("Strict warnings gate passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
