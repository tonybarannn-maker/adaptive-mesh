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
