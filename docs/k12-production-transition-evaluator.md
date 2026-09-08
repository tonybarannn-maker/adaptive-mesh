# K12 Production Transition Evaluator

## Status

K12-I1 implements the frozen production prerequisite orchestration boundary as a header-only, eligibility-only component. It does not integrate with live `SpatialAdaptiveMesh` state. The production-state backend used by behavioral and concurrency tests is synthetic and test-owned. Live production integration remains a separate K12-I2 gate.

The terminal positive result remains:

```text
eligible_for_authority_consideration -> STOP
```

It does not grant transition authority and does not perform mutation.

## Public contract

The caller supplies only `ProductionTransitionEvaluationLocator{sourceNodeId, targetNodeId}`. The locator is descriptive only: it is not relationship identity, generation, state version, request binding, transition class, direction, permission, evidence, capability, or credential.

The public result is limited to `no_request`, `not_eligible`, and `eligible_for_authority_consideration`. `no_request` means authoritative resolution completed sufficiently to establish that no transition request currently exists, including confirmed relationship absence or production-derived `PRESERVE`. Resolution/capture/producer/revalidation failures and coherent K11 rejection map to `not_eligible`; detailed causes remain internal.

## Per-call semantics

Each call performs fresh resolution and capture:

```text
locator
-> current directed relationship
-> coherent relationship generation + state version
-> evaluator-owned transition class
-> production-native direction
-> permission validation
-> invariant validation
-> resilience validation
-> freshness validation
-> final revalidation
-> K11 eligibility
-> STOP
```

Reusing the same locator cannot reuse an old relationship generation, state version, request, or evidence.

## Construction privilege

K11 restricted-origin values have one production construction owner: `AdaptiveMesh::detail::ProductionTransitionConstructionAccess`. K11 public constructors remain unchanged; only private access topology is extended. The complete construction-access definition lives in `include/detail/production_transition_evaluation_internal.hpp`.

Construction privilege is separate from validation authority. Each prerequisite domain produces a distinct typed validation result. The construction access type can only materialize the matching K11 evidence. There is no generic `kind + bool` evidence materializer.

This is a C++ structural misuse barrier, not cryptographic or process-isolation security against hostile same-address-space code.

## Validation and revalidation

Permission, invariant, resilience, and freshness have three internal outcomes: `satisfied`, `not_satisfied`, and `unavailable_or_failed`. Validated negatives become negative K11 evidence and continue so K11 remains the canonical rejection classifier. `unavailable_or_failed` terminates fail-closed before K11.

Final revalidation is separate: `revalidated`, `stale`, or `unavailable_or_failed`. Only `revalidated` can materialize revalidation evidence. Stale context terminates the attempt; no S0 evidence is combined with a later context.

## Lifecycle/version rules

The snapshot owner alone originates the captured production state version. Validators consume that exact version and cannot create, advance, replace, or reinterpret it. Relationship generation identifies one continuous directed relationship incarnation; recreation of the same endpoint pair is a new generation. Relevant state changes advance lineage even when visible values later return to an earlier value, preventing resurrection by scalar equality. Changes explicitly outside the registered transition domain do not invalidate that domain version.

K12-I1 verifies these semantics with a deterministic synthetic state owner. It does not add lifecycle/version counters to live topology classes.

## Test separation and concurrency

`tests/internal/production_transition_evaluator_test_access.hpp` is synthetic test infrastructure only. Production K12 code does not include it. Existing K11 test access remains independent from production construction access.

Concurrency tests use synchronization barriers, not sleeps, to force changes between capture and final revalidation. They verify relevant-state staleness, irrelevant-domain stability, relationship reincarnation invalidation, and independent concurrent attempts. No automatic retry exists.

## Explicit exclusions

K12-I1 contains no transition authority, token/capability/credential, `BridgeTransitionIntent` creation, executor, bridge/node/topology mutation, live `SpatialAdaptiveMesh` integration, KIKO-to-SOAM-Core dependency, K9/K10 promotion path, or transport/RF/PHY/MAC mechanism.

Any future eligibility-to-authority or mutation mechanism requires a new architecture, originality/prior-art, and preliminary FTO/design-around gate.

## IP qualification

This document records engineering architecture and provenance boundaries. It does not establish novelty, patentability, freedom-to-operate, non-infringement, validity, or commercialization clearance.


## K12 preliminary IP gate status

The current preliminary gate status is:

```text
K12-IP1          = CLOSED — PRELIMINARY
K12-IP2          = PRELIMINARY PASS WITH MANDATORY DESIGN-AROUND CONSTRAINTS
novelty          = NOT ESTABLISHED
patentability    = NO CONCLUSION
professional FTO = NOT PERFORMED
non-infringement = NONE
```

These are preliminary review classifications only. They do not establish
novelty, non-obviousness, patentability, validity, freedom-to-operate,
non-infringement, or commercialization clearance.

### Mandatory authority design-around constraints

The production-evaluation boundary must preserve all of the following
separations:

```text
validator result             != authorization decision
construction privilege       != validation authority
evidence                     != token/capability/credential
final revalidation           != authorization grant
K11/K12 positive result      != BridgeTransitionIntent
eligibility                  -> STOP
```

Construction access may mechanically materialize a restricted K11 value only
from the matching typed, lineage-bound internal result. It must not determine
that a prerequisite is satisfied and must not create a generic evidence or
authority-minting path.

Final revalidation establishes only that the captured attempt remains coherent
with its production state/version domain. It does not authorize a transition,
grant permission, create a capability, or extend the lifetime of an eligibility
result.

A positive K11/K12 result remains an eligibility classification only. It is not
`BridgeTransitionIntent`, an execution command, a permission, an authority
object, or a mutation request.

### Distributed-authority exclusion

K12-I1 explicitly excludes:

- cryptographic signing;
- quorum endorsement;
- threshold authorization;
- distributed authority issuance;
- distributed token or credential issuance;
- aggregate authority generation.

These mechanisms must not be inferred from prerequisite validation,
construction privilege, revalidation, evidence aggregation, or a positive
eligibility result.

Any future mechanism crossing `eligibility -> STOP` requires a separate
architecture, originality/prior-art, and preliminary FTO/design-around gate
before implementation.
