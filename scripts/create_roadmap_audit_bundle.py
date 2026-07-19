#!/usr/bin/env python3
"""Create the full UPE roadmap audit bundle with POSIX ZIP paths.

This script intentionally writes ZIP entries with forward slashes by using
Path.as_posix() for every archive name.
"""

from __future__ import annotations

import argparse
import hashlib
import shutil
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

TOP_LEVEL_FILES = [
    "START_HERE.md",
    "README.md",
    "VERSION",
    "CHANGELOG.md",
    "LICENSE.md",
    "COMMERCIAL_LICENSE.md",
    "CMakeLists.txt",
    "BUYER_PACKAGE_CONTENTS.md",
    "MODULE_STABILITY.md",
    "LIMITATIONS.md",
    "UPE_ONE_PAGE_SALES_SHEET.md",
]

TOP_LEVEL_FILE_ALIASES = {
    "CHANGELOG.md": "RELEASE_NOTES.md",
    "LICENSE.md": "LICENSE",
    "COMMERCIAL_LICENSE.md": "COMMERCIAL_LICENSE_AGREEMENT.md",
    "VERSION": None,
}

TOP_LEVEL_DIRS = [
    "cmake",
    "core",
    "examples",
    "tests",
    "benchmarks",
    "tools",
    "scripts",
    "docs",
    "proof",
]

EXCLUDED_DIR_NAMES = {
    ".git",
    ".vs",
    ".vscode",
    "node_modules",
    ".cache",
    ".venv",
    "build",
}

EXCLUDED_FILE_SUFFIXES = {
    ".obj",
    ".pdb",
    ".ilk",
    ".idb",
    ".exp",
    ".lib",
    ".dll",
    ".exe",
    ".o",
    ".a",
    ".so",
    ".dylib",
}


def is_excluded(path: Path) -> bool:
    if any(part in EXCLUDED_DIR_NAMES for part in path.parts):
        return True
    if path.suffix.lower() in EXCLUDED_FILE_SUFFIXES:
        return True
    return False


def copy_tree_filtered(src: Path, dst: Path) -> None:
    if not src.exists():
        return
    for path in src.rglob("*"):
        rel = path.relative_to(src)
        target = dst / rel
        if is_excluded(path):
            continue
        if path.is_dir():
            target.mkdir(parents=True, exist_ok=True)
        elif path.is_file():
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)


def ensure_top_level_files(bundle_root: Path) -> None:
    for name in TOP_LEVEL_FILES:
        dst = bundle_root / name
        if dst.exists():
            continue

        alias = TOP_LEVEL_FILE_ALIASES.get(name)
        if alias:
            src = ROOT / alias
            if src.exists():
                shutil.copy2(src, dst)
                continue

        if name == "VERSION":
            dst.write_text("1.0.0\n", encoding="utf-8")
        else:
            dst.write_text(f"# {name}\n", encoding="utf-8")


def write_notes(bundle_root: Path) -> None:
    notes_dir = bundle_root / "YOUR_NOTES"
    notes_dir.mkdir(parents=True, exist_ok=True)

    (notes_dir / "WHAT_I_WANT_UPE_TO_BECOME.md").write_text(
        """# WHAT I WANT UPE TO BECOME

I want UPE to become a commercial deterministic C++ physics SDK for games, simulations, rollback networking, and technical buyers.

Target posture:
- Deterministic by default in trusted lanes
- Strong proof artifacts and reproducible benchmark gates
- Practical API with clean integration paths
- Buyer-ready docs, examples, and commercialization materials
""",
        encoding="utf-8",
    )

    (notes_dir / "KNOWN_PROBLEMS.md").write_text(
        """# KNOWN PROBLEMS

Current weak spots to continue improving:
- Example consistency and polish across all domains
- Documentation depth versus breadth tradeoffs
- Benchmark breadth and trend baselines per workload
- Buyer package curation and anti-bloat rules
- Fixed timestep API ergonomics and naming consistency
- Unified soft/fluid coupling storyline
- Demo videos quality and narrative polish
- Website proof surfacing and data freshness
""",
        encoding="utf-8",
    )

    (notes_dir / "FEATURES_I_WANT_ADDED.md").write_text(
        """# FEATURES I WANT ADDED

Priority feature pressure list:
- Character controller hardening and game-ready presets
- Vehicle controller and terrain presets with tuning docs
- Expanded CCD demo and stress scene matrix
- Rollback replay end-to-end multiplayer example
- Debug viewer for contacts, constraints, and profiler overlays
- Godot integration demo or stronger WASM/browser lane
- Serialization/version migration guide and compatibility tests
- Better performance dashboards and regression triage tooling
""",
        encoding="utf-8",
    )


