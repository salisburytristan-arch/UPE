#!/usr/bin/env python3
"""
Release package gate for Unified Physics Engine.

Validates the buyout release zip file contains required files/directories before publishing.

Usage:
    python scripts/verify_release_gate.py --dir releases_sanity
    python scripts/verify_release_gate.py --create --dir releases_ci
"""

from __future__ import annotations

import argparse
import sys
import subprocess
import zipfile
import re
import posixpath
from pathlib import Path
from typing import Dict, List


TIERS = ("buyout",)

REQUIRED_FILES = {
    "buyout": [
        "README.md",
        "LICENSE",
        "CONTRIBUTING.md",
        "DISTRIBUTION_POLICY.md",
        "IP_CHAIN_OF_TITLE_CHECKLIST.md",
        "COMMERCIAL_LICENSE_AGREEMENT.md",
        "docs/BUYER_START_HERE.md",
        "docs/INDEX.md",
        "docs/BUILD_GUIDE.md",
        "docs/TEST_GUIDE.md",
        "docs/DETERMINISM_GUIDE.md",
        "docs/INTEGRATION_GUIDE.md",
        "docs/API_REFERENCE.md",
        "docs/BINDINGS_STATUS.md",
        "docs/TROUBLESHOOTING.md",
        "docs/KNOWN_LIMITATIONS.md",
        "docs/SECURITY_AND_SAFETY.md",
        "docs/CHANGELOG.md",
        "docs/RELEASE_NOTES_v1.0.0.md",
        "docs/ACCEPTANCE_CHECKLIST.md",
        "docs/TOPIC_TRUTH_AUDIT.md",
        "tools/build_windows_msvc.bat",
        "tools/build_linux.sh",
        "tools/run_tests.bat",
        "tools/run_tests.sh",
        "tools/run_determinism_proof.bat",
        "tools/run_determinism_proof.sh",
        "tools/run_examples_smoketest.bat",
        "tools/run_examples_smoketest.sh",
        "tools/diagnose_environment.bat",
        "tools/diagnose_environment.sh",
        "tools/hash_package.bat",
        "tools/hash_package.sh",
        "tools/hash_package.py",
        "proof/CLOSING_HASH_RECORD.txt",
        "proof/EXPECTED_OUTPUTS/build_success_windows.txt",
        "proof/EXPECTED_OUTPUTS/build_success_linux.txt",
        "proof/EXPECTED_OUTPUTS/tests_pass.txt",
        "proof/EXPECTED_OUTPUTS/determinism_hash_expected.txt",
        "proof/EXPECTED_OUTPUTS/examples_smoketest_expected.txt",
        "proof/REFERENCE_RUNS/windows_reference_run_YYYYMMDD.log",
        "proof/REFERENCE_RUNS/linux_reference_run_YYYYMMDD.log",
        "proof/REFERENCE_RUNS/hashes_reference_run_YYYYMMDD.json",
        "QUICK_START.md",
        "API_REFERENCE.md",
        "cmake/UpeConfig.cmake.in",
        "build_msvc.bat",
        "CMakeLists.txt",
        "BUILD_STATUS.md",
        "topic_truth_audit.csv",
    ],
}

REQUIRED_DIR_PREFIXES = {
    "buyout": [
        "core/",
        "examples/",
        "bindings/",
        "tests/",
        "benchmarks/",
        "scripts/",
        "docs/",
        "tools/",
        "proof/",
    ],
}

LIBRARY_EXTENSIONS = {".lib", ".a", ".dll", ".so", ".dylib"}
FORBIDDEN_DIR_PREFIXES = (
    "build/",
    "build_warn/",
    "build_linux/",
    "build_windows/",
)

FORBIDDEN_REFERENCE_STRINGS = [
    "build_demo.bat",
    "QUICK_START_3.0.md",
    "MIT License",
    "license-MIT",
    "open-source license",
]

TEXT_SCAN_EXTENSIONS = {".md", ".txt", ".rst"}
MARKDOWN_EXTENSIONS = {".md"}
MARKDOWN_LINK_RE = re.compile(r"\[[^\]]+\]\(([^)]+)\)")


def required_entry_present(required: str, entries: set[str]) -> bool:
    if required in entries:
        return True
    if "YYYYMMDD" not in required:
        return False

    pattern = re.escape(required).replace("YYYYMMDD", r"\d{8}")
    regex = re.compile(rf"^{pattern}$")
    return any(regex.match(entry) for entry in entries)


def run_create_release(output_dir: Path) -> int:
    script_path = Path(__file__).resolve().parent / "create_release.py"
    command = [
        sys.executable,
        str(script_path),
        "--tier",
        "buyout",
        "--output",
        str(output_dir),
        "--confirm-private-distribution",
    ]
    print("Running package creation:", " ".join(command))
    result = subprocess.run(command)
    return result.returncode


def find_tier_zip(output_dir: Path, tier: str) -> Path | None:
    pattern = f"UnifiedPhysicsEngine-*-{tier}-*.zip"
    candidates = sorted(output_dir.glob(pattern), key=lambda p: p.stat().st_mtime, reverse=True)
    return candidates[0] if candidates else None


