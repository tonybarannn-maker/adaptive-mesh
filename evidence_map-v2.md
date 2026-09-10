# SOAM Evidence Map (v2)

Цей документ пов’язує теоретичні та технічні твердження проєкту **Self-Organizing Adaptive Mesh (SOAM)** із конкретними програмними та документаційними артефактами в офіційному репозиторії. Він слугує дорожнім вказівником для незалежних технічних рецензентів та грантових аудиторів (зокрема, фонду NLnet) для швидкої верифікації працездатності та зрілості системи (TRL 3).

---

## 1. Core Technological Claims (Основні технічні твердження)

### Claim A: C++20 Domain/Compiled-Runtime Split
* **Твердження:** Domain/value surface є header-defined, а production lifecycle та simulation runtime належать compiled static library.
* **Доказ у коді (Code Evidence):** `include/soam_domain.hpp`, `cmake/templates/system_architecture.hpp.in`, `src/system_architecture.cpp` та `src/detail/spatial_adaptive_mesh_impl.hpp`.
* **Доказ збіжки (Build Evidence):** `CMakeLists.txt` (`set(CMAKE_CXX_STANDARD 20)`).
* **Доказ суворої компіляції (Strict Compilation Logs):** Контур CI у `.github/workflows/ci.yml` із прапорцями `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`.

### Claim B: Deterministic Simulation & Experimental Reproducibility
* **Твердження:** Симуляція є детермінованою та відтворюваною за умови використання однакового Random Seed.
* **Доказ у коді (Code Evidence):** Використання генератора `std::mt19937_64` із фіксованим `Random Seed = 42`.
* **Доказ верифікації (Verification Evidence):** Юніт-тести в `tests/mesh_tests.cpp`.

### Claim C: Adaptive Topology Control (Edge Pruning & Auto-Discovery)
* **Твердження:** Мережа динамічно адаптує топологію, видаляючи деструктивні мости та відкриваючи нові в радіусі взаємодії.
* **Доказ у коді (Code Evidence):** Методи `SpatialAdaptiveMesh::pruneIsolatedBridges()` та `autoConnectNearbyNodes()`.

### Claim D: Mathematical Stability Enforcement (CFL Boundary)
* **Твердження:** Для запобігання чисельним осциляціям підтримується умова стабільності Куранта–Фрідріхса–Леві (CFL) із 20% запасом.
* **Доказ у коді (Code Evidence):** Метод `SpatialAdaptiveMesh::enforceStabilityCondition()`.

---

## 2. Work Package (WP) to Deliverables Mapping

| Робочий пакет (Work Package) | Очікуваний артефакт (Deliverable) | Цільовий файл у репозиторії (Target File) | Статус валідації |
| :--- | :--- | :--- | :--- |
| **WP1: Performance & Scale** | **D1:** Відтворюваний benchmark harness | `benchmarks/mesh_benchmark.cpp` | ✅ Реалізовано |
| **WP2: Formal Analysis** | **D3:** Документація інваріантів | `docs/mathematics.md` | ✅ Затверджено |
| **WP3: Integrity & Resilience** | **D4:** Перевірки pruning та bounded simulation | `tests/mesh_tests.cpp` | ✅ Реалізовано |
