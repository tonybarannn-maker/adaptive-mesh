# SOAM 2.0 static-runtime migration

SOAM 2.0 separates its header-defined value/domain surface from production
runtime ownership.

## Targets

- `AdaptiveMesh::adaptive_mesh_domain` is the installed header-only domain target.
- `AdaptiveMesh::adaptive_mesh` is the installed static production runtime.
- `AdaptiveMesh::adaptive_mesh_k12_live` is the installed static K12 live companion.

Code that only uses observations, confidence, persistence, authorization value
types, or eligibility types should link `adaptive_mesh_domain`. Code that creates
`SpatialAdaptiveMesh`, nodes, or bridges must link `adaptive_mesh`. Code that
calls `productionTransitionEvaluator()` must link `adaptive_mesh_k12_live`.

The migration is source-compatible where verified but build/link breaking:
runtime consumers must link the corresponding compiled target and all consumers
must be rebuilt.

## Configuration universes

Normal, phase-profile, and scenario-test runtimes are separate build-owned
configurations. Their class declarations are generated as immutable build
artifacts. Consumer-local macros cannot add scenario friendship or change the
supported access topology. Consumer-local profiling macros are unsupported.
Scenario-test headers and symbols are not installed or exported.

`CITATION.cff` and `.zenodo.json` continue to describe the deposited 1.1.0
artifact. The source tree version identifies the 2.0.0 development line; this
does not publish or tag a 2.0.0 release.
