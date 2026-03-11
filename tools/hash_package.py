#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import sys
from datetime import datetime
from pathlib import Path
from zipfile import ZipFile


def sha256_file(path: Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as f:
        while True:
            chunk = f.read(1024 * 1024)
            if not chunk:
                break
            hasher.update(chunk)
    return hasher.hexdigest().upper()


def zip_manifest_hash(path: Path) -> str:
    with ZipFile(path, "r") as zip_file:
        lines = []
        for info in zip_file.infolist():
            if info.is_dir():
                continue
            lines.append(f"{info.filename}|{info.file_size}")
    payload = "\n".join(sorted(lines)).encode("utf-8")
    return hashlib.sha256(payload).hexdigest().upper()


def main() -> int:
    if len(sys.argv) < 2:
        print("Usage: tools/hash_package.py <zip_path> [output_file]")
        return 1

    zip_path = Path(sys.argv[1])
    output = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("proof/CLOSING_HASH_RECORD.txt")

    if not zip_path.exists():
        print(f"[UPE] Zip not found: {zip_path}")
        return 1

    output.parent.mkdir(parents=True, exist_ok=True)

    record = {
        "closing_timestamp": datetime.now().astimezone().isoformat(timespec="seconds"),
        "package_path": str(zip_path).replace("\\", "/"),
        "package_sha256": sha256_file(zip_path),
        "zip_manifest_sha256": zip_manifest_hash(zip_path),
    }

    output.write_text(
        "\n".join(f"{key}={value}" for key, value in record.items()) + "\n",
        encoding="utf-8",
    )

    print(f"[UPE] Hash record written: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
