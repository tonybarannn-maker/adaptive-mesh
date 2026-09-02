# API Semantics

## Simulation steps

`SpatialAdaptiveMesh::simulationStep()` is the canonical blocking API. It returns only after one complete simulation step has finished and the resulting node and bridge state has been committed and validated.

`SpatialAdaptiveMesh::simulationStepAsync()` is retained as a compatibility wrapper for the 1.1 API. Despite its historical name, it is synchronous and blocks until the same simulation step is complete. New code should call `simulationStep()` to make this behavior explicit.

A true non-blocking future-based API is intentionally not introduced in this patch. A safe asynchronous API must define object lifetime, cancellation, overlap, and error propagation semantics before exposing background work to callers.
