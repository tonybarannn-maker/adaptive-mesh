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
- Known first implementation commit/PR: TBD until the implementation commit is
  created
- Provenance status: scalar domain primitive; not a standalone invention or
  novel mechanism
- Claimed invention date: TBD
