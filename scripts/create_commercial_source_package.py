#!/usr/bin/env python3
"""Create a clean commercial-source delivery ZIP for UPE.

Output:
    releases_commercial/UnifiedPhysicsEngine-1.0.0-commercial-source.zip

Contents:
    /core/                          source tree only
    /docs/BUYER_START_HERE.pdf
    /docs/COMMERCIAL_LICENSE.pdf
    /docs/RELEASE_NOTES.pdf
    /docs/PROOF_INDEX.pdf
    /checksums/sha256.txt
"""

from __future__ import annotations

import hashlib
import shutil
import textwrap
import unicodedata
import zipfile
from datetime import datetime, timezone
from pathlib import Path

VERSION = "1.0.0"
PACKAGE_STEM = f"UnifiedPhysicsEngine-{VERSION}-commercial-source"
ROOT = Path(__file__).resolve().parents[1]
OUTPUT_DIR = ROOT / "releases_commercial"
STAGE_DIR = OUTPUT_DIR / PACKAGE_STEM
ZIP_PATH = OUTPUT_DIR / f"{PACKAGE_STEM}.zip"
PACKAGE_HASH_PATH = OUTPUT_DIR / f"{PACKAGE_STEM}.sha256.txt"

PDF_SOURCES = {
    "BUYER_START_HERE.pdf": ROOT / "docs" / "BUYER_START_HERE.md",
    "COMMERCIAL_LICENSE.pdf": ROOT / "COMMERCIAL_LICENSE_AGREEMENT.md",
    "RELEASE_NOTES.pdf": ROOT / "RELEASE_NOTES.md",
    "PROOF_INDEX.pdf": ROOT / "docs" / "PROOF_INDEX.md",
}


def sanitize_text(value: str) -> str:
    replacements = {
        "—": "-",
        "–": "-",
        "’": "'",
        "‘": "'",
        "“": '"',
        "”": '"',
        "•": "*",
        "✓": "[OK]",
        "✅": "[OK]",
        "⚠": "[!]",
        "⚠️": "[!]",
        "ℹ": "[i]",
        "→": "->",
        "←": "<-",
        "→": "->",
        "…": "...",
        "©": "(c)",
    }
    for old, new in replacements.items():
        value = value.replace(old, new)
    value = unicodedata.normalize("NFKD", value)
    return value.encode("latin-1", "ignore").decode("latin-1")


def escape_pdf_text(value: str) -> str:
    return value.replace("\\", r"\\").replace("(", r"\(").replace(")", r"\)")


def markdown_to_lines(title: str, text: str) -> list[str]:
    lines: list[str] = [title, "", f"Generated from UPE source docs on {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')}", "", "-" * 88, ""]
    in_code_block = False

    for raw_line in sanitize_text(text).splitlines():
        stripped = raw_line.rstrip()

        if stripped.startswith("```"):
            in_code_block = not in_code_block
            lines.append("-" * 88)
            continue

        if not stripped:
            lines.append("")
            continue

        if in_code_block:
            for wrapped in textwrap.wrap(stripped, width=92, break_long_words=False, drop_whitespace=False) or [""]:
                lines.append(f"    {wrapped}")
            continue

        if stripped.startswith("#"):
            heading = stripped.lstrip("#").strip().upper()
            lines.extend(["", heading, ""])
            continue

        prefix = ""
        content = stripped
        if stripped.startswith(("- ", "* ")):
            prefix = "* "
            content = stripped[2:].strip()
        elif len(stripped) > 2 and stripped[0].isdigit() and stripped[1] in ".)":
            prefix = stripped[:2] + " "
            content = stripped[2:].strip()

        wrapped_lines = textwrap.wrap(
            content,
            width=92,
            initial_indent=prefix,
            subsequent_indent=(" " * len(prefix)) if prefix else "",
            break_long_words=False,
            break_on_hyphens=False,
        ) or [prefix.rstrip()]
        lines.extend(wrapped_lines)

    return lines


def paginate(lines: list[str], lines_per_page: int = 50) -> list[list[str]]:
    pages: list[list[str]] = []
    current: list[str] = []
    for line in lines:
        current.append(line)
        if len(current) >= lines_per_page:
            pages.append(current)
            current = []
    if current or not pages:
        pages.append(current)
    return pages


