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

## K11 preliminary IP review status and mandatory design-around constraints

The K11 originality/prior-art review and preliminary FTO/design-around review
are separate from the technical correctness gate.

Current review status:

- K11-IP1 = preliminary originality/prior-art review complete
- K11-IP2 = preliminary pass with mandatory design-around constraints

The following constraints are mandatory for the current K11 design and for
later work relying on this boundary.

### DA-1 — Eligibility remains eligibility only

`eligible_for_authority_consideration` remains an eligibility classification
only. It does not grant, represent, transfer, or imply transition authority.

Eligibility is not permission, authority, execution, or mutation.

### DA-2 — No token, credential, capability, or permission

K11 must not create or return an authorization token, credential, capability,
permission object, access grant, or equivalent authority-bearing artifact.

### DA-3 — No portable execution-authority bundle

The K11 result and prerequisite evidence must not form a portable,
transferable, replayable, or independently exercisable execution-authority
bundle.

### DA-4 — No access grant or operation authorization

A positive K11 result must not be interpreted as an access grant, operation
authorization, command authorization, or permission to perform a transition.

### DA-5 — No execution

K11 terminates after eligibility classification:

`eligibility -> STOP`

K11 contains no transition executor and performs no production actuation.

### DA-6 — No bridge, state, or topology mutation

K11 must not mutate a bridge, node state, topology, production relationship,
or other production control state.

### DA-7 — Freshness is not token TTL

K11 freshness expresses coherence with the relevant production state and
request context. Freshness must not be reinterpreted as token TTL, credential
expiration, lease duration, authorization lifetime, or bearer-token freshness.

### DA-8 — StateVersion is not historical access-control audit

`ProductionStateVersion` is an opaque equality-bound version for
authority-relevant production-state coherence. It must not become a historical
access-control audit sequence, authorization ledger, permission epoch, or
credential-revocation history.

### DA-9 — Evidence is not reusable authorization claims

K11 prerequisite evidence is context-bound prerequisite evidence only. It must
not become reusable authorization claims, bearer assertions, portable proofs
of authority, or independently exercisable credentials.

### DA-10 — No distributed token, signature, or consensus issuance

K11 must not expand into distributed issuance, signing, endorsement,
replication, quorum, consensus, or validation of authority-bearing tokens or
credentials without a separate architecture and IP review.

### DA-11 — KIKO or shadow evidence is not a production credential

KIKO, K9 interchange observations, K10 shadow decisions, shadow metrics,
correlation identifiers, and other experimental artifacts must not be promoted
into production credentials or evidence that independently grants production
authority.

Experimental information may inform separately validated production-boundary
inputs, but it does not cross the boundary as authority.

### DA-12 — Eligibility-to-authority expansion requires a new IP gate

Any future mechanism that converts K11 eligibility into transition authority,
permission, capability, credential, execution right, or mutation path requires
a new architecture/originality/FTO gate before implementation.

K11 itself ends before that boundary.

## Preliminary-review qualification

Preliminary review found no complete literal mapping in the reviewed active US
independent claims.

This does not establish FTO, non-infringement, patentability, validity, or
commercialization clearance.

Individual K11 primitives are in crowded or prior-art-adjacent areas. The full
K11 composition was not found in this preliminary pass. Novelty is not
established. A professional claim-level FTO review has not been performed.