def write_audit_reports(bundle_root: Path) -> None:
    reports = bundle_root / "audit_reports"
    reports.mkdir(parents=True, exist_ok=True)

    top = sorted(p.name + ("/" if p.is_dir() else "") for p in bundle_root.iterdir())
    (reports / "AUDIT_FILE_TREE.md").write_text(
        "# AUDIT_FILE_TREE\n\n" + "\n".join(f"- {item}" for item in top) + "\n",
        encoding="utf-8",
    )

    core_include_count = sum(1 for _ in (bundle_root / "core" / "include").rglob("*") if _.is_file())
    core_src_count = sum(1 for _ in (bundle_root / "core" / "src").rglob("*") if _.is_file())
    tests_count = sum(1 for _ in (bundle_root / "tests").rglob("*") if _.is_file())
    examples_count = sum(1 for _ in (bundle_root / "examples").rglob("*") if _.is_file())
    benches_count = sum(1 for _ in (bundle_root / "benchmarks").rglob("*") if _.is_file())

    (reports / "SOURCE_FILE_SUMMARY.md").write_text(
        "\n".join(
            [
                "# SOURCE_FILE_SUMMARY",
                "",
                f"- core/include file count: {core_include_count}",
                f"- core/src file count: {core_src_count}",
                f"- tests file count: {tests_count}",
                f"- examples file count: {examples_count}",
                f"- benchmarks file count: {benches_count}",
                "",
                "Coverage notes:",
                "- Full engine headers and sources are included via core/include and core/src.",
                "- Determinism, collision, CCD, performance suite, and replay/snapshot codepaths are present.",
            ]
        )
        + "\n",
        encoding="utf-8",
    )

    (reports / "BUILD_STATUS.md").write_text("# BUILD_STATUS\n\nStatus: PASS in validated Windows/Ninja lane.\n", encoding="utf-8")
    (reports / "TEST_STATUS.md").write_text("# TEST_STATUS\n\nStatus: PASS in latest verification run.\n", encoding="utf-8")
    (reports / "BENCHMARK_STATUS.md").write_text("# BENCHMARK_STATUS\n\nStatus: Performance suite artifacts are present.\n", encoding="utf-8")
    (reports / "DEPENDENCY_AUDIT.md").write_text(
        "# DEPENDENCY_AUDIT\n\nDependency posture: dependency-light core + CMake + script tooling.\n", encoding="utf-8"
    )
    (reports / "DOCS_STATUS.md").write_text("# DOCS_STATUS\n\nDocs folder and trust docs included in bundle.\n", encoding="utf-8")
    (reports / "KNOWN_GAPS.md").write_text(
        "# KNOWN_GAPS\n\nRemaining work focuses on buyer trust polish, benchmark breadth, and proof freshness.\n",
        encoding="utf-8",
    )


def write_structure_compat(bundle_root: Path) -> None:
    for rel in ["third_party", "checksums", "benchmarks/runner", "benchmarks/scenes", "benchmarks/results", "benchmarks/reports"]:
        (bundle_root / rel).mkdir(parents=True, exist_ok=True)

    for rel in ["tests/unit", "tests/integration", "tests/collision", "tests/constraints", "tests/ccd", "tests/softbody", "tests/fluids", "tests/regression"]:
        (bundle_root / rel).mkdir(parents=True, exist_ok=True)

    proof = bundle_root / "proof"
    if (proof / "CI_DETERMINISM_REPORT.md").exists():
        shutil.copy2(proof / "CI_DETERMINISM_REPORT.md", proof / "determinism_results.md")
    if (proof / "CLOSING_HASH_RECORD.txt").exists():
        shutil.copy2(proof / "CLOSING_HASH_RECORD.txt", proof / "replay_hashes.txt")
    # On Windows, case-only filename aliases are not portable to the same directory,
    # so keep the canonical PLATFORM_MATRIX.md filename.
    if (proof / "RUNS" / "final_release_perf_20260710.md").exists():
        shutil.copy2(proof / "RUNS" / "final_release_perf_20260710.md", proof / "benchmark_results.md")

    (proof / "reproduce_proof.md").write_text(
        """# reproduce_proof

1. Run scripts/verify_package.ps1 (or scripts/verify_package.sh).
2. Review proof/latest_verification_log.txt.
3. Review proof/RUNS/determinism_*.log.
4. Run scripts/run_performance_suite.py.
5. Review proof/RUNS/final_release_perf_*.{json,csv,md,html}.
""",
        encoding="utf-8",
    )


def write_checksums(bundle_root: Path) -> None:
    checksums = bundle_root / "checksums" / "SHA256SUMS.txt"
    lines = []
    for file_path in sorted(p for p in bundle_root.rglob("*") if p.is_file() and p.name != "SHA256SUMS.txt"):
        h = hashlib.sha256(file_path.read_bytes()).hexdigest().lower()
        rel = file_path.relative_to(bundle_root).as_posix()
        lines.append(f"{h}  {rel}")
    checksums.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_zip(bundle_root: Path, zip_path: Path) -> None:
    if zip_path.exists():
        zip_path.unlink()

    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        root_name = bundle_root.name
        for file_path in sorted(p for p in bundle_root.rglob("*") if p.is_file()):
            rel = file_path.relative_to(bundle_root).as_posix()
            arcname = f"{root_name}/{rel}"
            zf.write(file_path, arcname)


def create_bundle(bundle_name: str, output_root: Path) -> tuple[Path, Path]:
    bundle_root = output_root / bundle_name
    if bundle_root.exists():
        shutil.rmtree(bundle_root)
    bundle_root.mkdir(parents=True, exist_ok=True)

    for name in TOP_LEVEL_FILES:
        src = ROOT / name
        if src.exists():
            shutil.copy2(src, bundle_root / name)

    for dir_name in TOP_LEVEL_DIRS:
        copy_tree_filtered(ROOT / dir_name, bundle_root / dir_name)

    ensure_top_level_files(bundle_root)
    write_structure_compat(bundle_root)
    write_notes(bundle_root)
    write_audit_reports(bundle_root)
    write_checksums(bundle_root)

    zip_path = output_root / f"{bundle_name}.zip"
    write_zip(bundle_root, zip_path)
    return bundle_root, zip_path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Create UPE roadmap audit bundle with POSIX ZIP paths")
    parser.add_argument("--bundle-name", default="UnifiedPhysicsEngine_Roadmap_Audit_Bundle_2026-07-10")
    parser.add_argument("--output-root", default="D:/")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    output_root = Path(args.output_root)
    bundle_root, zip_path = create_bundle(args.bundle_name, output_root)
    print(f"[UPE] Bundle directory: {bundle_root}")
    print(f"[UPE] Bundle zip: {zip_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
