# Topic Truth Audit

Generated from `core/src/config/upe_science_topics_generated.cpp`.

## Global Counts

- Total topics: **282**
- Tier A (Runtime-Integrated Module): **103**
- Tier B (API/Helper-Level): **117**
- Tier C (Catalog-Only / Not Implemented): **62**

## Domain Breakdown

| Domain | Total | Tier A | Tier B | Tier C |
|---|---:|---:|---:|---:|
| ASTROLOGY | 9 | 0 | 0 | 9 |
| ASTRONOMY | 37 | 0 | 37 | 0 |
| CHEM | 35 | 0 | 0 | 35 |
| MATH | 55 | 0 | 55 | 0 |
| OPT | 64 | 64 | 0 | 0 |
| PHYSICS | 39 | 39 | 0 | 0 |
| SPACE | 25 | 0 | 25 | 0 |
| STRING | 18 | 0 | 0 | 18 |

## Tier Rule Used

- If catalog level is `NOT_IMPLEMENTED`, classify as Tier C.
- Otherwise domain mapping is conservative: `PHYSICS/OPT -> A`, `MATH/SPACE/ASTRONOMY -> B`, `CHEM/ASTROLOGY/STRING -> C`.

## Artifact

- Per-topic CSV: `../topic_truth_audit.csv`
