# Changelog

All notable changes to this project will be documented in this file. The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-08-02

### Added
- **Dynamic Topology:** Added `pruneIsolatedBridges()` and `autoConnectNearbyNodes()` for edge lifecycle management.
- **Async Execution:** Multi-threaded state processing via `std::jthread` in `simulationStepAsync()`.
- **CFL Condition:** Automatic computation and adjustment of diffusion rate $\alpha \le \frac{1}{D_{\max}}$.
- **Documentation:** Added `assumptions_and_limitations.md`, `cybernetics.md`, and `CITATION.cff`.
- **Benchmarks:** Added visual experiment benchmarks in `docs/diagrams/convergence_benchmarks.png`.

### Fixed
- Corrected terminology across documentation (e.g., "distributed" instead of typos).
- Bounded thread-safe atomic operations on node states and health metrics.

## [1.0.0] - 2026-07-28
- Initial architecture release with basic diffusion model and 3-layer control hierarchy.
