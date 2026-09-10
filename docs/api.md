# API Semantics

## Build and linkage contract

Value/domain-only consumers link `AdaptiveMesh::adaptive_mesh_domain`.
Consumers that construct `SpatialAdaptiveMesh`, `AutopoieticNode`, or
`SpatialBridge` link `AdaptiveMesh::adaptive_mesh`. Calling
`productionTransitionEvaluator()` additionally requires
`AdaptiveMesh::adaptive_mesh_k12_live`.

The supported public method signatures are preserved, but runtime linkage is a
breaking SOAM 2.0 build-contract change.

## Simulation steps

`SpatialAdaptiveMesh::simulationStep()` is the canonical blocking API. It returns only after one complete simulation step has finished and the resulting node and bridge state has been committed and validated.

`SpatialAdaptiveMesh::simulationStepAsync()` is retained as a compatibility wrapper for the 1.1 API. Despite its historical name, it is synchronous and blocks until the same simulation step is complete. New code should call `simulationStep()` to make this behavior explicit.

A true non-blocking future-based API is intentionally not introduced in this patch. A safe asynchronous API must define object lifetime, cancellation, overlap, and error propagation semantics before exposing background work to callers.

## Stability and compatibility

The canonical stability classification is defined in
[`public-api-stability.md`](public-api-stability.md). In summary:

- the core domain and normal runtime surfaces are stable preview;
- `api_access.hpp` and the K12 eligibility/evaluator surface are experimental;
- `SpatialBridge::getEffectiveTransmission()` and
  `SpatialAdaptiveMesh::simulationStepAsync()` are compatibility-only and are
  not currently deprecated;
- `AdaptiveMesh::detail` declarations and profile/scenario declarations are
  unsupported internal surfaces.

Stable preview is a source and documented-behavior commitment for the
developer-preview line. It is not an ABI or public object-layout guarantee.
Diagnostic text is not a compatibility surface.

The generated package-version file uses same-minor matching. A `2.0.x` package
may satisfy a request within the `2.0` developer-preview line when it is not
older than the requested version; different minor or major lines are rejected.
This package-selection rule does not establish ABI compatibility or publish a
release.
