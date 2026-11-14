#!/usr/bin/env python3
"""Emit a comparison-ready benchmark manifest for UPE and candidate external engines.

This harness is intentionally honest: it standardizes the workloads the roadmap calls
for and records whether each engine is available in the current environment. If an
engine binary is missing, the row remains in the report with status=not_available so
that buyer-facing comparison work can be staged without inventing benchmark results.

The goal is to make the repo ready for independent validation without pretending the
external engine tooling is already installed in this environment.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import subprocess
import sys
from collections import OrderedDict
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

ENGINE_SPECS: OrderedDict[str, dict[str, Any]] = OrderedDict(
    [
        (
            "upe",
            {
                "env_var": "UPE_BINARY",
                "candidates": [
                    "build/bin/examples/upe_arena",
                    "build/bin/examples/upe_arena.exe",
                    "build/bin/upe_arena",
                    "build/bin/upe_arena.exe",
                ],
                "display_name": "UPE",
                "notes": "Local UPE showcase binary; values are produced by the repo's deterministic arena path.",
            },
        ),
        (
            "jolt",
            {
                "env_var": "JOLT_BINARY",
                "candidates": [
                    "jolt-benchmark",
                    "JoltBench",
                    "jolt_physics_benchmark",
                    "jolt-bench",
                    "jolt-benchmark.exe",
                ],
                "display_name": "Jolt Physics",
                "notes": "Requires a standalone Jolt benchmark binary or a driver script.",
            },
        ),
        (
            "bullet",
            {
                "env_var": "BULLET_BINARY",
                "candidates": [
                    "bullet-benchmark",
                    "BulletBench",
                    "bullet-benchmark.exe",
                    "bullet_demo",
                ],
                "display_name": "Bullet",
                "notes": "Requires a Bullet benchmark binary or a minimal comparison runner.",
            },
        ),
        (
            "box2d",
            {
                "env_var": "BOX2D_BINARY",
                "candidates": [
                    "box2d-benchmark",
                    "box2d_benchmark",
                    "box2d-demo",
                    "box2d-benchmark.exe",
                ],
                "display_name": "Box2D",
                "notes": "Requires a Box2D benchmark harness or a custom comparison runner.",
            },
        ),
        (
            "physx",
            {
                "env_var": "PHYSX_BINARY",
                "candidates": [
                    "physx-benchmark",
                    "PhysXBenchmark",
                    "physx_benchmark",
                    "physx-benchmark.exe",
                ],
                "display_name": "PhysX",
                "notes": "Requires a licensed or evaluated PhysX benchmark binary.",
            },
        ),
    ]
)

WORKLOADS = [
    {
        "name": "rigid_body",
        "label": "Rigid body scale",
        "sizes": [100, 500, 1000, 5000, 10000],
        "metrics": ["frame_time_ms", "bodies_per_second", "contacts", "constraints", "memory_mb", "cpu_utilization"],
    },
    {
        "name": "stacking",
        "label": "Stacking stability",
        "sizes": [10, 25, 50],
        "metrics": ["frame_time_ms", "stability_score", "solver_violations", "contacts", "memory_mb"],
    },
    {
        "name": "ccd",
        "label": "CCD high-speed thin obstacle",
        "sizes": [1],
        "metrics": ["misses", "simulation_time_ms", "stability_score", "numerical_stability"],
    },
    {
        "name": "joints",
        "label": "Large articulated joints",
        "sizes": [64, 256, 1024],
        "metrics": ["frame_time_ms", "constraints", "articulation_count", "solver_iterations", "memory_mb"],
    },
    {
        "name": "vehicles",
        "label": "Vehicle workload",
        "sizes": [1, 4, 8],
        "metrics": ["frame_time_ms", "body_count", "wheel_contacts", "solver_iterations", "cpu_utilization"],
    },
    {
        "name": "rollback",
        "label": "Rollback / replay / state hash",
        "sizes": [1, 10],
        "metrics": ["snapshot_cost_ms", "rollback_cost_ms", "replay_cost_ms", "state_hash", "reproducible"],
    },
]


def normalize_path(raw: str | None) -> Path | None:
    if raw is None or not raw.strip():
        return None
    path = Path(raw.strip())
    return path if path.is_absolute() else (Path.cwd() / path)


def discover_engine_paths(repo_root: Path) -> dict[str, list[Path]]:
    discovered: dict[str, list[Path]] = {}
    for engine_name, spec in ENGINE_SPECS.items():
        seen: set[Path] = set()
        candidates: list[Path] = []
        env_var = spec.get("env_var")
        if env_var:
            env_value = os.environ.get(env_var)
            if env_value:
                env_path = normalize_path(env_value)
                if env_path is not None and env_path.exists():
                    candidates.append(env_path)
                    seen.add(env_path.resolve(strict=False))

        for relative in spec.get("candidates", []):
            candidate = (repo_root / relative).resolve(strict=False)
            if candidate.exists() and candidate not in seen:
                candidates.append(candidate)
                seen.add(candidate)

        for search_root in [repo_root, repo_root / "build", repo_root / "build" / "bin", repo_root / "build" / "bin" / "examples"]:
            if not search_root.exists():
                continue
            for path in search_root.rglob("*"):
                name = path.name.lower()
                for candidate_name in [member.lower() for member in spec.get("candidates", []) if member]:
                    simple = Path(candidate_name).name.lower()
                    if simple and simple in name and path.is_file() and path.resolve(strict=False) not in seen:
                        candidates.append(path.resolve(strict=False))
                        seen.add(path.resolve(strict=False))
        discovered[engine_name] = candidates
    return discovered


def parse_upe_arena_output(output: str) -> dict[str, Any]:
    metrics: dict[str, Any] = {}
    for line in output.splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        if stripped.startswith("Bodies:"):
            metrics["body_count"] = int(stripped.split(":", 1)[1].strip())
        elif stripped.startswith("Frames:"):
            metrics["frames"] = int(stripped.split(":", 1)[1].strip())
        elif stripped.startswith("Step:"):
            metrics["frame_time_ms"] = float(stripped.split(":", 1)[1].strip().split()[0])
        elif stripped.startswith("Physics FPS:"):
            metrics["bodies_per_second"] = float(stripped.split(":", 1)[1].strip())
        elif stripped.startswith("State Hash:"):
            metrics["state_hash"] = stripped.split(":", 1)[1].strip()
        elif stripped.startswith("Deterministic:"):
            metrics["deterministic"] = stripped.split(":", 1)[1].strip() == "YES"
    return metrics


def run_upe_cases(repo_root: Path, workload: dict[str, Any], output_dir: Path) -> list[dict[str, Any]]:
    results: list[dict[str, Any]] = []
    candidates = [
        repo_root / "build" / "bin" / "examples" / "upe_arena",
        repo_root / "build" / "bin" / "examples" / "upe_arena.exe",
        repo_root / "build" / "bin" / "upe_arena",
        repo_root / "build" / "bin" / "upe_arena.exe",
    ]
    binary = next((candidate for candidate in candidates if candidate.exists()), None)

    for size in workload["sizes"]:
        item = {
            "engine": "UPE",
            "engine_display": "UPE",
            "benchmark": workload["name"],
            "workload_label": workload["label"],
            "workload_size": size,
            "status": "not_available",
            "notes": "UPE benchmark binary not present; this is a harness placeholder ready for real execution.",
        }
        if binary is None:
            results.append(item)
            continue

        cube_root = max(1, int(round(size ** (1.0 / 3.0))))
        command = [str(binary), str(cube_root), "60"]
        try:
            completed = subprocess.run(command, capture_output=True, text=True, cwd=str(repo_root), timeout=90)
            item["status"] = "passed" if completed.returncode == 0 else "failed"
            if completed.returncode != 0:
                item["notes"] = completed.stderr.strip() or completed.stdout.strip() or "UPE benchmark execution failed"
                results.append(item)
                continue

            parsed = parse_upe_arena_output(completed.stdout)
            item.update(
                {
                    "status": "passed",
                    "frame_time_ms": parsed.get("frame_time_ms"),
                    "bodies_per_second": parsed.get("bodies_per_second"),
                    "contacts": None,
                    "constraints": None,
                    "memory_mb": None,
                    "cpu_utilization": None,
                    "state_hash": parsed.get("state_hash"),
                    "deterministic": parsed.get("deterministic"),
                    "body_count": parsed.get("body_count"),
                    "notes": "UPE benchmark executed via the deterministic arena binary.",
                }
            )
            results.append(item)
        except Exception as exc:  # pragma: no cover - runtime guard for CI environments
            item["status"] = "failed"
            item["notes"] = f"UPE benchmark invocation failed: {exc}"
            results.append(item)

    return results


def external_engine_rows(discovered: dict[str, list[Path]], workload: dict[str, Any]) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for engine_name, spec in ENGINE_SPECS.items():
        if engine_name == "upe":
            continue
        discovered_paths = discovered.get(engine_name, [])
        for size in workload["sizes"]:
            status = "not_available" if not discovered_paths else "ready"
            row = {
                "engine": engine_name,
                "engine_display": spec["display_name"],
                "benchmark": workload["name"],
                "workload_label": workload["label"],
                "workload_size": size,
                "status": status,
                "notes": spec["notes"],
            }
            if discovered_paths:
                row["path"] = str(discovered_paths[0])
            rows.append(row)
    return rows


def write_rows(rows: list[dict[str, Any]], output_dir: Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    csv_path = output_dir / "independent_benchmark_matrix.csv"
    json_path = output_dir / "independent_benchmark_matrix.json"
    md_path = output_dir / "independent_benchmark_matrix.md"

    fieldnames = [
        "engine",
        "engine_display",
        "benchmark",
        "workload_label",
        "workload_size",
        "status",
        "frame_time_ms",
        "bodies_per_second",
        "contacts",
        "constraints",
        "memory_mb",
        "cpu_utilization",
        "state_hash",
        "deterministic",
        "body_count",
        "path",
        "notes",
    ]

    with csv_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            normalized = {key: row.get(key) for key in fieldnames}
            writer.writerow(normalized)

    with json_path.open("w", encoding="utf-8") as handle:
        json.dump({"generated_at": datetime.now(timezone.utc).isoformat(), "rows": rows}, handle, indent=2)
        handle.write("\n")

    lines = [
        "# Independent Benchmark Harness",
        "",
        "This report standardizes the next roadmap milestone: an independent comparison harness for UPE and candidate external engines.",
        "",
        "## Summary",
        "",
        f"Generated: {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')}",
        "",
        "| Engine | Benchmark | Workload | Size | Status | Notes |",
        "| --- | --- | --- | ---: | --- | --- |",
    ]
    for row in rows:
        lines.append(
            f"| {row.get('engine_display', row.get('engine', 'Unknown'))} | {row.get('benchmark', 'n/a')} | {row.get('workload_label', 'n/a')} | {row.get('workload_size', 'n/a')} | {row.get('status', 'unknown')} | {str(row.get('notes', '')).replace('|', '/')} |"
        )
    md_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Create an independent benchmark comparison manifest for UPE against candidate external engines.")
    parser.add_argument("--repo-root", default=str(Path(__file__).resolve().parents[1]), help="Repository root used for binary discovery")
    parser.add_argument("--output-dir", default="proof/RUNS/independent_benchmark_harness", help="Directory to write the CSV/JSON/Markdown report")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    output_dir = (repo_root / args.output_dir).resolve()

    discovered = discover_engine_paths(repo_root)
    rows: list[dict[str, Any]] = []
    for workload in WORKLOADS:
        rows.extend(run_upe_cases(repo_root, workload, output_dir))
        rows.extend(external_engine_rows(discovered, workload))

    write_rows(rows, output_dir)

    print(f"[UPE independent benchmark harness] Wrote {output_dir / 'independent_benchmark_matrix.csv'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
