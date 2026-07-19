#!/usr/bin/env python3
"""Validate UPE roadmap audit bundle ZIP structure and path formatting."""

from __future__ import annotations

import argparse
import zipfile
from pathlib import Path

REQUIRED_ENTRIES = [
    "START_HERE.md",
    "README.md",
    "VERSION",
    "CHANGELOG.md",
    "LICENSE.md",
    "COMMERCIAL_LICENSE.md",
    "CMakeLists.txt",
    "core/include/",
    "core/src/",
    "examples/",
    "tests/",
    "benchmarks/",
    "tools/",
    "scripts/",
    "docs/",
    "proof/",
    "audit_reports/AUDIT_FILE_TREE.md",
    "audit_reports/SOURCE_FILE_SUMMARY.md",
    "audit_reports/BUILD_STATUS.md",
    "audit_reports/TEST_STATUS.md",
    "audit_reports/BENCHMARK_STATUS.md",
    "audit_reports/DEPENDENCY_AUDIT.md",
    "audit_reports/DOCS_STATUS.md",
    "audit_reports/KNOWN_GAPS.md",
    "YOUR_NOTES/WHAT_I_WANT_UPE_TO_BECOME.md",
    "YOUR_NOTES/KNOWN_PROBLEMS.md",
    "YOUR_NOTES/FEATURES_I_WANT_ADDED.md",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate roadmap audit bundle zip")
    parser.add_argument("zip_path", help="Path to bundle zip")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    zip_path = Path(args.zip_path)
    if not zip_path.exists():
        print(f"ERROR: zip not found: {zip_path}")
        return 2

    with zipfile.ZipFile(zip_path, "r") as zf:
        names = zf.namelist()

    if not names:
        print("ERROR: zip is empty")
        return 3

    root_prefix = names[0].split("/", 1)[0]
    errors: list[str] = []

    for name in names:
        if "\\" in name:
            errors.append(f"backslash path entry found: {name}")

    for req in REQUIRED_ENTRIES:
        prefixed = f"{root_prefix}/{req}"
        if req.endswith("/"):
            if not any(n.startswith(prefixed) for n in names):
                errors.append(f"missing required directory: {req}")
        else:
            if prefixed not in names:
                errors.append(f"missing required file: {req}")

    if errors:
        print("VALIDATION FAILED:")
        for err in errors:
            print(f"  - {err}")
        return 1

    print("VALIDATION PASSED")
    print(f"ZIP: {zip_path}")
    print(f"Root folder: {root_prefix}")
    print(f"Entries: {len(names)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
