window.ARTIC_BENCHMARKS = [
  {
    category: 'World Step',
    name: 'World Step V2',
    config: 'Reference fixed-step scene with deterministic build settings.',
    lastBaseline: 'See docs/BENCHMARK_BASELINES.md',
    currentReference: 'Repo benchmark runner available',
    delta: 'ref-only',
    environment: 'MSYS2/UCRT64 + CMake/Ninja reference lane',
    note: 'Used to assess total step cost under the documented reference methodology.'
  },
  {
    category: 'Collision',
    name: 'Collision Benchmark',
    config: 'Broadphase + narrowphase heavy scene sweep.',
    lastBaseline: 'Published in baseline docs',
    currentReference: 'Reproducible from benchmark sources',
    delta: 'ref-only',
    environment: 'See benchmark baseline notes',
    note: 'Measures contact-heavy collision performance rather than abstract synthetic claims.'
  },
  {
    category: 'Solver',
    name: 'Constraint Solver Benchmark',
    config: 'Iterative solve behavior under stacked contact and joint load.',
    lastBaseline: 'Published reference',
    currentReference: 'Rerunnable in local environment',
    delta: 'ref-only',
    environment: 'Reference build flow',
    note: 'Useful for comparing stability-oriented iteration budgets versus throughput.'
  },
  {
    category: 'Fluids',
    name: 'SPH / Fluid Benchmark',
    config: 'Particle-based fluid scenario from the repo benchmark set.',
    lastBaseline: 'Published reference',
    currentReference: 'Rerunnable from `bench_fluids.cpp`',
    delta: 'ref-only',
    environment: 'Same published environment notes',
    note: 'Represents breadth benchmarking, not the core sale story alone.'
  },
  {
    category: 'Science Export',
    name: 'Science Export Benchmark',
    config: 'Documentation/export lane used for proof packaging and reporting.',
    lastBaseline: 'Published reference',
    currentReference: 'Rerunnable from repo tooling',
    delta: 'ref-only',
    environment: 'Reference packaging flow',
    note: 'Shows discipline around reporting and export tooling, not just runtime speed.'
  },
  {
    category: 'Scaling',
    name: 'Standard Scenes Scaling Test',
    config: 'Scaling behavior across standard benchmark scenes.',
    lastBaseline: 'Published reference',
    currentReference: 'Rerunnable locally',
    delta: 'ref-only',
    environment: 'See methodology notes on this page',
    note: 'Should always be interpreted alongside workload description and hardware context.'
  }
];
