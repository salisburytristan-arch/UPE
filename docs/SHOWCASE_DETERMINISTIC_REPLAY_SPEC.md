# Showcase Spec: Deterministic Replay and Rollback

## Goal

Provide one high-signal buyer demo that proves UPE's strategic wedge:
reproducible deterministic replay/rollback with observable divergence detection.

## Why This Demo

- Distinguishes UPE from commodity physics middleware.
- Maps directly to netcode, rollback, and simulation verification needs.
- Connects to existing proof artifacts and CI evidence.
- Is reproducible with one command.

## Demo Flow

1. Run a baseline scene for N frames with fixed dt and fixed seed.
2. Capture periodic snapshots and per-frame state hashes.
3. Replay from snapshot using the same input stream.
4. Show frame-by-frame hash agreement (`MATCH=true` expected).
5. Optional perturbation mode flips one controlled input to show divergence detection.

## Required Outputs

- Console summary with:
- frame count
- hash_run_1
- hash_run_2
- match status
- optional divergence frame index
- Artifact files under `proof/RUNS/`:
- run log
- input trace
- replay trace
- hash comparison summary (machine-readable JSON)

## Success Criteria

- One-command execution from repository root.
- Repeatable pass on default supported proof path.
- Clear pass/fail messaging understandable by non-engineering stakeholders.
- Documentation links to proof hierarchy and determinism certification.

## Recommended Command Shape

```bash
python scripts/run_buyer_determinism_proof.py --showcase
```

If `--showcase` is not implemented yet, document current command and planned extension.

## Audience Mapping

- Technical buyer: reproducibility mechanics, hashes, artifacts, replay semantics.
- Business buyer: risk reduction and verification confidence.
- Integration lead: command reproducibility and boundary conditions.

## Related References

- `PROOF_INDEX.md`
- `DETERMINISM_CERTIFICATION.md`
- `CI_GATES.md`
- `KNOWN_LIMITATIONS.md`
