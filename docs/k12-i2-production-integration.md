# K12-I2 Production Integration — D1–D6 Safe Partition

## Status

This implementation contains only the authorized non-D7 slice. It adds a
library-owned live evaluator binding, lifetime-safe evaluation leases,
invalidation-and-drain semantics, and private complete-state change detection
for `BridgePersistence`.

Production observation, confidence, adaptive evidence production, persistence
updates from adaptive evidence, and BM-2 floor materialization are absent.
The D7 floor-design branch remains evidence-blocked.

## Lifetime contract

```text
ProductionTransitionEvaluationBinding
-> BindingState
-> BindingHandle
-> EvaluationLease
-> backend for one active evaluation
```

Invalidation makes new lease acquisition fail immediately. Existing leases keep
backend storage alive, and drain completes only after every active lease is
released. Evaluators never dereference a destroyed binding merely to determine
that it has expired.

Binding validity is not production-state currentness. A valid lease protects
storage only; normal snapshot and final-revalidation rules remain responsible
for context currentness.

## Live mesh boundary

`SpatialAdaptiveMesh` owns the production binding and issues evaluators without
accepting a caller-supplied backend. The current live adapter intentionally
resolves directed production relationship lifecycle records and captures their
production-owned node-incarnation, relationship-generation, transition-domain,
and context-lineage provenance under the mesh coherence lock. Numeric node IDs
remain descriptive; the private incarnation values establish lifecycle
continuity. Opposite relationship directions receive distinct generations.

The adapter intentionally reports request-direction provenance as unavailable
because D7 production observation and confidence mechanisms are not authorized.

Therefore the live result is fail-closed:

```text
live mesh
-> lifetime-safe backend lease
-> D7 provenance unavailable
-> not_eligible
-> STOP
```

It creates no permission, authority, `BridgeTransitionIntent`, execution path,
or bridge/node/topology mutation.

## Persistence instrumentation

The internal persistence seam compares the complete authority-relevant private
state before and after one evolution operation. It reports changed only when
recommendation, pending direction, or consecutive-sample state changed.

No pending-state field becomes public. The evaluator does not call `observe()`
or `reset()`, and the instrumentation does not grant persistence-updater,
validation, eligibility, or transition authority.

## Deterministic verification

The concurrency test acquires one lease, starts invalidation, waits until
invalidation is observably published, proves new acquisition fails, proves drain
cannot finish while the old lease exists, releases it, and then proves drain and
backend destruction complete safely. It uses condition-variable coordination
and thread joining, with no sleeps or scheduler-timing assertions.

Compile-time checks preserve the public locator/result/evaluate contract and
verify that supported public code cannot construct an evaluator from a backend
or construct the internal binding/handle.

## Qualification

This is a structural C++ misuse boundary, not a security boundary against
arbitrary hostile code in the same process. It establishes neither transition
authority nor system security, novelty, patentability, freedom to operate, or
non-infringement.