def normalize_zip_entries(zf: zipfile.ZipFile) -> tuple[str, set[str]]:
    names = {name for name in zf.namelist() if name and not name.endswith("/")}
    if not names:
        return "", set()

    top_dir = min(name.split("/", 1)[0] for name in names if "/" in name)
    normalized = set()
    for name in names:
        prefix = f"{top_dir}/"
        if name.startswith(prefix):
            normalized.add(name[len(prefix):])
        else:
            normalized.add(name)
    return top_dir, normalized


def verify_tier_zip(zip_path: Path, tier: str) -> List[str]:
    errors: List[str] = []

    with zipfile.ZipFile(zip_path, "r") as zf:
        top_dir, entries = normalize_zip_entries(zf)
        if not top_dir:
            errors.append("zip is empty")
            return errors

        missing_files = [f for f in REQUIRED_FILES[tier] if not required_entry_present(f, entries)]
        if missing_files:
            errors.append(f"missing files: {', '.join(missing_files)}")

        forbidden_dirs = [prefix for prefix in FORBIDDEN_DIR_PREFIXES if any(name.startswith(prefix) for name in entries)]
        if forbidden_dirs:
            errors.append(f"forbidden build directories present: {', '.join(forbidden_dirs)}")

        for prefix in REQUIRED_DIR_PREFIXES[tier]:
            if not any(name.startswith(prefix) for name in entries):
                errors.append(f"missing directory content under: {prefix}")

        has_binary = any(Path(name).suffix.lower() in LIBRARY_EXTENSIONS for name in entries)
        if not has_binary:
            print("  ℹ buyout: no prebuilt library binary in zip (allowed; package builds from source)")

        for entry in sorted(entries):
            if Path(entry).suffix.lower() not in TEXT_SCAN_EXTENSIONS:
                continue

            entry_name = f"{top_dir}/{entry}"
            try:
                content = zf.read(entry_name).decode("utf-8", errors="ignore")
            except KeyError:
                continue

            for forbidden in FORBIDDEN_REFERENCE_STRINGS:
                if forbidden in content:
                    errors.append(
                        f"broken reference in {entry}: contains '{forbidden}'"
                    )

            if Path(entry).suffix.lower() in MARKDOWN_EXTENSIONS:
                errors.extend(validate_markdown_links(entry, content, entries))

    return errors


def validate_markdown_links(entry: str, content: str, entries: set[str]) -> List[str]:
    errors: List[str] = []
    base_dir = Path(entry).parent

    for match in MARKDOWN_LINK_RE.finditer(content):
        raw_link = match.group(1).strip()
        if not raw_link:
            continue

        if raw_link.startswith(("http://", "https://", "mailto:", "#")):
            continue

        link_without_anchor = raw_link.split("#", 1)[0].strip()
        if not link_without_anchor:
            continue

        link_path = Path(link_without_anchor)
        if link_path.is_absolute():
            continue

        resolved = (base_dir / link_path).as_posix()
        normalized = posixpath.normpath(resolved)

        if normalized not in entries:
            errors.append(f"broken relative link in {entry}: {raw_link}")

    return errors


def verify_all(output_dir: Path) -> int:
    topic_audit_check = subprocess.run(
        [sys.executable, str(Path(__file__).resolve().parent / "generate_topic_truth_audit.py"), "--check"]
    )
    if topic_audit_check.returncode != 0:
        print("\nRELEASE GATE FAILED")
        print("  [repo]")
        print("    - topic truth audit artifacts are stale")
        return topic_audit_check.returncode

    failures: Dict[str, List[str]] = {}

    print(f"Verifying release packages in: {output_dir}")

    for tier in TIERS:
        zip_path = find_tier_zip(output_dir, tier)
        if not zip_path:
            failures[tier] = ["zip file not found"]
            continue

        print(f"- Checking {tier}: {zip_path.name}")
        tier_errors = verify_tier_zip(zip_path, tier)
        if tier_errors:
            failures[tier] = tier_errors

    if failures:
        print("\nRELEASE GATE FAILED")
        for tier, tier_errors in failures.items():
            print(f"  [{tier}]")
            for error in tier_errors:
                print(f"    - {error}")
        return 1

    print("\nRELEASE GATE PASSED")
    print("Buyout package includes required publish-time files and content.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify release zip packages before publishing")
    parser.add_argument("--dir", default="releases", help="Directory containing release zip files")
    parser.add_argument(
        "--create",
        action="store_true",
        help="Create all release zips first using scripts/create_release.py",
    )

    args = parser.parse_args()
    output_dir = Path(args.dir)

    if args.create:
        output_dir.mkdir(parents=True, exist_ok=True)
        code = run_create_release(output_dir)
        if code != 0:
            print("Release package creation failed; skipping verification.")
            return code

    if not output_dir.exists():
        print(f"Directory not found: {output_dir}")
        return 1

    return verify_all(output_dir)


if __name__ == "__main__":
    sys.exit(main())
