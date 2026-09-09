# SOAM Invention and Provenance Log

**Baseline documentation date:** 2026-09-06
**Status:** Engineering provenance record

## Purpose

This file records repository provenance for SOAM concepts, mechanisms,
experiments, benchmarks, and design decisions.

It must distinguish:

- date of documentation;
- verified repository provenance;
- claimed invention date, if independently evidenced;
- unknown or unverified history.

A documentation date must not be represented as an invention date.

## Current baseline

### IdentityInvariant

- Documentation date: 2026-09-06
- Current repository concept: bounded identity reference used by node-state
  evaluation and health logic.
- Current baseline value: `1.6180339887`
- Known first commit/PR: not yet verified
- Provenance status: existing repository mechanism; historical origin requires
  repository-history verification.
- Claimed invention date: TBD

### MetaEvaluator

- Documentation date: 2026-09-06
- Current repository concept: local classification of candidate state relative
  to node health and `IdentityInvariant`.
- Known first commit/PR: not yet verified
- Provenance status: existing repository mechanism; historical origin requires
  repository-history verification.
- Claimed invention date: TBD

### SpatialBridge bounded state transition

- Documentation date: 2026-09-06
- Current repository concept: bridge capacity/status transition driven by the
  locally evaluated signal category.
- Known first commit/PR: not yet verified
- Provenance status: existing repository mechanism; historical origin requires
  repository-history verification.
- Claimed invention date: TBD

### Effective coupling

- Documentation date: 2026-09-06
- SOAM v2 canonical terminology: `effective coupling`
- Compatibility terminology: `effective transmission`
- Current expression:
  `capacity * spatialAttenuation * orientationWeight`
- Expression provenance: existing repository implementation predating this
  terminology cleanup.
- Known first commit containing the expression: not yet verified
- Provenance status: terminology documented; historical implementation origin
  remains to be verified.
- Claimed invention date: TBD

### spatialAffinity terminology

- Documentation date: 2026-09-06
- SOAM v2 conceptual name: `spatialAffinity`
- Current compatibility storage name: `orientationWeight`
- Storage/layout migration: none in PR-1
- Known first commit/PR for the underlying field: not yet verified
- Provenance status: terminology mapping documented; historical origin remains
  to be verified.
- Claimed invention date: TBD

## Evidence policy

Future entries should reference reproducible evidence where available, including:

- commit SHA;
- pull request;
- dated design record;
- benchmark artifact;
- test artifact;
- experiment inputs and outputs.

Unknown data must remain `TBD` or `not yet verified` until evidence is available.

### InteractionObservation

- Documentation date: 2026-09-06
- Role: infrastructure primitive / construction-validated representation
- Semantics: one non-temporal finite compatibility value in `[0.0, 1.0]`
- Known first implementation commit: `245178cfefac285a0153ab3134bb457b114fd7ea`
- First pull request: `#26`
- Provenance status: new repository type introduced by PR-2; historical
  provenance review remains separate
- Claimed invention date: TBD

### BridgeConfidence

- Documentation date: 2026-09-06
- Role: infrastructure/domain primitive / construction-validated representation
- Semantics: one non-temporal finite normalized confidence value in `[0.0, 1.0]`
- Estimation semantics: none; confidence estimation remains a future boundary
  producer responsibility
- Known first implementation commit: `b61f3494a04296052152f720f1fd4ca3e97ec1ff`
- First pull request: `#27`
- Provenance status: scalar domain primitive; not a standalone invention or
  novel mechanism
- Claimed invention date: TBD

### AdaptiveBridgePolicy and BridgePolicyEvidence

- Documentation date: 2026-09-06
- Role: infrastructure/domain primitive and stateless deterministic evaluator
- Formula: `E = (2C - 1)Q`, with evidence bounded to `[-1.0, 1.0]`
- Estimation, persistence, recommendation, and transition semantics: none
- Known first implementation commit: `e13e9ed345b769013f8e1c4248b0080921a4ded3`
- First pull request: `#28`
- Provenance status: bounded evidence representation; not a standalone
  invention or novel mechanism
- Claimed invention date: TBD

### BridgePersistence

