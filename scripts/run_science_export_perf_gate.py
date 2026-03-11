#!/usr/bin/env python3
"""
Run science export performance benchmark and validate perf gates.

Usage examples:
  python scripts/run_science_export_perf_gate.py
  python scripts/run_science_export_perf_gate.py --compare-baseline
  python scripts/run_science_export_perf_gate.py --update-baseline
"""

from __future__ import annotations

import argparse
import csv
import json
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
        build_dir / "bin" / "benchmarks" / "upe_bench_science_export.exe",
        build_dir / "bin" / "benchmarks" / "upe_bench_science_export",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return candidates[0]


def load_benchmarks(path: Path) -> dict[str, dict[str, float]]:
    with path.open("r", encoding="utf-8") as f:
        data = json.load(f)

    benchmarks: dict[str, dict[str, float]] = {}
    for entry in data.get("benchmarks", []):
        if isinstance(entry, dict) and isinstance(entry.get("name"), str):
            name = entry["name"]
            numeric = {}
            for key, value in entry.items():
                if isinstance(value, (int, float)):
                    numeric[key] = float(value)
            benchmarks[name] = numeric
    return benchmarks


def load_previous_run(history_csv_path: Path) -> dict[str, dict[str, float]]:
    if not history_csv_path.exists():
        return {}

    latest_by_scenario: dict[str, dict[str, float]] = {}
    with history_csv_path.open("r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            scenario = row.get("scenario", "")
            if not scenario:
                continue
            try:
                latest_by_scenario[scenario] = {
                    "avg_us": float(row.get("avg_us", "0") or "0"),
                }
            except ValueError:
                continue
    return latest_by_scenario


def pct_delta(current: float, reference: float) -> float | None:
    if reference <= 0.0:
        return None
    return ((current - reference) / reference) * 100.0


def write_trend_artifacts(runs_dir: Path,
                          results_path: Path,
                          baseline_path: Path,
                          timestamp_iso: str) -> None:
    current = load_benchmarks(results_path)
    baseline = load_benchmarks(baseline_path) if baseline_path.exists() else {}

    history_csv = runs_dir / "science_export_perf_history.csv"
    previous = load_previous_run(history_csv)

    header = [
        "timestamp",
        "scenario",
        "avg_us",
        "min_us",
        "max_us",
        "iterations",
        "json_bytes",
        "delta_vs_baseline_percent",
        "delta_vs_previous_percent",
        "results_file",
    ]

    write_header = not history_csv.exists()
    with history_csv.open("a", encoding="utf-8", newline="") as f:
        writer = csv.writer(f)
        if write_header:
            writer.writerow(header)

        delta_report = {
            "schema": "upe.perf.science_export.trend.v1",
            "timestamp": timestamp_iso,
            "results_file": str(results_path),
            "scenarios": {},
        }

        for scenario in sorted(current.keys()):
            metrics = current[scenario]
            avg_us = float(metrics.get("avg_us", 0.0))
            min_us = float(metrics.get("min_us", 0.0))
            max_us = float(metrics.get("max_us", 0.0))
            iterations = int(metrics.get("iterations", 0.0))
            json_bytes = int(metrics.get("json_bytes", 0.0))

            baseline_avg = float(baseline.get(scenario, {}).get("avg_us", 0.0))
            previous_avg = float(previous.get(scenario, {}).get("avg_us", 0.0))

            delta_baseline = pct_delta(avg_us, baseline_avg)
            delta_previous = pct_delta(avg_us, previous_avg)

            writer.writerow([
                timestamp_iso,
                scenario,
                f"{avg_us:.3f}",
                f"{min_us:.3f}",
                f"{max_us:.3f}",
                iterations,
                json_bytes,
                "" if delta_baseline is None else f"{delta_baseline:.3f}",
                "" if delta_previous is None else f"{delta_previous:.3f}",
                str(results_path),
            ])

            delta_report["scenarios"][scenario] = {
                "avg_us": avg_us,
                "min_us": min_us,
                "max_us": max_us,
                "iterations": iterations,
                "json_bytes": json_bytes,
                "delta_vs_baseline_percent": delta_baseline,
                "delta_vs_previous_percent": delta_previous,
            }

    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    trend_json = runs_dir / f"science_export_perf_trend_{stamp}.json"
    with trend_json.open("w", encoding="utf-8") as f:
        json.dump(delta_report, f, indent=2)
        f.write("\n")

    print(f"[UPE] Updated trend history: {history_csv}")
    print(f"[UPE] Wrote trend deltas: {trend_json}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run UPE science export perf benchmark + gate validation")
    parser.add_argument("--build-dir", default="build", help="CMake build directory")
    parser.add_argument("--results", default=None, help="Output JSON path (default proof/RUNS timestamped file)")
    parser.add_argument("--baseline", default="proof/EXPECTED_OUTPUTS/science_export_perf_baseline.json", help="Baseline JSON path")
    parser.add_argument("--threshold", type=float, default=None, help="Regression threshold percent override")
    parser.add_argument("--skip-build", action="store_true", help="Skip benchmark target build")
    parser.add_argument("--compare-baseline", action="store_true", help="Compare current run against baseline")
    parser.add_argument("--update-baseline", action="store_true", help="Overwrite baseline file with current results")
    parser.add_argument("--no-trend-history", action="store_true", help="Disable writing trend history artifacts")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    build_dir = (repo_root / args.build_dir).resolve()
    baseline_path = (repo_root / args.baseline).resolve()

    runs_dir = repo_root / "proof" / "RUNS"
    runs_dir.mkdir(parents=True, exist_ok=True)

    if args.results:
        results_path = (repo_root / args.results).resolve()
    else:
        stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        results_path = (runs_dir / f"science_export_perf_{stamp}.json").resolve()

    if not args.skip_build:
        code = run_command([
            "cmake",
            "--build",
            str(build_dir),
            "--target",
            "upe_bench_science_export",
            "-j",
        ], repo_root)
        if code != 0:
            return code

    binary_path = resolve_benchmark_binary(build_dir)
    code = run_command([str(binary_path), str(results_path)], repo_root)
    if code != 0:
        return code

    gate_cmd = [
        sys.executable,
        str(repo_root / "docs" / "perf_gates.py"),
        "--results",
        str(results_path),
    ]

    if args.compare_baseline and baseline_path.exists():
        gate_cmd.extend(["--baseline", str(baseline_path)])
        if args.threshold is not None:
            gate_cmd.extend(["--threshold", str(args.threshold)])

    code = run_command(gate_cmd, repo_root)
    gate_code = code

    if args.compare_baseline and not baseline_path.exists():
        print(f"[UPE] Baseline missing (skipped compare): {baseline_path}")

    if args.update_baseline:
        if gate_code != 0:
            print("[UPE] Skipping baseline update because perf gate failed")
        else:
            baseline_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(results_path, baseline_path)
            print(f"[UPE] Updated baseline: {baseline_path}")

    if not args.no_trend_history:
        write_trend_artifacts(
            runs_dir=runs_dir,
            results_path=results_path,
            baseline_path=baseline_path,
            timestamp_iso=datetime.now().isoformat(timespec="seconds"),
        )

    if gate_code != 0:
        return gate_code

    print(f"[UPE] Perf run complete: {results_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
