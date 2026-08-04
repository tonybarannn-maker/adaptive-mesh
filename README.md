# Self-Organizing Adaptive Mesh (SOAM)
## Highlights

### Licensing
- Added SOAM copyright attribution to GNU AGPLv3 license.
- Confirmed AGPLv3 open-source licensing model.
- Added commercial licensing contact information.

### Verification
- GPG signed release tag.
- Verified author signature:
  Mykola Bezruchko <tonybarannn@gmail.com>
  
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.21796944.svg)](https://doi.org/10.5281/zenodo.21796944)
[![CI](https://github.com/tonybarannn-maker/adaptive-mesh/actions/workflows/ci.yml/badge.svg)](https://github.com/tonybarannn-maker/adaptive-mesh/actions/workflows/ci.yml)
[![Version](https://img.shields.io/badge/version-1.1.0-blue.svg)](CHANGELOG.md)
[![License](https://img.shields.io/badge/license-AGPLv3-green.svg)](LICENSE)

Header-only C++20 бібліотека та специфікація для побудови самоорганізованих, відмовостійких мереж. Модель поєднує дифузійні процеси на графах, трирівневу ієрархію стабілізації та кібернетичні рефлексивні контури управління.

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
Цей проєкт суворо дотримується [Semantic Versioning (SemVer) 2.0.0](https://semver.org/spec/v2.0.0.html). Будь-які зміни в публічному API, форматах матриці `MetaEvaluator` або глобальних константах безпеки призводитимуть до зміни мажорної або мінорної версії.
