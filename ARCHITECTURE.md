# Self-Organizing Adaptive Mesh: Architecture Specification

**Version:** 1.1.0  
**Status:** Approved  
**Domain:** Distributed Adaptive Systems / Second-Order Cybernetics  

---

## 1. System Overview & Mathematical Core

Ця специфікація описує архітектуру **адаптивної самоорганізованої мережі (Self-Organizing Adaptive Mesh)**, що поєднує принципи класичної теорії керування, просторової дифузії та кібернетики другого порядку. 

Основна мета системи — **адаптивне утримання ідентичності**: здатність інтегрувати нові дані та змінювати внутрішні режими без втрати структурної цілісності.

### 1.1 Fundamental Robustness Law
Динамічна стійкість системи виражається як відношення здатності до адаптації до втрати системних інваріантів:

$$\text{Robustness} = \frac{\text{Adaptation Capacity}}{\text{Invariant Loss}}$$

* **Rigidity Boundary:** При $\text{Adaptation Capacity} \to 0$ система стає крихкою та руйнується при зовнішньому збуренні.
* **Chaos Boundary:** При $\text{Invariant Loss} \to \infty$ система втрачає ідентичність та розпадається (катастрофічне забування).

### 1.2 Master State Equation
Зміна стану $i$-го вузла у часі визначається дифузійним процесом із мультимасштабною фільтрацією:

$$S_{i}^{t+1} = S_{i}^{t} + \alpha_i \cdot \mathbf{M}(S_i^t) \sum_{j \in \text{Neighbors}} T_{ij}(C, d, \theta, \phi) \cdot (S_j^t - S_i^t) + \text{ExtSignal}_i$$

Де:
* $S_i^t$ — поточний стан $i$-го вузла.
* $\alpha_i$ — коефіцієнт локальної адаптивності.
* $\mathbf{M}(S_i^t)$ — матриця мета-оцінки значущості сигналу.
* $T_{ij}(\dots)$ — пропускна здатність адаптивного мосту.

---

## SOAM v2 Architecture Terminology and Core Boundary

**Documentation baseline date:** 2026-09-06

SOAM v2 treats the system as an invariant-guided decentralized adaptive
graph-control system. The architectural interpretation of the current core is:

**IdentityInvariant → local state/health → MetaEvaluator → bounded bridge-state
transition → spatial/resilience compatibility → hysteretic graph adaptation →
emergent topology.**

This statement establishes terminology and architectural direction. It does not
introduce mechanisms that are not already implemented in the current repository.

### Effective coupling

`SpatialBridge::getEffectiveCoupling()` is the canonical SOAM v2 name for the
existing validated graph-interaction coefficient.

The current expression remains:

`capacity * spatialAttenuation * orientationWeight`

where:

`spatialAttenuation = 1.0 / (1.0 + 0.1 * distance)`

This PR does not change that expression, its operand order, or its numeric types.

`SpatialBridge::getEffectiveTransmission()` remains a compatibility API and
delegates to `getEffectiveCoupling()`.

The diagnostic text associated with a non-finite computed result changes from
`effective transmission` to `effective coupling`. This is an observable
diagnostic-text change.

### Spatial affinity terminology

`spatialAffinity` is the SOAM v2 conceptual term for the spatial compatibility
factor currently stored in `SpatialBridge::orientationWeight`.

`orientationWeight` is the current compatibility storage name for the concept
called `spatialAffinity` in SOAM v2 terminology.

PR-1 does not rename that field, add an alias or reference member, duplicate
storage, add migration getters/setters, or change object layout.

### SOAM Core boundary

SOAM Core remains transport- and waveform-agnostic. Core graph adaptation must
not depend on a particular packet-routing protocol, radio waveform, PHY, MAC,
baseband implementation, or proprietary forwarding mechanism.

The following mechanism classes are outside SOAM Core unless separately
reviewed for architecture and intellectual-property implications:

- packet backtracking;
- barrage or cooperative RF relay mechanisms;
- proprietary multicast forwarding schemes;
- TDMA/TOA synchronization mechanisms;
- distributed beamforming;
- MIMO or baseband signal processing;
- transmit-power control;
- dynamic spectrum control;
- other PHY/MAC-specific control mechanisms.

Transport-, waveform-, RF-, MAC-, and PHY-specific integration belongs below a
strict adapter boundary rather than inside the invariant-guided graph-control
core.

This boundary is an engineering architecture constraint. It is not a patent
clearance conclusion.

### InteractionObservation

`AdaptiveMesh::InteractionObservation` is a standalone,
construction-validated value type with a read-only public interface for one
non-temporal interaction
compatibility value. Its sole field is `compatibility`, constrained to the
finite range `[0.0, 1.0]`.

The type is an infrastructure primitive only. It is not stored in
`SpatialBridge`, does not participate in simulation or topology updates, and
does not encode history, confidence, persistence, trend, or hysteresis.
