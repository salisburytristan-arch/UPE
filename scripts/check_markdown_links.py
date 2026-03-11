#!/usr/bin/env python3
"""Fast repository markdown link integrity check."""

from __future__ import annotations

import posixpath
import re
from pathlib import Path

MARKDOWN_LINK_RE = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
SKIP_DIRS = {
    ".git",
    ".venv",
    "build",
    "build-release",
    "build-debug",
    "cmake-build",
    "cmake-build-ninja",
    "releases",
    "releases_ci",
    "releases_sanity",
}


def iter_markdown_files(root: Path):
    for path in root.rglob("*.md"):
        if any(part in SKIP_DIRS for part in path.parts):
            continue
        if path.is_file():
            yield path


def is_external_link(link: str) -> bool:
    return link.startswith(("http://", "https://", "mailto:", "#"))


def check_file(md_path: Path, root: Path):
    errors: list[str] = []
    content = md_path.read_text(encoding="utf-8", errors="ignore")
    base_dir = md_path.parent

    for match in MARKDOWN_LINK_RE.finditer(content):
        raw_link = match.group(1).strip()
        if not raw_link or is_external_link(raw_link):
            continue

        link_without_anchor = raw_link.split("#", 1)[0].strip()
        if not link_without_anchor:
            continue

        link_path = Path(link_without_anchor)
        if link_path.is_absolute():
            continue

        resolved = (base_dir / link_path).as_posix()
        normalized = posixpath.normpath(resolved)
        target = root / Path(normalized)

        if not target.exists():
            line = content.count("\n", 0, match.start()) + 1
            rel_file = md_path.relative_to(root).as_posix()
            errors.append(f"{rel_file}:{line}: broken relative link -> {raw_link}")

    return errors


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    all_errors: list[str] = []

    for md in iter_markdown_files(root):
        all_errors.extend(check_file(md, root))

    if all_errors:
        print("Markdown link check failed:")
        for error in sorted(all_errors):
            print(f"  - {error}")
        return 1

    print("Markdown link check passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
