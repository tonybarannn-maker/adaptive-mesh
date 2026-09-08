# K11 Production Transition Eligibility

K11 implements an eligibility-only boundary.

restricted-origin ProductionTransitionRequestBinding
+
five context-bound prerequisite evidence objects
        ↓
ProductionTransitionEligibilityEvaluator
        ↓
not_eligible
or
eligible_for_authority_consideration
        ↓
STOP

## Semantics

Eligibility is descriptive only.

recommendation
!= permission
!= prerequisite evidence
!= eligibility
!= transition authority
!= mutation

eligible_for_authority_consideration does not authorize a transition and
cannot be converted through the supported public API into
BridgeTransitionPermissions, BridgeTransitionIntent, an authority object,
a bridge-status mutation, a capacity delta, or a topology operation.

RequestedTransitionDirection contains only constrain and support.
PRESERVE does not produce an authority-consideration request.

## Restricted-origin construction

The public K11 surface exposes no supported producer/factory API for:

- relationship identities;
- production state versions;
- transition-class identities;
- request bindings;
- prerequisite evidence.

Their constructors are non-public.

The behavioral test target uses
tests/internal/production_transition_eligibility_test_access.hpp to create
synthetic restricted-origin fixtures. That helper is test infrastructure, not
a production evidence producer and not part of the supported public include
surface.

C++ private constructors, friend access, and non-public test helpers are
structural public-API misuse barriers. They are not a cryptographic or
process-isolation security boundary against hostile code executing in the
same address space.

The supported claim is:

unsupported through the public API

not that fabrication is impossible for arbitrary in-process code.

## Prerequisites

The evaluator consumes five independently represented evidence classes:

- permission;
- invariant;
- resilience;
- freshness;
- revalidation.

Every evidence value carries an exact
ProductionTransitionRequestBinding.

Binding equality includes:

- directed source/target relationship identity;
- relationship generation;
- requested direction;
- transition class;
- production state version.

Any mismatch maps publicly to
EligibilityRejectionReason::binding_mismatch.

Raw BridgeTransitionPermissions is not prerequisite evidence.

## Missing versus negative evidence

Absence is distinct from a present negative result.

missing invariant
→ missing_prerequisite

present invariant(false)
→ invariant_not_satisfied

Both classifications are fail-closed.

## Rejection precedence

The public rejection precedence is fixed:

1. invalid_input
2. missing_prerequisite
3. binding_mismatch
4. permission_not_satisfied
5. invariant_not_satisfied
6. resilience_not_satisfied
7. freshness_not_satisfied
8. revalidation_failed

invalid_input is retained by the frozen public contract but may be
unreachable through ordinary supported construction. Tests do not invent
malformed fixture objects solely to exercise it.

## Isolation

K11 does not implement:

- production context capture;
- relationship-generation registry;
- state-version registry;
- evidence producers against live production state;
- transition authority;
- executor;
- bridge/topology mutation;
- KIKO/K9 input;
- K10 shadow promotion.

The evaluator operates only on immutable K11 values.

The behavioral suite includes a supplemental before/after production-state
comparison to demonstrate that the tested evaluator path does not modify node
state, health, bridge count, target IDs, capacity, or bridge status.

This is a software-contract test, not a system-wide security claim.

## IP and provenance qualification

K11 remains transport- and waveform-agnostic. It introduces no routing,
cooperative RF relay, TDMA/TOA synchronization, beamforming, MIMO/baseband,
transmit-power, dynamic-spectrum, or other PHY/MAC control behavior.

Passing K11 tests does not establish patentability, freedom-to-operate,
non-infringement, system-wide security, or safe production actuation.