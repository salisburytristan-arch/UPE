#!/usr/bin/env python3
"""
Create a clean, audit-ready sale bundle from a clean tagged commit.

This script enforces:
1) Clean git working tree
2) Exact tag -> HEAD match
3) Required reference-run artifacts present in tagged tree
4) Release gate creation + verification pass
5) Source archive generated from git tag (not working tree copy)
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import List


def run(cmd: List[str], cwd: Path) -> str:
    result = subprocess.run(cmd, cwd=str(cwd), capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(
            f"command failed ({' '.join(cmd)}):\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )
    return result.stdout.strip()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def require_clean_tree(repo: Path) -> None:
    status = run(["git", "status", "--porcelain"], cwd=repo)
    if status:
        raise RuntimeError(
            "working tree is not clean. Commit/stash/clean changes before sale packaging."
        )


def require_exact_tag_at_head(repo: Path, tag: str) -> tuple[str, str]:
    tag_commit = run(["git", "rev-parse", f"{tag}^{{commit}}"], cwd=repo)
    head_commit = run(["git", "rev-parse", "HEAD"], cwd=repo)
    if tag_commit != head_commit:
        raise RuntimeError(
            f"tag {tag} does not match HEAD. tag={tag_commit}, head={head_commit}"
        )
    return tag_commit, head_commit


def require_reference_runs_in_tag(repo: Path, tag: str) -> None:
    paths = run(["git", "ls-tree", "-r", "--name-only", tag, "proof/REFERENCE_RUNS"], cwd=repo)
    files = [p.strip() for p in paths.splitlines() if p.strip()]

    has_windows = any(re.fullmatch(r"proof/REFERENCE_RUNS/windows_reference_run_\d{8}\.log", p) for p in files)
    has_linux = any(re.fullmatch(r"proof/REFERENCE_RUNS/linux_reference_run_\d{8}\.log", p) for p in files)
    has_hashes = any(re.fullmatch(r"proof/REFERENCE_RUNS/hashes_reference_run_\d{8}\.json", p) for p in files)

    missing = []
    if not has_windows:
        missing.append("windows_reference_run_YYYYMMDD.log")
    if not has_linux:
        missing.append("linux_reference_run_YYYYMMDD.log")
    if not has_hashes:
        missing.append("hashes_reference_run_YYYYMMDD.json")

    if missing:
        raise RuntimeError(
            "tagged tree is missing required reference-run files: " + ", ".join(missing)
        )


def create_sale_bundle(repo: Path, tag: str, output_dir: Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d")
    sale_dir = output_dir / f"upe_sale_bundle_{stamp}_{tag.replace('/', '_')}"
    if sale_dir.exists():
        shutil.rmtree(sale_dir)
    sale_dir.mkdir(parents=True, exist_ok=True)

    archives_dir = sale_dir / "archives"
    archives_dir.mkdir(parents=True, exist_ok=True)

    # 1) Build and verify release package from clean checked-out tag state.
    run(
        [
            sys.executable,
            "scripts/verify_release_gate.py",
            "--create",
            "--dir",
            str(sale_dir / "release_gate"),
        ],
        cwd=repo,
    )

    # 2) Source zip generated directly from git tag tree for exact Git match.
    source_zip = archives_dir / f"UnifiedPhysicsEngine_source_{tag}_{stamp}.zip"
    run(
        ["git", "archive", "--format=zip", "--output", str(source_zip), tag],
        cwd=repo,
    )

    # 3) Portable git history bundle.
    history_bundle = archives_dir / f"UnifiedPhysicsEngine_git_history_{tag}_{stamp}.bundle"
    run(["git", "bundle", "create", str(history_bundle), "--all"], cwd=repo)
    run(["git", "bundle", "verify", str(history_bundle)], cwd=repo)

    # 4) Copy transfer docs if present.
    transfer_docs = repo / "transfer_bundle_2026-07-15"
    if transfer_docs.exists():
        shutil.copytree(transfer_docs, sale_dir / "transfer_docs", dirs_exist_ok=True)

    # 5) Checksums and manifest.
    artifacts = [source_zip, history_bundle]
    checksums = []
    for artifact in artifacts:
        checksums.append(f"{sha256_file(artifact)}  {artifact.name}")

    sha_path = archives_dir / "SHA256SUMS.txt"
    sha_path.write_text("\n".join(checksums) + "\n", encoding="ascii")

    head_commit = run(["git", "rev-parse", "HEAD"], cwd=repo)
    commit_count = run(["git", "rev-list", "--count", "HEAD"], cwd=repo)
    commit_time = run(["git", "show", "-s", "--format=%ci", "HEAD"], cwd=repo)

    manifest = {
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "tag": tag,
        "head_commit": head_commit,
        "commit_count": int(commit_count),
        "head_commit_time": commit_time,
        "artifacts": [
            {"name": source_zip.name, "sha256": sha256_file(source_zip)},
            {"name": history_bundle.name, "sha256": sha256_file(history_bundle)},
            {"name": sha_path.name, "sha256": sha256_file(sha_path)},
        ],
    }

    (sale_dir / "SALE_BUNDLE_MANIFEST.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )

    print(f"Created sale bundle at: {sale_dir}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Create clean sale bundle from exact tagged commit")
    parser.add_argument("--tag", required=True, help="Git tag that must resolve exactly to HEAD")
    parser.add_argument("--output", default="sale_bundles", help="Output directory")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    output_dir = (repo / args.output).resolve()

    try:
        require_clean_tree(repo)
        require_exact_tag_at_head(repo, args.tag)
        require_reference_runs_in_tag(repo, args.tag)
        create_sale_bundle(repo, args.tag, output_dir)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
