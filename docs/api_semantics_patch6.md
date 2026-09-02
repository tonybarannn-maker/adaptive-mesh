Patch 6 establishes `SpatialAdaptiveMesh::simulationStep()` as the canonical blocking API while retaining `simulationStepAsync()` as a compatibility wrapper.

A true non-blocking API is deferred until object lifetime, cancellation, overlap, and asynchronous error propagation are specified and tested together.
