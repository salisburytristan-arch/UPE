#!/usr/bin/env python3
"""Run world-step benchmark suite, emit artifacts, and validate regressions."""

from __future__ import annotations

import argparse
import csv
import json
import re
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path

CSV_HEADER = "bodyCount,stepCount,totalMs,avgMs,stepsPerSec,activeBodies,pairs,contacts,constraints,islands,solverIters,solverThreads,solverIslandTasks,solverParallelIslands,solverSerialIslands,ccdCandidates,ccdTested,ccdHits,frameHash"


def run_command(command: list[str], cwd: Path) -> int:
    print("[UPE]", " ".join(command))
    completed = subprocess.run(command, cwd=str(cwd))
    return completed.returncode


def resolve_benchmark_binary(build_dir: Path) -> Path:
    candidates = [
        build_dir / "bin" / "benchmarks" / "upe_bench_world_step_v2.exe",
        build_dir / "bin" / "benchmarks" / "upe_bench_world_step_v2",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return candidates[0]


def parse_world_step_output(output: str) -> list[dict[str, float | str]]:
    rows: list[dict[str, float | str]] = []
    capture = False
    for raw in output.splitlines():
        line = raw.strip()
        if not line:
            continue
        if line == CSV_HEADER:
            capture = True
            continue
        if not capture:
            continue
        if not re.match(r"^\d+,", line):
            continue
        parts = line.split(",")
        if len(parts) != 19:
            continue

        body_count = int(parts[0])
        display_name = f"Rigid Bodies ({body_count})"
        rows.append(
            {
                "name": f"rigid_body_{body_count}",
                "display_name": display_name,
                "body_count": body_count,
                "step_count": int(parts[1]),
                "total_time_ms": float(parts[2]),
                "frame_time_ms": float(parts[3]),
                "steps_per_sec": float(parts[4]),
                "active_bodies": float(parts[5]),
                "pairs": float(parts[6]),
                "contacts": float(parts[7]),
                "avg_constraints": float(parts[8]),
                "islands": float(parts[9]),
                "solver_iters": float(parts[10]),
                "solver_threads": float(parts[11]),
                "ccd_candidates": float(parts[15]),
                "ccd_tested": float(parts[16]),
                "ccd_hits": float(parts[17]),
            }
        )
    return rows


def write_plot_csv(rows: list[dict[str, float | str]], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(
            [
                "Benchmark",
                "Avg_Step_ms",
                "Min_Step_ms",
                "Max_Step_ms",
                "Max_Contacts",
                "Avg_Constraints",
                "Total_Time_ms",
            ]
        )
        for row in rows:
            # world_step benchmark exposes average frame time; mirror into min/max for chart compatibility
            writer.writerow(
                [
                    row["display_name"],
                    f"{float(row['frame_time_ms']):.6f}",
                    f"{float(row['frame_time_ms']):.6f}",
                    f"{float(row['frame_time_ms']):.6f}",
                    int(float(row["contacts"])),
                    int(float(row["avg_constraints"])),
                    f"{float(row['total_time_ms']):.3f}",
                ]
            )


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
        "schema": "upe.perf.world_step.v1",
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
        print("[UPE] World-step benchmark gate failed")
        for failure in failures:
            print(f"- {failure}")
        return 1

    print("[UPE] World-step benchmark gate passed")
    return 0


def write_trend_artifacts(
    runs_dir: Path,
    rows: list[dict[str, float | str]],
    baseline_path: Path,
    results_json_path: Path,
    timestamp_iso: str,
) -> None:
    baseline = load_json_results(baseline_path)
    history_csv = runs_dir / "world_step_benchmark_history.csv"
    write_header = not history_csv.exists()

    header = [
        "timestamp",
        "scenario",
        "display_name",
        "frame_time_ms",
        "active_bodies",
        "pairs",
        "contacts",
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
            "schema": "upe.perf.world_step.trend.v1",
            "timestamp": timestamp_iso,
            "results_file": str(results_json_path),
            "scenarios": {},
        }

        for row in rows:
            scenario = str(row["name"])
            baseline_frame = float(baseline.get(scenario, {}).get("frame_time_ms", 0.0))
            delta = pct_delta(float(row["frame_time_ms"]), baseline_frame)
            writer.writerow(
                [
                    timestamp_iso,
                    scenario,
                    row["display_name"],
                    f"{float(row['frame_time_ms']):.3f}",
                    int(float(row["active_bodies"])),
                    int(float(row["pairs"])),
                    int(float(row["contacts"])),
                    int(float(row["avg_constraints"])),
                    f"{float(row['total_time_ms']):.3f}",
                    "" if delta is None else f"{delta:.3f}",
                    str(results_json_path),
                ]
            )

            trend_payload["scenarios"][scenario] = {
                "display_name": row["display_name"],
                "frame_time_ms": float(row["frame_time_ms"]),
                "active_bodies": float(row["active_bodies"]),
                "pairs": float(row["pairs"]),
                "contacts": float(row["contacts"]),
                "avg_constraints": float(row["avg_constraints"]),
                "total_time_ms": float(row["total_time_ms"]),
                "delta_vs_baseline_percent": delta,
            }

    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    trend_json = runs_dir / f"world_step_benchmark_trend_{stamp}.json"
    with trend_json.open("w", encoding="utf-8") as f:
        json.dump(trend_payload, f, indent=2)
        f.write("\n")

    print(f"[UPE] Updated trend history: {history_csv}")
    print(f"[UPE] Wrote trend deltas: {trend_json}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run UPE world-step benchmarks and validate regressions")
    parser.add_argument("--build-dir", default="build", help="CMake build directory")
    parser.add_argument("--results-json", default=None, help="Output JSON path")
    parser.add_argument("--results-csv", default=None, help="Output CSV copy path")
    parser.add_argument("--report-html", default=None, help="Output HTML report path")
    parser.add_argument(
        "--baseline",
        default="proof/EXPECTED_OUTPUTS/world_step_benchmark_baseline.json",
        help="Optional baseline JSON path",
    )
    parser.add_argument("--threshold", type=float, default=20.0, help="Regression threshold percent")
    parser.add_argument("--skip-build", action="store_true", help="Skip benchmark target build")
    parser.add_argument("--compare-baseline", action="store_true", help="Compare results against baseline")
    parser.add_argument("--update-baseline", action="store_true", help="Overwrite baseline with current results if the gate passes")
    parser.add_argument("--no-trend-history", action="store_true", help="Disable trend artifact generation")
    parser.add_argument("--steps", type=int, default=600, help="Measured simulation steps")
    parser.add_argument("--warmup", type=int, default=120, help="Warmup steps")
    parser.add_argument("--threads", type=int, default=0, help="Requested thread count")
    parser.add_argument("--no-sleep", action="store_true", help="Disable sleeping in benchmark scenarios")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    build_dir = (repo_root / args.build_dir).resolve()
    baseline_path = (repo_root / args.baseline).resolve()
    runs_dir = repo_root / "proof" / "RUNS"
    runs_dir.mkdir(parents=True, exist_ok=True)

    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    results_json = (repo_root / args.results_json).resolve() if args.results_json else (runs_dir / f"world_step_benchmarks_{stamp}.json").resolve()
    results_csv = (repo_root / args.results_csv).resolve() if args.results_csv else (runs_dir / f"world_step_benchmarks_{stamp}.csv").resolve()
    report_html = (repo_root / args.report_html).resolve() if args.report_html else (runs_dir / f"world_step_benchmark_report_{stamp}.html").resolve()

    if not args.skip_build:
        code = run_command(
            [
                "cmake",
                "--build",
                str(build_dir),
                "--target",
                "upe_bench_world_step_v2",
                "-j",
            ],
            repo_root,
        )
        if code != 0:
            return code

    benchmark_binary = resolve_benchmark_binary(build_dir)
    if not benchmark_binary.exists():
        print(f"[UPE] Missing world-step benchmark binary: {benchmark_binary}")
        return 1

    benchmark_cmd = [
        str(benchmark_binary),
        "--steps",
        str(max(1, args.steps)),
        "--warmup",
        str(max(0, args.warmup)),
        "--threads",
        str(max(0, args.threads)),
    ]
    if args.no_sleep:
        benchmark_cmd.append("--no-sleep")

    print("[UPE]", " ".join(benchmark_cmd))
    completed = subprocess.run(benchmark_cmd, cwd=str(build_dir), capture_output=True, text=True)
    output = (completed.stdout or "") + (completed.stderr or "")
    if completed.returncode != 0:
        print(output)
        return completed.returncode

    rows = parse_world_step_output(output)
    if not rows:
        print("[UPE] Failed to parse world-step benchmark CSV output")
        print(output)
        return 1

    results_csv.parent.mkdir(parents=True, exist_ok=True)
    write_plot_csv(rows, results_csv)
    write_results_json(rows, results_json)

    plot_code = run_command(
        [
            sys.executable,
            str(repo_root / "scripts" / "plot_benchmarks.py"),
            str(results_csv),
        ],
        repo_root,
    )
    if plot_code != 0:
        return plot_code

    generated_report = Path(str(results_csv).replace('.csv', '_report.html'))
    if generated_report.exists():
        report_html.parent.mkdir(parents=True, exist_ok=True)
        if generated_report.resolve() != report_html.resolve():
            shutil.move(str(generated_report), str(report_html))

    gate_code = 0
    if args.compare_baseline:
        gate_code = compare_against_baseline(results_json, baseline_path, args.threshold)

    if args.update_baseline:
        if gate_code != 0:
            print("[UPE] Skipping baseline update because world-step benchmark gate failed")
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

    print(f"[UPE] World-step benchmark CSV: {results_csv}")
    print(f"[UPE] World-step benchmark JSON: {results_json}")
    print(f"[UPE] World-step benchmark report: {report_html}")
    return gate_code


if __name__ == "__main__":
    raise SystemExit(main())
