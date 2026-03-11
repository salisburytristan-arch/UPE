# Troubleshooting

| Symptom | Likely Cause | Fix | How to Verify |
| --- | --- | --- | --- |
| Configure fails (compiler not found) | Missing toolchain in PATH | Install compiler and rerun diagnostics | `tools/diagnose_environment` shows compiler path |
| Build fails quickly | Missing CMake/Ninja | Install missing prerequisites | Build script exits `0` and writes success log |
| Tests fail | Runtime mismatch or regression | Rebuild clean and rerun tests | `ctest` reports `100% tests passed` |
| Determinism mismatch | Build flags/toolchain drift | Rebuild with deterministic flags and rerun proof | `MATCH=true` in determinism log |
| Example smoketest fails | Missing example binaries | Build examples then rerun smoketest | Script reports `SUCCESS` |