- Documentation date: 2026-09-06
- Role: caller-owned infrastructure/control primitive
- Input: typed `BridgePolicyEvidence` only
- Semantics: state-relative activation/release persistence with saturating
  consecutive-sample counters and typed `PRESERVE`, `CONSTRAIN`, `SUPPORT`
  output
- Integration semantics: none; no bridge mutation, topology integration,
  timestamps, timers, smoothing, adaptive thresholds, or transport/RF/PHY/MAC
  inputs
- Known first implementation commit: `8fc804fd112c634cd94a9ef207545c5440f47dea`
- First pull request: `#29`
- Provenance status: generic persistence gate; not a standalone invention or
  novel mechanism
- Claimed invention date: TBD

### Bridge transition authorization seam

- Documentation date: 2026-09-06
- Role: standalone authorization infrastructure primitive
- Input: persistent recommendation plus explicitly resolved directional
  permissions
- Semantics: fail-closed, direction-preserving intent authorization only
- Integration semantics: no permission producer, bridge/status mutation,
  simulation, topology, routing, transport, or RF/PHY/MAC behavior
- Provenance status: authorization seam; no novelty or patentability claim
- Claimed invention date: TBD

### Bridge transition authorization implementation

- Known first implementation commit: `6aceed3c2d996bb9436c83e9f41b6732e8c03f24`
- First pull request: `#30`
- Claimed invention date: TBD

### K11 Production Transition Eligibility boundary

- Documentation date: 2026-09-08
- Canonical implementation baseline:
  `0083c69647129876f467fca52e8652f01f3670bd`
- Design lineage: K11-D1 repository/API audit; K11-D2 authority-boundary
  threat model; K11-D3 eligibility API proposal; K11-D3.1 binding/evidence
  producer contract; K11-D3.2 ownership/versioning review; K11-D4 API freeze.
- Role: eligibility-only boundary between validated production prerequisite
  evidence and any future transition-authority consideration.
- Central separation:
  recommendation != permission != prerequisite evidence != eligibility !=
  authority != mutation.
- Restricted-origin design: public K11 types expose inspection/evaluation
  semantics without a supported public evidence/request minting API.
- Test construction: synthetic privileged fixture access is confined to
  `tests/internal/production_transition_eligibility_test_access.hpp`; it is not
  a production evidence producer.
- Diagnostic semantics: deterministic fail-closed precedence with all
  context-component mismatches represented publicly as `binding_mismatch`.
- Integration semantics: no authority object, executor, topology/state
  mutation, KIKO-to-SOAM-Core coupling, K9 input, or K10 shadow promotion.
- Evidence status: correctness, compile-separation, isolation, and regression
  evidence must be captured before commit authorization.
- First implementation commit: `040e3cf4da1a090523d86920798c70c49166cad4`
- First pull request: TBD
- Originality/prior-art review: preliminary review complete; see K11
  preliminary IP gate status below.
- Preliminary FTO/design-around review: preliminary pass with mandatory
  design-around constraints; professional FTO not performed.
- Patentability/non-infringement claim: none.

### K11 — Preliminary IP gate status

- K11-IP1 = preliminary originality/prior-art review complete
- K11-IP2 = preliminary pass with mandatory design-around constraints
- individual primitives = crowded/prior-art-adjacent
- full composition = not found in this preliminary pass
- novelty = not established
- professional FTO = not performed
- claimed invention date = TBD

The preliminary review found no complete literal mapping in the reviewed
active US independent claims.

This is a preliminary search result only. It does not establish novelty,
non-obviousness, patentability, validity, freedom-to-operate,
non-infringement, or commercialization clearance.

Mandatory K11 design-around constraints DA-1 through DA-12 are recorded in
`docs/k11-production-transition-eligibility.md`.

The boundary remains:

recommendation != permission != prerequisite evidence != eligibility !=
transition authority != mutation

In particular, K11 ends at:

`eligible_for_authority_consideration -> STOP`

Any future eligibility-to-authority, credential, capability, execution, or
mutation mechanism requires a new architecture/IP/FTO gate before
implementation.

### K12 Production Prerequisite Producer / Evaluation Boundary

- Documentation date: 2026-09-08
- Canonical implementation baseline:
  `1c3b81821deae0cdb5114bce9db89777909a477d`
