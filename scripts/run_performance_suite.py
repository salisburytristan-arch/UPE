#!/usr/bin/env python3
"""Run the UPE performance suite and emit JSON, CSV, Markdown, and HTML artifacts."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path


def run_command(command: list[str], cwd: Path) -> int:
    print("[UPE]", " ".join(command))
    completed = subprocess.run(command, cwd=str(cwd))
    return completed.returncode


def resolve_benchmark_binary(build_dir: Path) -> Path:
    candidates = [
        build_dir / "bin" / "benchmarks" / "upe_bench_performance_suite.exe",
        build_dir / "bin" / "benchmarks" / "upe_bench_performance_suite",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return candidates[0]


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the UPE performance suite")
    parser.add_argument("--build-dir", default="build", help="CMake build directory")
    parser.add_argument("--output-prefix", default=None, help="Output prefix under proof/RUNS")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    build_dir = (repo_root / args.build_dir).resolve()
    runs_dir = repo_root / "proof" / "RUNS"
    runs_dir.mkdir(parents=True, exist_ok=True)

    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    prefix = args.output_prefix or f"performance_suite_{stamp}"
    json_path = (runs_dir / f"{prefix}.json").resolve()
    csv_path = (runs_dir / f"{prefix}.csv").resolve()
    md_path = (runs_dir / f"{prefix}.md").resolve()
    html_path = (runs_dir / f"{prefix}.html").resolve()

    code = run_command([
        "cmake",
        "--build",
        str(build_dir),
        "--target",
        "upe_bench_performance_suite",
        "-j",
    ], repo_root)
    if code != 0:
        return code

    binary_path = resolve_benchmark_binary(build_dir)
    code = run_command([str(binary_path), str(json_path), str(csv_path), str(md_path), str(html_path)], repo_root)
    if code != 0:
        return code

    latest_json = repo_root / "proof" / "latest_performance_report.json"
    latest_csv = repo_root / "proof" / "latest_performance_report.csv"
    latest_md = repo_root / "proof" / "latest_performance_report.md"
    latest_html = repo_root / "proof" / "latest_performance_report.html"
    shutil.copyfile(json_path, latest_json)
    shutil.copyfile(csv_path, latest_csv)
    shutil.copyfile(md_path, latest_md)
    shutil.copyfile(html_path, latest_html)

    print(f"[UPE] Performance suite complete: {json_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())