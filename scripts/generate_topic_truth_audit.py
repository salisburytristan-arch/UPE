import ast
import csv
import io
import re
import argparse
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC_CPP = ROOT / "core" / "src" / "config" / "upe_science_topics_generated.cpp"
OUT_CSV = ROOT / "topic_truth_audit.csv"
OUT_MD = ROOT / "docs" / "TOPIC_TRUTH_AUDIT.md"

TOPIC_BLOCK_RE = re.compile(
    r"\{\s*"
    r"(?P<id>\d+)u,\s*"
    r"(?P<domain>UPE_SCIENCE_DOMAIN_[A-Z_]+),\s*"
    r"(?P<level>UPE_SCIENCE_LEVEL_[A-Z_]+),\s*"
    r'"(?P<topic>(?:[^"\\]|\\.)*)"\s*'
    r"\},",
    re.MULTILINE,
)

DOMAIN_TO_TIER = {
    "UPE_SCIENCE_DOMAIN_OPT": "A",
    "UPE_SCIENCE_DOMAIN_PHYSICS": "A",
    "UPE_SCIENCE_DOMAIN_MATH": "B",
    "UPE_SCIENCE_DOMAIN_SPACE": "B",
    "UPE_SCIENCE_DOMAIN_ASTRONOMY": "B",
    "UPE_SCIENCE_DOMAIN_CHEM": "C",
    "UPE_SCIENCE_DOMAIN_ASTROLOGY": "C",
    "UPE_SCIENCE_DOMAIN_STRING": "C",
}

TIER_LABELS = {
    "A": "Runtime-Integrated Module",
    "B": "API/Helper-Level",
    "C": "Catalog-Only / Not Implemented",
}

LEVEL_TO_TEXT = {
    "UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED": "NOT_IMPLEMENTED",
    "UPE_SCIENCE_LEVEL_JUST_STARTED": "JUST_STARTED",
    "UPE_SCIENCE_LEVEL_MINIMAL": "MINIMAL",
    "UPE_SCIENCE_LEVEL_EXTREME": "EXTREME",
}


def _decode_cpp_string(value: str) -> str:
    return ast.literal_eval(f'"{value}"')


def _truth_tier(domain: str, level: str) -> str:
    if level == "UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED":
        return "C"
    return DOMAIN_TO_TIER.get(domain, "C")


def _truth_basis(domain: str, level: str, tier: str) -> str:
    if level == "UPE_SCIENCE_LEVEL_NOT_IMPLEMENTED":
        return "Catalog marks NOT_IMPLEMENTED"
    if tier == "A":
        return "Conservative domain mapping: runtime-integrated core exists"
    if tier == "B":
        return "Conservative domain mapping: helper/API-level coverage"
    return "Conservative fallback"


def _build_rows(source_text: str):
    rows = []
    for match in TOPIC_BLOCK_RE.finditer(source_text):
        topic_id = int(match.group("id"))
        domain = match.group("domain")
        level = match.group("level")
        topic = _decode_cpp_string(match.group("topic"))
        tier = _truth_tier(domain, level)
        rows.append(
            {
                "topic_id": topic_id,
                "domain": domain.replace("UPE_SCIENCE_DOMAIN_", ""),
                "catalog_level": LEVEL_TO_TEXT.get(level, level),
                "truth_tier": tier,
                "truth_tier_label": TIER_LABELS[tier],
                "topic": topic,
                "basis": _truth_basis(domain, level, tier),
            }
        )
    rows.sort(key=lambda row: row["topic_id"])
    return rows


def _render_csv(rows) -> str:
    fieldnames = [
        "topic_id",
        "domain",
        "catalog_level",
        "truth_tier",
        "truth_tier_label",
        "topic",
        "basis",
    ]
    buffer = io.StringIO(newline="")
    writer = csv.DictWriter(buffer, fieldnames=fieldnames)
    writer.writeheader()
    writer.writerows(rows)
    return buffer.getvalue()


