# Contributing

Thanks for your interest in contributing to Unified Physics Engine.

## Scope

This project accepts contributions for:
- Build reliability and packaging quality
- Physics correctness and determinism
- Documentation accuracy and examples
- Test coverage and CI stability

## Workflow

1. Fork or branch from `main`.
2. Make focused changes (one logical issue per PR when possible).
3. Run local checks before opening a PR.
4. Open a PR with a clear problem statement and validation notes.

## Local Validation

Recommended commands:

```bash
cmake -S . -B build -DUPE_BUILD_TESTS=ON -DUPE_BUILD_EXAMPLES=ON -DUPE_DETERMINISTIC=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

For release quality checks:

```bash
python scripts/verify_release_gate.py --create --dir releases_ci
```

## Coding Guidelines

- Keep changes minimal and scoped to the issue.
- Preserve existing public API unless explicitly changing versioned behavior.
- Prefer deterministic behavior over platform-specific shortcuts.
- Avoid introducing third-party dependencies without discussion.

## Documentation Guidelines

- Keep links relative and valid inside packaged tiers.
- Update docs when behavior or commands change.
- Avoid claims that cannot be validated by current CI/build scripts.

## Pull Request Checklist

- [ ] Build passes with your target toolchain.
- [ ] Tests pass (or failures are documented with rationale).
- [ ] New/changed docs are accurate for shipped package contents.
- [ ] Release gate passes for packaging-related changes.

## Reporting Issues

Please include:
- OS and compiler version
- Configure/build command used
- First failing error block
- Repro steps or minimal input
