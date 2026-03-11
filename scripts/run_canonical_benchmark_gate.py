#!/usr/bin/env python3
"""Run canonical benchmark suite, emit artifacts, and validate regressions."""

from __future__ import annotations

import argparse
import csv
import json
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path


SCENE_NAME_MAP = {
    "Sphere Stack (10K)": "rigid_body_10k",
    "Ragdoll Stack": "ragdoll_stack",
    "Vehicle Scene": "vehicle_scene",
    "Cloth + Wind": "cloth_10k_verts",
    "Fluid Spray": "sph_fluid_50k",
}


def run_command(command: list[str], cwd: Path) -> int:
    print("[UPE]", " ".join(command))
    completed = subprocess.run(command, cwd=str(cwd))
    return completed.returncode


def resolve_benchmark_binary(build_dir: Path) -> Path:
    candidates = [
        build_dir / "bin" / "benchmarks" / "upe_bench_canonical_scenes.exe",
        build_dir / "bin" / "benchmarks" / "upe_bench_canonical_scenes",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return candidates[0]


def load_csv_results(path: Path) -> list[dict[str, float | str]]:
    rows: list[dict[str, float | str]] = []
    with path.open("r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            benchmark_name = str(row["Benchmark"])
            rows.append(
                {
                    "name": SCENE_NAME_MAP.get(benchmark_name, benchmark_name.lower().replace(" ", "_").replace("+", "plus")),
                    "display_name": benchmark_name,
                    "frame_time_ms": float(row["Avg_Step_ms"]),
                    "min_step_ms": float(row["Min_Step_ms"]),
                    "max_step_ms": float(row["Max_Step_ms"]),
                    "max_contacts": float(row["Max_Contacts"]),
                    "avg_constraints": float(row["Avg_Constraints"]),
                    "total_time_ms": float(row["Total_Time_ms"]),
                }
            )
    return rows


def load_json_results(path: Path) -> dict[str, dict[str, float]]:
    if not path.exists():
        return {}
    with path.open("r", encoding="utf-8") as f:
        data = json.load(f)

    results: dict[str, dict[str, float]] = {}
    for entry in data.get("benchmarks", []):
        if not isinstance(entry, dict) or not isinstance(entry.get("name"), str):
            continue
        name = entry["name"]
        numeric = {k: float(v) for k, v in entry.items() if isinstance(v, (int, float))}
        results[name] = numeric
    return results


def write_results_json(rows: list[dict[str, float | str]], path: Path) -> None:
    payload = {
        "schema": "upe.perf.canonical_scenes.v1",
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "benchmarks": rows,
    }
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        json.dump(payload, f, indent=2)
        f.write("\n")


def pct_delta(current: float, baseline: float) -> float | None:
    if baseline <= 0.0:
        return None
    return ((current - baseline) / baseline) * 100.0


def compare_against_baseline(current_path: Path, baseline_path: Path, threshold_percent: float) -> int:
    current = load_json_results(current_path)
    baseline = load_json_results(baseline_path)
    if not baseline:
        print(f"[UPE] Baseline missing or empty (skipped compare): {baseline_path}")
        return 0

    failures: list[str] = []
    for scenario, metrics in current.items():
        if scenario not in baseline:
            continue
        current_frame = float(metrics.get("frame_time_ms", 0.0))
        baseline_frame = float(baseline[scenario].get("frame_time_ms", 0.0))
        delta = pct_delta(current_frame, baseline_frame)
        if delta is not None and delta > threshold_percent:
            failures.append(
                f"{scenario}: frame time regression {delta:.2f}% exceeds {threshold_percent:.2f}% "
                f"(current={current_frame:.3f} ms, baseline={baseline_frame:.3f} ms)"
            )

    if failures:
        print("[UPE] Canonical benchmark gate failed")
        for failure in failures:
            print(f"- {failure}")
        return 1

    print("[UPE] Canonical benchmark gate passed")
    return 0


def write_trend_artifacts(runs_dir: Path,
                          rows: list[dict[str, float | str]],
                          baseline_path: Path,
                          results_json_path: Path,
                          timestamp_iso: str) -> None:
    baseline = load_json_results(baseline_path)
    history_csv = runs_dir / "canonical_benchmark_history.csv"
    write_header = not history_csv.exists()

    header = [
        "timestamp",
        "scenario",
        "display_name",
        "frame_time_ms",
        "min_step_ms",
        "max_step_ms",
        "max_contacts",
        "avg_constraints",
        "total_time_ms",
        "delta_vs_baseline_percent",
        "results_file",
    ]

    with history_csv.open("a", encoding="utf-8", newline="") as f:
        writer = csv.writer(f)
        if write_header:
            writer.writerow(header)

        trend_payload = {
            "schema": "upe.perf.canonical_scenes.trend.v1",
            "timestamp": timestamp_iso,
            "results_file": str(results_json_path),
            "scenarios": {},
        }

        for row in rows:
            scenario = str(row["name"])
            baseline_frame = float(baseline.get(scenario, {}).get("frame_time_ms", 0.0))
            delta = pct_delta(float(row["frame_time_ms"]), baseline_frame)
            writer.writerow([
                timestamp_iso,
                scenario,
                row["display_name"],
                f"{float(row['frame_time_ms']):.3f}",
                f"{float(row['min_step_ms']):.3f}",
                f"{float(row['max_step_ms']):.3f}",
                int(float(row["max_contacts"])),
                int(float(row["avg_constraints"])),
                f"{float(row['total_time_ms']):.3f}",
                "" if delta is None else f"{delta:.3f}",
                str(results_json_path),
            ])

            trend_payload["scenarios"][scenario] = {
                "display_name": row["display_name"],
                "frame_time_ms": float(row["frame_time_ms"]),
                "min_step_ms": float(row["min_step_ms"]),
                "max_step_ms": float(row["max_step_ms"]),
                "max_contacts": float(row["max_contacts"]),
                "avg_constraints": float(row["avg_constraints"]),
                "total_time_ms": float(row["total_time_ms"]),
                "delta_vs_baseline_percent": delta,
            }

    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    trend_json = runs_dir / f"canonical_benchmark_trend_{stamp}.json"
    with trend_json.open("w", encoding="utf-8") as f:
        json.dump(trend_payload, f, indent=2)
        f.write("\n")

    print(f"[UPE] Updated trend history: {history_csv}")
    print(f"[UPE] Wrote trend deltas: {trend_json}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run UPE canonical benchmarks and validate regressions")
    parser.add_argument("--build-dir", default="build", help="CMake build directory")
    parser.add_argument("--results-json", default=None, help="Output JSON path")
    parser.add_argument("--results-csv", default=None, help="Output CSV copy path")
    parser.add_argument("--report-html", default=None, help="Output HTML report path")
    parser.add_argument("--baseline", default="proof/EXPECTED_OUTPUTS/canonical_benchmark_baseline.json", help="Optional baseline JSON path")
    parser.add_argument("--threshold", type=float, default=20.0, help="Regression threshold percent")
    parser.add_argument("--skip-build", action="store_true", help="Skip benchmark target build")
    parser.add_argument("--compare-baseline", action="store_true", help="Compare results against baseline")
    parser.add_argument("--update-baseline", action="store_true", help="Overwrite baseline with current results if the gate passes")
    parser.add_argument("--no-trend-history", action="store_true", help="Disable trend artifact generation")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    build_dir = (repo_root / args.build_dir).resolve()
    baseline_path = (repo_root / args.baseline).resolve()
    runs_dir = repo_root / "proof" / "RUNS"
    runs_dir.mkdir(parents=True, exist_ok=True)

    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    results_json = (repo_root / args.results_json).resolve() if args.results_json else (runs_dir / f"canonical_benchmarks_{stamp}.json").resolve()
    results_csv = (repo_root / args.results_csv).resolve() if args.results_csv else (runs_dir / f"canonical_benchmarks_{stamp}.csv").resolve()
    report_html = (repo_root / args.report_html).resolve() if args.report_html else (runs_dir / f"canonical_benchmark_report_{stamp}.html").resolve()

    if not args.skip_build:
        code = run_command([
            "cmake",
            "--build",
            str(build_dir),
            "--target",
            "upe_bench_canonical_scenes",
            "-j",
        ], repo_root)
        if code != 0:
            return code

    benchmark_binary = resolve_benchmark_binary(build_dir)
    if not benchmark_binary.exists():
        print(f"[UPE] Missing canonical benchmark binary: {benchmark_binary}")
        return 1

    csv_in_build = build_dir / "benchmark_results.csv"
    if csv_in_build.exists():
        csv_in_build.unlink()

    code = run_command([str(benchmark_binary)], build_dir)
    if code != 0:
        return code

    if not csv_in_build.exists():
        print(f"[UPE] Benchmark CSV not produced: {csv_in_build}")
        return 1

    results_csv.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(csv_in_build, results_csv)
    rows = load_csv_results(results_csv)
    write_results_json(rows, results_json)

    plot_code = run_command([
        sys.executable,
        str(repo_root / "scripts" / "plot_benchmarks.py"),
        str(results_csv),
    ], repo_root)
    if plot_code != 0:
        return plot_code

    generated_report = repo_root / "benchmark_report.html"
    if generated_report.exists():
        report_html.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(generated_report), str(report_html))

    gate_code = 0
    if args.compare_baseline:
        gate_code = compare_against_baseline(results_json, baseline_path, args.threshold)

    if args.update_baseline:
        if gate_code != 0:
            print("[UPE] Skipping baseline update because canonical benchmark gate failed")
        else:
            baseline_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(results_json, baseline_path)
            print(f"[UPE] Updated baseline: {baseline_path}")

    if not args.no_trend_history:
        write_trend_artifacts(
            runs_dir=runs_dir,
            rows=rows,
            baseline_path=baseline_path,
            results_json_path=results_json,
            timestamp_iso=datetime.now().isoformat(timespec="seconds"),
        )

    print(f"[UPE] Canonical benchmark CSV: {results_csv}")
    print(f"[UPE] Canonical benchmark JSON: {results_json}")
    print(f"[UPE] Canonical benchmark report: {report_html}")
    return gate_code


if __name__ == "__main__":
    raise SystemExit(main())