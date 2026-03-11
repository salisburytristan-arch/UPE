# UPE Performance Gates
# Reference hardware: Intel i7-12700K, 32GB RAM, RTX 3080

from __future__ import annotations

import argparse
import json
from pathlib import Path


performance_targets = {
    # Phase 1: Rigid body dynamics
    "rigid_body_10k": {
        "description": "10,000 rigid body spheres in container",
        "target_fps": 60,
        "target_frame_time_ms": 16.67,
        "deterministic": True,
    },
    "rigid_body_stacks": {
        "description": "100 stacks of 10 boxes each (1000 bodies)",
        "target_fps": 120,
        "target_frame_time_ms": 8.33,
        "deterministic": True,
    },

    # Phase 2: Soft body
    "cloth_10k_verts": {
        "description": "Cloth with 10,000 vertices",
        "target_fps": 60,
        "target_frame_time_ms": 16.67,
        "deterministic": True,
    },

    # Phase 3: Fluids
    "sph_fluid_50k": {
        "description": "SPH fluid with 50,000 particles",
        "target_fps": 30,
        "target_frame_time_ms": 33.33,
        "deterministic": True,
    },

    # Collision detection
    "broadphase_100k": {
        "description": "Broadphase sweep-and-prune 100k AABBs",
        "target_time_us": 500,
        "deterministic": True,
    },
    "narrowphase_1k_contacts": {
        "description": "1000 contact manifolds per frame",
        "target_time_us": 1000,
        "deterministic": True,
    },

    # Solver
    "pgs_solver_5k_constraints": {
        "description": "PGS solver 5000 constraints, 10 iterations",
        "target_time_us": 2000,
        "deterministic": True,
    },

    # Determinism
    "replay_accuracy": {
        "description": "10-second replay hash match rate",
        "target_match_rate": 1.0,
        "deterministic": True,
    },
    "network_rollback_10_frames": {
        "description": "Rollback and resimulate 10 frames",
        "target_time_ms": 5.0,
        "deterministic": True,
    },

    # Catalog export
    "science_export_compact_full": {
        "description": "Full catalog compact JSON export",
        "target_time_us": 2000,
        "regression_threshold_percent": 25.0,
        "deterministic": True,
    },
    "science_export_filtered_pretty": {
        "description": "Filtered pretty JSON export",
        "target_time_us": 2500,
        "regression_threshold_percent": 20.0,
        "deterministic": True,
    },
}


# Performance gate CI configuration
ci_config = {
    "run_on_commit": True,
    "fail_on_regression": True,
    "regression_threshold_percent": 10.0,
    "platforms": ["windows_x64", "linux_x64", "macos_arm64"],
    "output_format": "json",
    "output_path": "perf_results.json",
}


def _load_results(path: Path) -> dict[str, dict[str, float]]:
    with path.open("r", encoding="utf-8") as f:
        data = json.load(f)

    results: dict[str, dict[str, float]] = {}
    if isinstance(data, dict):
        if "benchmarks" in data and isinstance(data["benchmarks"], list):
            for entry in data["benchmarks"]:
                if isinstance(entry, dict) and isinstance(entry.get("name"), str):
                    results[entry["name"]] = {k: v for k, v in entry.items() if isinstance(v, (int, float))}
        elif all(isinstance(v, dict) for v in data.values()):
            for key, value in data.items():
                numeric = {k: v for k, v in value.items() if isinstance(v, (int, float))}
                results[str(key)] = numeric
    return results


def _resolve_metric(entry: dict[str, float]) -> tuple[str, float] | tuple[None, None]:
    if "avg_us" in entry:
        return "target_time_us", float(entry["avg_us"])
    if "time_us" in entry:
        return "target_time_us", float(entry["time_us"])
    if "frame_time_ms" in entry:
        return "target_frame_time_ms", float(entry["frame_time_ms"])
    if "time_ms" in entry:
        return "target_time_ms", float(entry["time_ms"])
    if "fps" in entry:
        return "target_fps", float(entry["fps"])
    if "match_rate" in entry:
        return "target_match_rate", float(entry["match_rate"])
    return None, None


def _is_within_target(target_key: str, actual: float, target_value: float) -> bool:
    if target_key in ("target_time_us", "target_time_ms", "target_frame_time_ms"):
        return actual <= target_value
    if target_key in ("target_fps", "target_match_rate"):
        return actual >= target_value
    return True


def _regressed(target_key: str,
               actual: float,
               baseline: float,
               threshold_percent: float) -> bool:
    if baseline <= 0.0:
        return False
    ratio = threshold_percent / 100.0
    if target_key in ("target_time_us", "target_time_ms", "target_frame_time_ms"):
        return actual > baseline * (1.0 + ratio)
    if target_key in ("target_fps", "target_match_rate"):
        return actual < baseline * (1.0 - ratio)
    return False


def evaluate_results(results_path: Path,
                     baseline_path: Path | None = None,
                     threshold_percent: float | None = None) -> int:
    global_threshold = ci_config["regression_threshold_percent"] if threshold_percent is None else threshold_percent
    current = _load_results(results_path)
    baseline = _load_results(baseline_path) if baseline_path else {}

    failures: list[str] = []
    for scenario, target in performance_targets.items():
        if scenario not in current:
            continue

        target_key, actual_value = _resolve_metric(current[scenario])
        if target_key is None or target_key not in target:
            continue

        target_value = float(target[target_key])
        if not _is_within_target(target_key, actual_value, target_value):
            failures.append(
                f"{scenario}: target failed ({target_key}={actual_value:.3f}, target={target_value:.3f})"
            )

        if scenario in baseline:
            _, baseline_value = _resolve_metric(baseline[scenario])
            scenario_threshold = float(target.get("regression_threshold_percent", global_threshold))
            if baseline_value is not None and _regressed(target_key, actual_value, baseline_value, scenario_threshold):
                failures.append(
                    f"{scenario}: regression exceeded {scenario_threshold:.1f}% (current={actual_value:.3f}, baseline={baseline_value:.3f})"
                )

    if failures:
        print("PERF GATE FAILED")
        for failure in failures:
            print(f"- {failure}")
        return 1

    print("PERF GATE PASSED")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate benchmark results against UPE performance targets")
    parser.add_argument("--results", required=True, help="Path to benchmark results JSON")
    parser.add_argument("--baseline", default=None, help="Optional baseline JSON for regression checking")
    parser.add_argument("--threshold", type=float, default=None, help="Optional regression threshold percent override")
    args = parser.parse_args()

    return evaluate_results(
        Path(args.results),
        Path(args.baseline) if args.baseline else None,
        args.threshold,
    )


if __name__ == "__main__":
    raise SystemExit(main())