- Design lineage: K12-D1, K12-D1.1/F1-F3, K12-D1.2/F4-F5, K12-D2,
  K12-D2.1, and K12-CA1.
- Public subject model: descriptive directed endpoint locator only.
- Public result model: `no_request`, `not_eligible`,
  `eligible_for_authority_consideration`.
- State/version rule: snapshot ownership exclusively originates production
  context/version; prerequisite producers consume the exact captured version.
- Lifecycle rule: relationship generation identifies one continuous directed
  relationship incarnation; same endpoint values do not revive an old
  generation.
- Anti-resurrection rule: an authority-relevant state change advances lineage
  even when externally visible values later return to an earlier value.
- Construction topology: one complete library-owned
  `detail::ProductionTransitionConstructionAccess` materializes K11
  restricted-origin values from matching typed validation results.
- Construction/validation separation: construction privilege does not grant
  validation authority; there is no generic `kind + bool` evidence minting API.
- Revalidation rule: stale context destroys the logical attempt; old evidence
  is not combined with a new revalidation result.
- K12-I1 integration status: header-only production-evaluation semantics with
  synthetic test-owned state backend; no live `SpatialAdaptiveMesh`
  production-state attachment.
- Authority semantics: none. Positive K12 output is the existing K11
  eligibility classification followed by STOP.
- Mutation semantics: none. K12-I1 creates no topology, bridge, node-state, or
  execution path.
- KIKO/K9/K10 authority promotion: none.
- First implementation commit: `9e4bb77bd46ebbf16f98965e68805833940ca242`.
- First pull request: TBD.
- Originality/prior-art review: required as a separate K12 gate.
- Preliminary FTO/design-around review: required as a separate K12 gate.
- Patentability/non-infringement/FTO claim: none.

### K12 — Preliminary IP gate status

- K12-IP1 = CLOSED — PRELIMINARY
- K12-IP2 = PRELIMINARY PASS WITH MANDATORY DESIGN-AROUND CONSTRAINTS
- novelty = NOT ESTABLISHED
- patentability = NO CONCLUSION
- professional FTO = NOT PERFORMED
- non-infringement conclusion = NONE
- claimed invention date = TBD

These statuses record preliminary engineering/IP review gates only. They do not
establish novelty, non-obviousness, patentability, validity,
freedom-to-operate, non-infringement, or commercialization clearance.

The K12 authority firewall remains mandatory:

validator result != authorization decision

construction privilege != validation authority

evidence != token/capability/credential

final revalidation != authorization grant

K11/K12 positive result != BridgeTransitionIntent

eligibility -> STOP

K12 must not introduce or imply cryptographic signing, quorum endorsement,
threshold authorization, distributed authority issuance, distributed
token/credential issuance, or aggregate authority generation.

Any future eligibility-to-authority, credential, capability, execution, or
mutation mechanism requires a new architecture/originality/FTO gate before
implementation.

### K12-I2 — Production integration, D1–D6 safe partition

- Documentation date: 2026-09-09
- Baseline: `525d9f44cf93d3f64082279e5c94e7461e322048`
- Scope: library-owned evaluator binding, lifetime-aware handle and evaluation
  lease, invalidate-and-drain ordering, private complete-state persistence
  change detection, and deterministic verification.
- D7 status: evidence-blocked; no production observation/confidence producer,
  BM-2 floor, adaptive evidence updater, or adaptive recommendation derivation
  is implemented.
- Live behavior while D7 provenance is unavailable: fail closed to
  `not_eligible`, then `STOP`.
- Authority boundary: binding lifetime is not state currentness; persistence
  change detection is not persistence updater authority; eligibility is not
  permission, intent, execution, or mutation.
- Originality, patentability, FTO, and non-infringement conclusions: none added
  by this implementation slice.

### K12-I2-R1 — Persistence snapshot/revalidation remediation

- Documentation date: 2026-09-09
- Scope: complete authority-relevant persistence-state capture, separate
  non-reusing persistence lineage capture, coherent final comparison, and
  deterministic anti-resurrection/isolation/concurrency verification.
- Persistence state remains private and observational; revalidation does not
  update persistence or create authority.
- D7 remains evidence-blocked. No observation/confidence producer, adaptive
  evidence path, authority, intent, execution, or mutation mechanism is added.
