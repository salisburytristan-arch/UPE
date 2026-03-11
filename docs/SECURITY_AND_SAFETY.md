# Security and Safety

## Telemetry and Network Behavior

- No telemetry requirement is imposed by build/test scripts.
- No network connectivity is required for local build/test/proof commands.

## Prohibited Unsafe Practices

- Do not enable non-deterministic optimization flags for determinism claims.
- Do not run unknown binaries from untrusted directories.

## Operational Safety

- Run scripts from repository root only.
- Review generated logs under `proof/RUNS/` for unexpected behavior.
