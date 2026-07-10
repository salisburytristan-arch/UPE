# Determinism Scenarios

These scenarios extend the replay and hash surface with focused regression checks.

- `test_determinism_replay_scenarios.cpp` exercises world hash and replay recorder hooks.
- `test_determinism_snapshot_roundtrip.cpp` anchors state-hash roundtrip coverage.

The actual test executable is still assembled through the main test target in `CMakeLists.txt`.