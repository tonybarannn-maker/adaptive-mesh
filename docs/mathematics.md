# Mathematical Foundations & Experimental Methodology

## 1. Дифузійний процес на графах
Динаміка стану вузла $i$ описується дискретним рівнянням дифузії:

$$S_i(t + \Delta t) = S_i(t) + \alpha \sum_{j \in N(i)} T_{ij}(t) \cdot (S_j(t) - S_i(t))$$

де:
* $S_i(t)$ — стан вузла $i$ в момент часу $t$.
* $T_{ij}(t) = \text{Capacity}_{ij} \cdot \text{Attenuation}(d_{ij}) \cdot \text{Orientation}_{ij}$ — ефективний коефіцієнт передачі мосту.
* $\alpha$ — глобальний коефіцієнт дифузії, обмежений умовою CFL: $\alpha \le \frac{0.8}{D_{\max}}$.

## 2. Метрики оцінки стійкості
1. **Recovery Time ($T_{rec}$):** Мінімальна кількість кроків $\Delta t$, за яку стан після збурення повертається у стабільну зону $|S_i - Baseline| \le \epsilon$.
2. **Isolation Ratio ($R_{iso}$):** Відношення кількості заблокованих мостов ($T_{ij} = 0$) до загальної кількості мостів у мережі.
3. **Health Recovery Index ($H_{sys}$):** Середнє значення `healthIndex` по всіх вузлах графа.

## 3. Candidate System Properties (Formal Framework)

### Assumption 1 (Bounded Edge Capacity & Symmetric Weights)
For all edges $(i, j) \in E$, transmission capacity is bounded ($0 \le T_{ij}(t) \le T_{\max} < \infty$) and dynamic edge weights maintain local symmetry ($T_{ij} = T_{ji}$).

### Candidate Proposition 1 (State Boundedness, under stated assumptions)
If initial node states are bounded such that $S_{\min}(0) \le S_i(0) \le S_{\max}(0)$ for all $i \in V$, and diffusion coefficient $\alpha$ satisfies the CFL condition $\alpha \le \frac{1}{D_{\max}}$, then for all $t > 0$:

$$\min_{k \in V} S_k(0) \le S_i(t) \le \max_{k \in V} S_k(0)$$

*Informal Argument / Rationale:* Under the CFL boundary condition, the discrete Laplace operator forms a convex combination of neighbor states at each integration step, preventing localized numerical overshoot outside the initial dynamic envelope.

### Conjecture (Mass Conservation under Conservative Conditions)
Provided that:
* The network topology graph $G(V, E)$ remains fixed (no dynamic edge insertion or pruning);
* Edge dynamic transmission weights are strictly symmetric ($T_{ij}(t) = T_{ji}(t)$);
* No external source, sink, or MetaEvaluator hard-resets are active;

the total system state is expected to remain conserved: $\sum_{i \in V} S_i(t) = \text{const}$.
