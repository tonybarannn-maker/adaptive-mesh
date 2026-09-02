# Reproducibility Specification

The current reference implementation is a header-only C++20 library built with CMake 3.25 or newer. The CI workflow configures a Release build with warnings as errors and separately runs a Debug AddressSanitizer/UndefinedBehaviorSanitizer build on Ubuntu.

## Runtime configuration

`SpatialAdaptiveMesh(maxWorkers)` controls the number of reusable simulation workers. `maxWorkers == 0` selects an automatic limit of `min(max(1, std::thread::hardware_concurrency()), node_count)`. A positive value is capped at `node_count`; workers receive deterministic index ranges for each step.

## Current reproducibility boundary

The repository does not currently implement a random-number generator, a random seed, or Erdős–Rényi, Barabási–Albert, and Watts–Strogatz topology generators. It also does not provide a `SIMULATION_THREADS` environment setting. Reproducible runs therefore require callers to construct the same nodes and connections in the same order and to use the same `maxWorkers` configuration.

The checked-in smoke test uses a three-node linear topology and a single external shock of `+2.0`; it validates finite state and health after one simulation step. The scale smoke tests use deterministic linear topologies with 100 and 1000 nodes.