def _render_markdown(rows) -> str:
    domain_counts = Counter(row["domain"] for row in rows)
    tier_counts = Counter(row["truth_tier"] for row in rows)

    domain_tier_counts = {}
    for row in rows:
        domain = row["domain"]
        tier = row["truth_tier"]
        if domain not in domain_tier_counts:
            domain_tier_counts[domain] = Counter()
        domain_tier_counts[domain][tier] += 1

    md_lines = []
    md_lines.append("# Topic Truth Audit")
    md_lines.append("")
    md_lines.append("Generated from `core/src/config/upe_science_topics_generated.cpp`.")
    md_lines.append("")
    md_lines.append("## Global Counts")
    md_lines.append("")
    md_lines.append(f"- Total topics: **{len(rows)}**")
    md_lines.append(f"- Tier A (Runtime-Integrated Module): **{tier_counts.get('A', 0)}**")
    md_lines.append(f"- Tier B (API/Helper-Level): **{tier_counts.get('B', 0)}**")
    md_lines.append(f"- Tier C (Catalog-Only / Not Implemented): **{tier_counts.get('C', 0)}**")
    md_lines.append("")
    md_lines.append("## Domain Breakdown")
    md_lines.append("")
    md_lines.append("| Domain | Total | Tier A | Tier B | Tier C |")
    md_lines.append("|---|---:|---:|---:|---:|")
    for domain in sorted(domain_counts):
        counts = domain_tier_counts.get(domain, Counter())
        md_lines.append(
            f"| {domain} | {domain_counts[domain]} | {counts.get('A', 0)} | {counts.get('B', 0)} | {counts.get('C', 0)} |"
        )

    md_lines.append("")
    md_lines.append("## Tier Rule Used")
    md_lines.append("")
    md_lines.append("- If catalog level is `NOT_IMPLEMENTED`, classify as Tier C.")
    md_lines.append("- Otherwise domain mapping is conservative: `PHYSICS/OPT -> A`, `MATH/SPACE/ASTRONOMY -> B`, `CHEM/ASTROLOGY/STRING -> C`.")
    md_lines.append("")
    md_lines.append("## Artifact")
    md_lines.append("")
    md_lines.append("- Per-topic CSV: `../topic_truth_audit.csv`")
    md_lines.append("")
    return "\n".join(md_lines)


def _existing_text(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate or validate topic truth audit artifacts")
    parser.add_argument(
        "--check",
        action="store_true",
        help="Validate that topic_truth_audit.csv and docs/TOPIC_TRUTH_AUDIT.md are up-to-date",
    )
    args = parser.parse_args()

    if not SRC_CPP.exists():
        raise FileNotFoundError(f"Missing source file: {SRC_CPP}")

    text = SRC_CPP.read_text(encoding="utf-8")
    rows = _build_rows(text)

    if not rows:
        raise RuntimeError("No topic records were parsed from generated topic source")

    csv_text = _render_csv(rows)
    md_text = _render_markdown(rows)

    if args.check:
        stale = []
        if _existing_text(OUT_CSV).replace("\r\n", "\n") != csv_text.replace("\r\n", "\n"):
            stale.append(str(OUT_CSV))
        if _existing_text(OUT_MD).replace("\r\n", "\n") != md_text.replace("\r\n", "\n"):
            stale.append(str(OUT_MD))
        if stale:
            print("Topic truth audit artifacts are stale. Regenerate with:")
            print("  py -3 scripts/generate_topic_truth_audit.py")
            print("Stale files:")
            for path in stale:
                print(f"  - {path}")
            return 1
        print("Topic truth audit artifacts are up-to-date.")
        return 0

    OUT_CSV.write_text(csv_text, encoding="utf-8", newline="")
    OUT_MD.parent.mkdir(parents=True, exist_ok=True)
    OUT_MD.write_text(md_text, encoding="utf-8")

    domain_counts = Counter(row["domain"] for row in rows)
    tier_counts = Counter(row["truth_tier"] for row in rows)
    print(f"Wrote {len(rows)} rows to {OUT_CSV}")
    print(f"Wrote summary to {OUT_MD}")
    print("Tier counts:", dict(sorted(tier_counts.items())))
    print("Domain counts:", dict(sorted(domain_counts.items())))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
