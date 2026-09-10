# Self-Organizing Adaptive Mesh (SOAM)
## Highlights

### Licensing
- Repository contents are distributed under the GNU Affero General Public
  License v3 as specified in [`LICENSE`](LICENSE).
- Separate commercial licensing terms are not granted by this repository.
  Inquiries may be sent to the copyright holder using the contact address in
  `LICENSE`.

### Verification
- Release and tag signatures must be verified against the exact Git object;
  source-tree documentation does not treat an unverified signature as evidence.
  
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.21796944.svg)](https://doi.org/10.5281/zenodo.21796944)
[![CI](https://github.com/tonybarannn-maker/adaptive-mesh/actions/workflows/ci.yml/badge.svg)](https://github.com/tonybarannn-maker/adaptive-mesh/actions/workflows/ci.yml)
[![Version](https://img.shields.io/badge/version-2.0.0--development-blue.svg)](CHANGELOG.md)
[![License](https://img.shields.io/badge/license-AGPLv3-green.svg)](LICENSE)

C++20 бібліотека та специфікація для побудови самоорганізованих, відмовостійких мереж. SOAM 2.0 розділяє installed header-defined domain surface (`AdaptiveMesh::adaptive_mesh_domain`) і compiled static production runtime (`AdaptiveMesh::adaptive_mesh`). Live K12 integration надається окремим target `AdaptiveMesh::adaptive_mesh_k12_live`.

## 📐 Architecture & Abstraction Layers

```text
Conceptual Model
       │
       ▼
Mathematical Formulation
       │
       ▼
Reference Algorithms
       │
       ▼
C++20 Reference Implementation
       │
       ▼
Experimental Evaluation
```

## 🎯 Project Scope

To prevent misinterpretation of the project's goals, the functional scope is explicitly bounded:

### In Scope
* **Graph Diffusion:** Discrete state propagation across static and dynamic graphs.
* **Adaptive Topology:** Local edge attenuation, capacity modulation, and edge pruning.
* **Reflective Control:** Two-tier feedback loops (local filter + meta-evaluation layer).
* **Benchmarking:** Reproducible empirical evaluations across synthetic graph topologies.

### Out of Scope
* **Biological Modeling:** Not intended as a model for biological neural networks or cellular autopoiesis.
* **Cognitive Architectures & AGI:** No claims regarding intelligence, cognition, or general reasoning.
* **Formal Verification:** Code and algorithms are reference implementations and lack machine-checked formal proofs (e.g., Coq/Lean).
* **Real-Time Guarantees:** Lacks hard real-time execution bounds (POSIX RT / WCET guarantees).

## 🚀 Quick Start

### Prerequisites
* C++20 compatible compiler (GCC 11+, Clang 13+, MSVC 2019+)
* CMake 3.25+

### Build and Run Tests
```bash
git clone https://github.com/tonybarannn-maker/adaptive-mesh.git
cd adaptive-mesh
cmake -B build -DBUILD_TESTS=ON
cmake --build build
./build/mesh_tests
```

## 🏛 Key Concepts
* **Local Reflex Layer (dt -> 0):** High-frequency node-level filters to damp localized noise.
* **Regional Consensus Layer (Mesoscale):** Dynamic topology management, edge pruning/auto-discovery, and CFL stability enforcement.
* **Global Invariant Boundary (Omega):** MetaEvaluator second-order reflection layer and safety limits.

Detailed models are specified in `ARCHITECTURE.md` and `docs/assumptions_and_limitations.md`.

## Versioning
Поточна гілка вихідного коду має ідентичність `2.0.0 development` і є
developer-preview surface, а не опублікованим релізом `2.0.0`. Класифікація
публічного API, сумісність і межі гарантій визначені в
[`docs/public-api-stability.md`](docs/public-api-stability.md). Семантичне
версіювання застосовується до опублікованих релізів; номер development package
не є гарантією ABI-сумісності.
