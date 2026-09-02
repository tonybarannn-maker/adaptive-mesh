# API Hardening

## Compatibility boundary

`AutopoieticNode` and `SpatialBridge` currently expose their data members publicly for backward compatibility. Existing callers may therefore still mutate fields directly, bypassing validation and synchronization conventions.

New integrations should prefer the helpers in `include/api_access.hpp`:

- `setNodeState()` validates finite state and serializes the mutation with `nodeMutex`.
- `setNodeHealthIndex()` validates the required `[0, 1]` range and serializes the mutation.
- `setNodePosition()` validates all coordinates before mutation.
- `snapshotNode()` returns a validated, immutable value snapshot for observation.
- `setBridgeCapacity()` validates the `[0, 1]` capacity range.
- `setBridgeGeometry()` validates target ID, distance, and orientation weight.
- `validateBridge()` applies the bridge transmission invariants.

These helpers are deliberately non-breaking: the existing public members remain available in the 1.x API. They establish a migration path toward private representation in a future major version.

Direct mutation of `bridges` remains a topology-level operation and should not be performed concurrently with mesh simulation or topology management. Use `SpatialAdaptiveMesh` topology methods for structural changes.

## Future major-version direction

A future major release may make node and bridge representation private, expose read-only views/snapshots, and move all validated mutations behind member functions. Such a change should be accompanied by a migration guide and a major SemVer version bump.
