# SOAM Design-Around and IP Boundary Governance

**Baseline documentation date:** 2026-09-06
**Status:** Engineering governance document

## Legal-status limitation

This document is an engineering governance record.

It does not establish or imply:

- patentability;
- freedom to operate;
- non-infringement;
- validity or invalidity of any patent;
- legal clearance for commercialization.

Those questions require separate evidence and, where appropriate, claim-level
legal analysis.

## SOAM Core principle

SOAM Core is developed as an invariant-guided decentralized adaptive
graph-control system.

Core development should preserve the conceptual sequence:

**IdentityInvariant → local state/health → MetaEvaluator → bounded bridge-state
transition → spatial/resilience compatibility → hysteretic graph adaptation →
emergent topology.**

PR-1 establishes this terminology and governance boundary only. It does not
implement additional stages or future SOAM v2 mechanisms.

## Core exclusions

Without a separate architecture and IP review, SOAM Core must not incorporate:

- packet backtracking;
- barrage or cooperative RF relay;
- proprietary multicast forwarding;
- TDMA/TOA synchronization;
- distributed beamforming;
- MIMO/baseband processing;
- transmit-power control;
- dynamic spectrum control;
- equivalent PHY/MAC-specific mechanisms.

SOAM Core should remain transport- and waveform-agnostic.

Any future transport-, RF-, MAC-, PHY-, or waveform-specific integration must
remain separated from Core through an explicit adapter/IP boundary.

## Change-entry gates

A substantial new SOAM mechanism should not enter `main` until it has:

1. technical correctness evidence;
2. measurable and reproducible evidence appropriate to the change;
3. an originality/prior-art review;
4. a preliminary FTO/design-around review.

Performance-only changes must preserve semantics and include correctness tests
plus reproducible A/B evidence.

Architecture changes require an explicit design rationale and regression
coverage.

## PR-1 limitation

This baseline intentionally does not design or specify future SOAM v2
mechanisms. Their architecture, APIs, algorithms, thresholds, state models,
and implementation details remain outside the scope of this document and PR-1.

## PR-5 persistence boundary

Threshold and consecutive-sample persistence is a known control primitive and
is not treated as novel by itself.

PR-5 accepts only typed `BridgePolicyEvidence`. The persistence gate has no
routing, RF measurement, handover, or link enable/disable semantics, and does
not mutate bridge or topology state.

The reviewed US claims received only a preliminary pass. Foreign family
coverage, doctrine-of-equivalents analysis, and professional freedom-to-
operate review remain incomplete and unresolved.
