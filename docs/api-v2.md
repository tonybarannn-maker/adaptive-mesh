# API Semantics

## Ownership and lifecycle

`SpatialAdaptiveMesh` owns its nodes, topology, reusable simulation buffers, and worker pool. Node and bridge storage is an implementation detail of the mesh; callers interact with that state through the mesh API.

The mesh is intentionally non-copyable and non-movable because its worker pool and synchronization state are tied to the owning object instance. Destroying a mesh stops the worker pool and joins its worker threads, so destruction can block until active worker callbacks finish.

`AutopoieticNode` and `SpatialBridge` remain public value types for compatibility with the current 1.1 API. Their mutable public fields are therefore not yet a stable encapsulation boundary. A future breaking API revision may make their state private and expose validated accessors/mutators instead.
