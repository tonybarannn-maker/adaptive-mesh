# API Semantics

## Simulation steps

`SpatialAdaptiveMesh::simulationStep()` is the canonical blocking API. It returns only after one complete simulation step has finished and the resulting node and bridge state has been committed and validated.

`SpatialAdaptiveMesh::simulationStepAsync()` is retained as a compatibility wrapper for the 1.1 API. Despite its historical name, it is synchronous and blocks until the same simulation step is complete. New code should call `simulationStep()` to make this behavior explicit.

A true non-blocking future-based API is intentionally not introduced in this patch. A safe asynchronous API must define object lifetime, cancellation, overlap, and error propagation semantics before exposing background work to callers.

## Ownership and lifecycle

`SpatialAdaptiveMesh` owns its nodes, topology, reusable simulation buffers, and worker pool. Node and bridge storage is an implementation detail of the mesh; callers interact with that state through the mesh API.

The mesh is intentionally non-copyable and non-movable because its worker pool and synchronization state are tied to the owning object instance. Destroying a mesh stops the worker pool and joins its worker threads, so destruction can block until active worker callbacks finish.

`AutopoieticNode` and `SpatialBridge` remain public value types for compatibility with the current 1.1 API. Their mutable public fields are therefore not yet a stable encapsulation boundary. A future breaking API revision may make their state private and expose validated accessors/mutators instead.

The current lifecycle contract permits only one owning mesh instance per object; callers should finish or destroy a mesh before allowing its associated worker pool to be torn down.
