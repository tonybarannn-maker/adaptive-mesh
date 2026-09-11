# Runtime consumption and migration contract

## Status

This document defines the downstream consumption contract for the SOAM `2.0.0 development` package. It does not change runtime semantics, authority semantics, mutation behavior, or ABI guarantees.

SOAM currently separates the header-defined domain surface from compiled runtime implementations:

- `AdaptiveMesh::adaptive_mesh_domain` — header-defined domain/value API;
- `AdaptiveMesh::adaptive_mesh` — normal compiled static runtime;
- `AdaptiveMesh::adaptive_mesh_k12_live` — experimental K12 live companion linked on top of the normal runtime.

## Canonical CMake consumption

Installed consumers should prefer the package targets exported by `find_package`:

```cmake
find_package(AdaptiveMesh 2.0 CONFIG REQUIRED)

add_executable(my_domain_tool main.cpp)
target_link_libraries(my_domain_tool PRIVATE AdaptiveMesh::adaptive_mesh_domain)

add_executable(my_runtime_tool runtime.cpp)
target_link_libraries(my_runtime_tool PRIVATE AdaptiveMesh::adaptive_mesh)
```

A consumer that requires the experimental K12 live evaluator links `AdaptiveMesh::adaptive_mesh_k12_live` instead. That target transitively links the normal runtime.

Consumers should not infer support from a declaration merely being visible in an installed header. Public support classification remains defined by `docs/public-api-stability.md`.

## Migration from include-only use

Code that uses only header-defined domain/value types may continue to link only `AdaptiveMesh::adaptive_mesh_domain`.

Code that constructs or calls the compiled runtime surface, including `SpatialAdaptiveMesh`, must link the compiled runtime target `AdaptiveMesh::adaptive_mesh`. Including `system_architecture.hpp` alone is not a complete runtime consumption model.

The migration rule is therefore:

```text
header/value-only consumer
    -> AdaptiveMesh::adaptive_mesh_domain

runtime consumer
    -> AdaptiveMesh::adaptive_mesh

K12 live consumer
    -> AdaptiveMesh::adaptive_mesh_k12_live
       -> AdaptiveMesh::adaptive_mesh
```

The static runtime model is already part of the current development package. This document does not introduce an `INTERFACE -> STATIC` transition; it documents the existing contract.

## Manual non-CMake linking

Manual linking is a lower-level compatibility path, not the preferred package interface. A manual consumer must provide all of the following consistently from the same SOAM installation/build:

1. an include path containing the installed SOAM headers, including the generated normal runtime headers;
2. the normal runtime archive (`adaptive_mesh`) when using `SpatialAdaptiveMesh` or other compiled runtime symbols;
3. the K12 live archive (`adaptive_mesh_k12_live`) in addition to the normal runtime when using the experimental live evaluator;
4. the platform thread dependency required by the runtime;
5. C++20 compilation with a compatible compiler and standard-library configuration.

Archive file names and platform-specific linker syntax are toolchain-dependent. Consumers must not treat one concrete `.a` or `.lib` spelling as a cross-platform API guarantee.

On Unix-like linkers, static archive ordering can matter. The consumer object normally precedes the SOAM archives, and dependency archives/libraries follow dependants as required by that toolchain.

## Compatibility and rebuild requirements

The development package does not promise ABI compatibility, object-layout stability, diagnostic stability, or cross-toolchain binary compatibility.

Consumers of the compiled static runtimes must rebuild when changing any of the following:

- compiler or compiler major configuration;
- standard-library implementation;
- relevant build configuration or ABI-affecting flags;
- SOAM package build/revision.

The package version file's same-minor matching is a package-selection rule only. It is not evidence of ABI compatibility.

## Install-surface policy

The supported install surface is defined by exported targets plus the classifications in `docs/public-api-stability.md`.

Installation of a header does not by itself promote every declaration in that header to a supported extension point. In particular:

- `AdaptiveMesh::detail` declarations remain internal/unsupported;
- scenario support, scenario friendship, tests, benchmarks, experiments, and `src/detail` declarations remain internal/test-only;
- profile-only declarations remain build-owned configuration surfaces;
- experimental declarations remain experimental even when installed;
- compatibility-only declarations remain migration aids rather than canonical new API.

The current build installs the source `include/` directory together with generated normal runtime headers. That mechanism is an implementation detail of the current package layout; support is determined by the documented classification, not by textual visibility alone.

## Verification matrix

The repository verifies the following downstream paths:

| Path | Expected linkage | Verification intent |
| --- | --- | --- |
| Domain-only CMake consumer | `AdaptiveMesh::adaptive_mesh_domain` | Header/value surface works without runtime linkage |
| Normal runtime CMake consumer | `AdaptiveMesh::adaptive_mesh` | Installed compiled runtime resolves and runs |
| K12 live CMake consumer | `AdaptiveMesh::adaptive_mesh_k12_live` | Experimental companion resolves transitively through normal runtime |
| Manual installed runtime consumer | direct include path + installed runtime archive + thread dependency | Non-CMake runtime linkage remains viable on the CI toolchain |
| Install-tree audit | installed files and exported package | No dependence on source-tree-only paths |

These checks validate consumption mechanics. They do not create ABI guarantees or expand public API classifications.
