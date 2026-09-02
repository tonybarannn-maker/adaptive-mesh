# API Semantics

## Simulation steps

`SpatialAdaptiveMesh::simulationStep()` is the canonical blocking API. It returns only after one complete simulation step has finished and the resulting node and bridge state has been committed and validated.

`SpatialAdaptiveMesh::simulationStepAsync()` is retained as a compatibility wrapper for the 1.1 API. Despite its historical name, it is synchronous and blocks until the same simulation step is complete. New code should call `simulationStep()` to make this behavior explicit.

A true non-blocking future-based API is intentionally not introduced yet. A safe asynchronous API must define object lifetime, cancellation, overlap, and error propagation semantics before exposing background work to callers.

## Ownership and encapsulation

`SpatialAdaptiveMesh` owns the node collection, topology, simulation buffers, worker pool, and synchronization state. These implementation details are intentionally private and are accessed through the mesh API rather than by callers.

`AutopoieticNode` and `SpatialBridge` remain public value/helper types in the 1.1 API for compatibility. Their data members are therefore not made private in this maintenance patch. A future major-version API can introduce immutable views or accessor-based wrappers without silently breaking existing source code.

The mesh is non-copyable and non-movable. Its destructor stops and joins the internal `std::jthread` worker pool, so destruction may block briefly while workers finish. Callers should therefore avoid destroying a mesh while application code still depends on an in-flight operation.

## Validation contract

Topology and numerical state are validated before and after each simulation step. Public mutation APIs reject invalid indices, non-finite numeric inputs, duplicate bridge pairs, and other states that would violate the mesh invariants.
