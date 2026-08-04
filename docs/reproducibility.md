# Reproducibility Specification

To ensure exact reproducibility of the benchmark results provided in `docs/diagrams/`, experiments must be executed under controlled environment conditions.

## Experimental Environment Setup

| Parameter | Reference Value / Toolchain |
| :--- | :--- |
| **Compiler** | `GCC 13.2.0` / `Clang 17.0.0` (C++20 flag enabled: `-std=c++20`) |
| **Build System** | `CMake >= 3.25` (Release Build: `-O3 -DNDEBUG`) |
| **Target OS** | `Linux x86_64` (Kernel 6.x or higher) |
| **CPU Architecture** | 8 Cores / 16 Threads (e.g., AMD Zen 4 / Intel 13th Gen) |
| **Thread Count** | Fixed via `SIMULATION_THREADS=8` |
| **Random Seed** | Fixed explicit seed: `42` (`std::mt19937_64(42)`) |

## Topology Generation Parameters
* **Erdős–Rényi:** $N = 1000, p = 0.005$
* **Barabási–Albert:** $N = 1000, m = 3$
* **Watts–Strogatz:** $N = 1000, k = 6, p = 0.1$
* **Perturbation Model:** Single-node impulse shock at $t = 10$, magnitude $\Delta S = +5.0$.
