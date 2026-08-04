# Conceptual Framework & Cybernetic Definitions

> **Disclaimer:** Throughout this project, "autopoietic" and "second-order cybernetics" are used as **engineering metaphors and architectural heuristics** describing self-maintaining, reflective network organization. They should not be interpreted as claims of implementing biological autopoiesis in the strict sense of Maturana & Varela, nor as a complete theoretical realization of Von Foerster's second-order cybernetics.

## 1. "Autopoiesis" as an Engineering Metaphor

* **Strict Academic Definition (Maturana & Varela):** The capacity of a system to continually produce and maintain its own organizational boundary and components.
* **Engineering Interpretation in Adaptive Mesh:** Describes a distributed network that maintains its structural invariants ($\Omega$) and health parameters ($HealthIndex > 0$) without external centralized control, relying exclusively on localized damping and adaptive edge weight dynamics ($T_{ij}$).

## 2. Dynamic Reflection Layer (Inspired by Second-Order Cybernetics)

* **Theoretical Origin:** Von Foerster's cybernetics of observing systems, where the mechanism of observation becomes an explicit part of the system's dynamics.
* **Architectural Implementation:** Implemented via a **reflective control layer** (`MetaEvaluator`). The system measures not merely state differentials ($S_j - S_i$), but evaluates the *contextual significance* of deviations (differentiating Noise from Destructive Drift). The meta-observer modulates parameter capacity ($T_{ij}$), effectively establishing a second feedback loop over the primary graph diffusion process.
