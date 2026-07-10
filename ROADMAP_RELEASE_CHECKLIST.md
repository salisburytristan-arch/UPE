# UPE Priority Roadmap Release Checklist

Date: 2026-07-10

## P0 Buyer Trust

1. START_HERE at root: [START_HERE.md](START_HERE.md)
2. Verify scripts: [scripts/verify_package.ps1](scripts/verify_package.ps1), [scripts/verify_package.sh](scripts/verify_package.sh)
3. Verification flow and log output path: [scripts/verify_package.ps1](scripts/verify_package.ps1), [scripts/verify_package.sh](scripts/verify_package.sh), [proof/latest_verification_log.txt](proof/latest_verification_log.txt)
4. Markdown links check script: [scripts/check_markdown_links.py](scripts/check_markdown_links.py)
5. Docs aligned to included files: [docs/INDEX.md](docs/INDEX.md), [BUYER_PACKAGE_CONTENTS.md](BUYER_PACKAGE_CONTENTS.md)
6. Module stability matrix: [MODULE_STABILITY.md](MODULE_STABILITY.md)
7. Current caveats: [LIMITATIONS.md](LIMITATIONS.md)
8. Buyer package contents: [BUYER_PACKAGE_CONTENTS.md](BUYER_PACKAGE_CONTENTS.md)
9. Checksum verification scripts: [scripts/verify_checksums.ps1](scripts/verify_checksums.ps1), [scripts/verify_checksums.sh](scripts/verify_checksums.sh)

## P1 Determinism

10. Determinism scenarios: [tests/determinism/test_determinism_replay_scenarios.cpp](tests/determinism/test_determinism_replay_scenarios.cpp), [tests/determinism/test_determinism_snapshot_roundtrip.cpp](tests/determinism/test_determinism_snapshot_roundtrip.cpp)
11. Final-state transform hash output: [tests/test_runner.cpp](tests/test_runner.cpp)
12. Replay recorder/player tools: [core/include/upe/upe_replay.h](core/include/upe/upe_replay.h), [core/include/upe/upe_replay_file.h](core/include/upe/upe_replay_file.h), [core/include/upe/upe_world_ex.h](core/include/upe/upe_world_ex.h)
13. Platform matrix: [proof/PLATFORM_MATRIX.md](proof/PLATFORM_MATRIX.md)
14. Deterministic replay example: [examples/deterministic_replay_demo/main.cpp](examples/deterministic_replay_demo/main.cpp)
15. CI-style determinism report: [proof/CI_DETERMINISM_REPORT.md](proof/CI_DETERMINISM_REPORT.md)

## P2 Collision Proof

16. CCD projectile stress tests: [tests/cases/test_ccd_stress.cpp](tests/cases/test_ccd_stress.cpp), [tests/cases/test_ccd_stress_box.cpp](tests/cases/test_ccd_stress_box.cpp)
17. CCD demo example: [examples/ccd_projectile_demo/main.cpp](examples/ccd_projectile_demo/main.cpp)
18. CCD benchmark scene coverage: [tests/cases/test_ccd_stress.cpp](tests/cases/test_ccd_stress.cpp), [examples/ccd_projectile_demo/main.cpp](examples/ccd_projectile_demo/main.cpp)
19. Broadphase comparison benchmark: [benchmarks/bench_broadphase_compare.cpp](benchmarks/bench_broadphase_compare.cpp)
20. Collision debug visualization: [examples/collision_debug_visualization/main.cpp](examples/collision_debug_visualization/main.cpp)

## P3 Performance

21. Benchmark runner: [scripts/run_performance_suite.py](scripts/run_performance_suite.py)
22. JSON/CSV/Markdown outputs: [benchmarks/bench_performance_suite.cpp](benchmarks/bench_performance_suite.cpp)
23. Thread scaling benchmark: [benchmarks/bench_performance_suite.cpp](benchmarks/bench_performance_suite.cpp)
24. SIMD on/off benchmark slice: [benchmarks/bench_performance_suite.cpp](benchmarks/bench_performance_suite.cpp)
25. Memory usage benchmark slice: [benchmarks/bench_performance_suite.cpp](benchmarks/bench_performance_suite.cpp)
26. Performance charts (HTML): [benchmarks/bench_performance_suite.cpp](benchmarks/bench_performance_suite.cpp)

## P4 Examples

27. Hello world example: [examples/hello_world/main.cpp](examples/hello_world/main.cpp)
28. Rigidbody stack example: [examples/rigidbody_stack_demo/main.cpp](examples/rigidbody_stack_demo/main.cpp)
29. Fixed timestep example: [examples/fixed_timestep_demo/main.cpp](examples/fixed_timestep_demo/main.cpp)
30. Constraints bridge example: [examples/constraints_bridge_demo/main.cpp](examples/constraints_bridge_demo/main.cpp)
31. Character controller demo: [examples/character_controller_demo/main.cpp](examples/character_controller_demo/main.cpp)
32. Vehicle demo: [examples/advanced_vehicle_demo/main.cpp](examples/advanced_vehicle_demo/main.cpp)
33. WASM bridge demo: [examples/wasm_bridge_demo/main.cpp](examples/wasm_bridge_demo/main.cpp)

## P5 API Polish

34. Fixed timestep API surface: [core/include/upe/upe_descriptor.h](core/include/upe/upe_descriptor.h), [core/include/upe/upe_api.h](core/include/upe/upe_api.h)
35. Debug draw API: [core/include/upe/upe_debug_draw.h](core/include/upe/upe_debug_draw.h)
36. Scene serialization API: [core/include/upe/upe_serialize.h](core/include/upe/upe_serialize.h)
37. Snapshot/restore API: [core/include/upe/upe_snapshot.h](core/include/upe/upe_snapshot.h)
38. C API wrapper surface: [core/include/upe/upe_api.h](core/include/upe/upe_api.h)
39. API cookbook docs: [docs/API_COOKBOOK.md](docs/API_COOKBOOK.md)

## P6 Commercial Polish

40. Commercial license docs: [COMMERCIAL_LICENSE_AGREEMENT.md](COMMERCIAL_LICENSE_AGREEMENT.md), [COMMERCIALIZATION_GUIDE.md](COMMERCIALIZATION_GUIDE.md)
41. Pricing/edition docs: [docs/PRICING_AND_EDITIONS.md](docs/PRICING_AND_EDITIONS.md)
42. One-page sales sheet: [UPE_ONE_PAGE_SALES_SHEET.md](UPE_ONE_PAGE_SALES_SHEET.md)
43. Product screenshots: [site-data/product-shots/index.png](site-data/product-shots/index.png), [site-data/product-shots/upe.png](site-data/product-shots/upe.png), [site-data/product-shots/determinism.png](site-data/product-shots/determinism.png), [site-data/product-shots/benchmarks.png](site-data/product-shots/benchmarks.png), [site-data/product-shots/visual-tests.png](site-data/product-shots/visual-tests.png)
44. Five demo videos: [site-data/product-videos/index_overview.mp4](site-data/product-videos/index_overview.mp4), [site-data/product-videos/upe_overview.mp4](site-data/product-videos/upe_overview.mp4), [site-data/product-videos/determinism_proof.mp4](site-data/product-videos/determinism_proof.mp4), [site-data/product-videos/benchmarks_dashboard.mp4](site-data/product-videos/benchmarks_dashboard.mp4), [site-data/product-videos/visual_tests_gallery.mp4](site-data/product-videos/visual_tests_gallery.mp4)
45. Product page updates: [products.html](products.html)