# Verification Status Dashboard

This dashboard is the quick status view for top-to-bottom verification progress.

## Current State

- Last dashboard update (UTC): 2026-03-11
- Current verified commit: `d9164ee`
- Full matrix run URL: `PENDING`
- Full matrix run ID: `PENDING`
- Overall status: `IN PROGRESS - awaiting CI evidence`

## CI Matrix Evidence Board

| Lane | Status | Evidence |
| --- | --- | --- |
| Linux Debug/Release gcc | pending | awaiting latest run URL/artifacts |
| Linux Debug/Release clang | pending | awaiting latest run URL/artifacts |
| Windows Debug/Release | pending | awaiting latest run URL/artifacts |
| macOS Debug/Release | pending | awaiting latest run URL/artifacts |
| strict warnings gcc | pending | awaiting latest run URL/artifacts |
| strict warnings clang | pending | awaiting latest run URL/artifacts |
| cross-compiler check | pending | awaiting latest run URL/artifacts |
| determinism Windows | pending | awaiting latest run URL/artifacts |
| determinism macOS | pending | awaiting latest run URL/artifacts |
| determinism certification | pending | awaiting latest run URL/artifacts |
| world-step benchmarks | pending | awaiting latest run URL/artifacts |
| release package gate | pending | awaiting latest run URL/artifacts |

## Proof Artifact Tracking

- Verification pass date: `PENDING`
- Artifact names: `PENDING`
- Artifact IDs: `PENDING`
- Benchmark artifact IDs: `PENDING`
- Determinism artifact IDs: `PENDING`
- Release artifact IDs: `PENDING`

## Mapping to Checklist

Primary checklist source:
- `docs/SCIENTIFIC_COMPLETENESS_PLAN.md` (Top-to-Bottom Verification Status section)

Proof index source:
- `docs/PROOF_INDEX.md` (Latest Full-Matrix Verification Record)

## Next Action

1. Capture the latest full-matrix workflow run URL and ID.
2. Mark each lane above as passed/failed with artifact evidence.
3. Update Section B in `docs/SCIENTIFIC_COMPLETENESS_PLAN.md` to evidence-backed `[x]` once all required lanes pass.