def write_simple_text_pdf(path: Path, title: str, text: str) -> None:
    pages = paginate(markdown_to_lines(title, text))

    objects: list[bytes] = [b"", b"", b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>"]
    font_id = 3
    page_ids: list[int] = []

    for page_lines in pages:
        content_lines = ["BT", "/F1 10 Tf", "14 TL", "50 760 Td"]
        first = True
        for line in page_lines:
            escaped = escape_pdf_text(line)
            if first:
                content_lines.append(f"({escaped}) Tj")
                first = False
            else:
                content_lines.append(f"T* ({escaped}) Tj")
        content_lines.append("ET")
        stream = "\n".join(content_lines).encode("latin-1")

        content_obj = b"<< /Length %d >>\nstream\n" % len(stream) + stream + b"\nendstream"
        objects.append(content_obj)
        content_id = len(objects)

        page_obj = (
            f"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
            f"/Resources << /Font << /F1 {font_id} 0 R >> >> /Contents {content_id} 0 R >>"
        ).encode("latin-1")
        objects.append(page_obj)
        page_ids.append(len(objects))

    kids = " ".join(f"{page_id} 0 R" for page_id in page_ids)
    objects[1] = f"<< /Type /Pages /Count {len(page_ids)} /Kids [ {kids} ] >>".encode("latin-1")
    objects[0] = b"<< /Type /Catalog /Pages 2 0 R >>"

    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as f:
        f.write(b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n")
        offsets = [0]
        for index, obj in enumerate(objects, start=1):
            offsets.append(f.tell())
            f.write(f"{index} 0 obj\n".encode("latin-1"))
            f.write(obj)
            f.write(b"\nendobj\n")

        xref_offset = f.tell()
        f.write(f"xref\n0 {len(objects) + 1}\n".encode("latin-1"))
        f.write(b"0000000000 65535 f \n")
        for offset in offsets[1:]:
            f.write(f"{offset:010d} 00000 n \n".encode("latin-1"))

        trailer = (
            f"trailer\n<< /Size {len(objects) + 1} /Root 1 0 R >>\n"
            f"startxref\n{xref_offset}\n%%EOF\n"
        )
        f.write(trailer.encode("latin-1"))


def sha256_file(path: Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as handle:
        while chunk := handle.read(1024 * 1024):
            hasher.update(chunk)
    return hasher.hexdigest().upper()


def copy_core_tree() -> None:
    src = ROOT / "core"
    dst = STAGE_DIR / "core"
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)


def create_pdfs() -> None:
    docs_dir = STAGE_DIR / "docs"
    for output_name, source_path in PDF_SOURCES.items():
        text = source_path.read_text(encoding="utf-8")
        title = output_name.replace(".pdf", "").replace("_", " ")
        write_simple_text_pdf(docs_dir / output_name, title, text)


def write_checksums() -> None:
    checksums_dir = STAGE_DIR / "checksums"
    checksums_dir.mkdir(parents=True, exist_ok=True)
    output_file = checksums_dir / "sha256.txt"

    lines = [
        f"# SHA256 checksums for {PACKAGE_STEM}",
        f"# Generated: {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M:%S UTC')}",
        "# Note: this file intentionally excludes its own checksum.",
        "",
    ]

    for file_path in sorted(STAGE_DIR.rglob("*")):
        if not file_path.is_file():
            continue
        rel = file_path.relative_to(STAGE_DIR).as_posix()
        if rel == "checksums/sha256.txt":
            continue
        lines.append(f"{sha256_file(file_path)}  {rel}")

    output_file.write_text("\n".join(lines) + "\n", encoding="utf-8")


def build_zip() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    if ZIP_PATH.exists():
        ZIP_PATH.unlink()

    with zipfile.ZipFile(ZIP_PATH, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for file_path in sorted(STAGE_DIR.rglob("*")):
            if file_path.is_file():
                arcname = f"{PACKAGE_STEM}/{file_path.relative_to(STAGE_DIR).as_posix()}"
                zf.write(file_path, arcname)

    PACKAGE_HASH_PATH.write_text(
        f"{sha256_file(ZIP_PATH)}  {ZIP_PATH.name}\n",
        encoding="utf-8",
    )


def main() -> int:
    if STAGE_DIR.exists():
        shutil.rmtree(STAGE_DIR)
    STAGE_DIR.mkdir(parents=True, exist_ok=True)

    copy_core_tree()
    create_pdfs()
    write_checksums()
    build_zip()

    print(f"[UPE] Package directory: {STAGE_DIR}")
    print(f"[UPE] Zip created: {ZIP_PATH}")
    print(f"[UPE] Zip SHA256: {sha256_file(ZIP_PATH)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
