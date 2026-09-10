# Production mutation surface

This inventory follows the S3 audit baseline
`ca0e39d4a5ea032194c35d34f56440ea05c8de46` and its exception-safety remediation.
Stability classifications remain defined in [public-api-stability.md](public-api-stability.md).
It introduces no authority, execution, new getter, or public mutation entry point.

## Ownership and reachability

`SpatialAdaptiveMesh` owns its nodes, directed bridges, lifecycle records,
alpha, buffers and workers through its private implementation. Its supported
API returns scalar observations and an evaluator, not mutable references to
its nodes or bridges. `api_access.hpp` helpers operate on the object supplied
by the caller. Standalone nodes and bridges are caller-owned even though their
methods are implemented in the runtime library. Helpers cannot insert those
objects into, or obtain references from, a mesh.

Direct mutation changes the owner's state explicitly; derived mutation is an
indirect effect of a supported operation; lifecycle invalidation retires or
drains an existing lifecycle. A single operation may have several effects.

| Entry point | Owner and mutation effects | Validation / synchronization | Observation |
| --- | --- | --- | --- |
| Mesh constructor | New binding and fixed worker limit | Construction before publication; allocation may fail | Object construction |
| addNode | Mesh: direct node insertion; derived incarnation and dirty flags | Finite geometry/baseline, sequential ID; exclusive topology lock | State, health, bridge-count getters |
| connectNodes / connectNodePairs | Mesh: direct paired edges; derived lifecycle, alpha and flags | IDs, duplicates, all geometry; exclusive topology lock | Bridge counts |
| autoConnectNearbyNodes | Mesh: derived paired edges, lifecycle, alpha and flags | Nonnegative finite radius and distances; exclusive topology lock | Bridge counts |
| enforceStabilityCondition | Mesh: direct alpha and validation flag | Exclusive topology lock; noexcept | Indirect simulation behavior |
| pruneIsolatedBridges | Mesh: direct paired removal; derived alpha; lifecycle retirement | Finite nonnegative threshold; candidate discovery before erase; exclusive lock | Counts and evaluator outcomes |
| injectExternalShock | Mesh: direct filtered state; derived health | Finite magnitude/result and valid ID; exclusive lock | State/health getters |
| simulationStep | Mesh: derived state, health, capacity/status; worker/buffer bookkeeping | Topology/dynamic checks; buffered computation; exclusive topology lock and work mutex | State/health/count getters |
| simulationStepAsync | Same effects as simulationStep | Blocking compatibility wrapper | Same getters |
| Mesh destructor | Binding invalidation/drain, worker join, storage retirement | Binding mutex/CV, then worker stop/join | Completion; expired evaluator result |
| productionTransitionEvaluator | Shared handle ownership | Caller must keep mesh alive during acquisition | Evaluator |
| Evaluator evaluate | Lease count bookkeeping, observational backend reads | Binding mutex; shared topology locks | Evaluation enum; no domain mutation |
| Evaluator destruction | Handle ownership release | Does not invalidate the mesh binding | No domain effect |
| api node setters | Supplied node: direct field updates | Numeric checks, nodeMutex, atomic state/health stores | Fields or snapshotNode |
| api bridge setters | Supplied bridge: direct field updates | Numeric checks; no internal lock | Fields |
| Public node/bridge/position/invariant fields | Caller-owned direct writes | Caller validation and synchronization | Fields |
| Node copy constructor | New caller-owned copy | Source must not race with non-atomic writes; no source lock | New object |
| Node updateHealth | Supplied node: derived health | Validates invariant/state; atomic store, no nodeMutex | healthIndex |
| Bridge updateBridgeState | Supplied bridge: capacity/status; derived use inside simulation | Assumes valid bridge state; no internal lock | Fields |
| BridgePersistence observe/reset | Caller-owned persistence state | Validated construction, bounded counters; external synchronization | Recommendation from observe |
| Value construction/copy/assignment and prerequisite optionals | Caller-owned value replacement | Type-specific validation; external synchronization | Value accessors |
| Mesh getters | Read-only | Shared topology lock, bounds checks | Scalars |
| snapshotNode / validateBridge | Read-only | Node mutex for snapshot; no bridge lock | Snapshot or validation exception |
| Geometry/invariant/reflex queries and policy/eligibility evaluate | Read-only or value result | Type-specific validation | Result; no mesh execution |

Normal headers and the three production targets are installed/exported according
to CMake. Profile/scenario support and private implementation headers are not
supported installed mutation entry points. Internal persistence evolution is
not called by the current live evaluator. D7 direction provenance remains
unavailable. F02 lineage semantics are outside this remediation.

## Failure semantics

`addNode`, `connectNodes`, `connectNodePairs`, and `autoConnectNearbyNodes`
provide a logical strong exception guarantee. On failure, node/bridge contents,
lifecycle records/counters and alpha retain their prior values; buffer and
validation flags remain coherent. Capacity or hash bucket allocation may change.
For batch/automatic connection this covers the entire call.

Preparation holds the exclusive topology lock. All geometry, adjacency reserves
and potentially allocating lifecycle insertions precede bridge publication.
Provisional lifecycle insertions are invisible to other operations and are erased
on exception. Identity counters are staged locally. Publication appends trivial
bridge values into reserved capacity and updates counters, alpha and flags.
Node insertion reserves incarnation storage before inserting the copyable node;
the subsequent incarnation append cannot allocate.

Simulation validates before computation and uses temporary result buffers. Worker
computation exceptions propagate before domain commit; internal workers/buffers
may already have been prepared. Successful publication uses validated candidate
states and non-throwing stores. This is not a guarantee that arbitrary caller
mutation of standalone objects preserves their invariants.

## Synchronization and lifetime

Node helper locking coordinates only with operations using that same nodeMutex.
Public field writes, the node copy constructor and updateHealth do not acquire
it automatically. Individual atomic fields do not make a compound snapshot or
copy atomic. Bridge helpers, bridge state updates and caller-owned persistence
require caller synchronization for shared use. setNodeState does not recompute
health; setNodePosition does not rebuild existing bridge geometry; capacity and
status setters are not a coherent pair update.

Mesh public operations use the topology lock, but separate getter calls do not
constitute one atomic compound snapshot. The caller must prevent ordinary member
calls from overlapping object destruction, including evaluator acquisition.
An already-acquired evaluator uses independent binding state: invalidation stops
new leases and drains existing leases before mesh backend storage is retired.
This protection is not a general concurrent-destruction contract for the mesh.
Mutex failures inside noexcept operations can terminate; there is no promise of
recovery from synchronization primitive failure.

## Observability and audit disposition

No normal public mesh getter exposes capacity/status, alpha or private lifecycle
records. This is a documented limitation, not a promise to expand the API.

MUT-001 and MUT-002 are addressed by transaction preparation/publication and
numeric/allocation-failure regression tests. Acceptance still requires running
those tests on the candidate and reviewing the private lifecycle publication.
MUT-003 is addressed by explicit caller ownership. MUT-004 records per-operation
synchronization, lifetime and failure obligations. MUT-005 adds failure-path
coverage and retains the limited observation surface. Public observations do not
prove all private lifecycle invariants; source review remains necessary.
