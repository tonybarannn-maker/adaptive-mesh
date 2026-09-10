# Public API stability

## Status and interpretation

This document classifies the installed SOAM `2.0.0 development` surface for a
developer preview. The source-tree version is not a published `2.0.0` release.
Installation or textual visibility does not by itself make a declaration a
supported extension point.

The classifications are:

- **stable preview** — supported for developer-preview use. Incompatible
  source or documented-behavior changes require explicit review, migration
  guidance, and version treatment;
- **experimental** — consumable but subject to incompatible revision;
- **compatibility-only** — retained for migration to a canonical replacement;
- **internal/test-only** — unsupported for consumer use, even when a name is
  textually visible in an installed header.

Deprecation is a separate explicit status. A compatibility-only declaration is
not deprecated unless the documentation or declaration says so.

## Compatibility dimensions

Stable preview currently covers header availability, documented source use,
documented behavior, and exported target names. It does not guarantee ABI,
public object layout, diagnostic text, or cross-toolchain binary compatibility.
Consumers of the static runtimes must rebuild when changing compiler,
standard-library implementation, build configuration, or SOAM package build.

The CMake package version file uses same-minor matching. A package in the
`2.0.x` developer-preview line may satisfy a request in that line only when the
installed package is not older than the requested version. Different minor or
major lines are rejected. This is a package-selection rule only: it is neither
ABI evidence nor a release record.

## Exported targets

| Target | Classification | Contract |
| --- | --- | --- |
| `AdaptiveMesh::adaptive_mesh_domain` | stable preview | Header-defined domain and value API |
| `AdaptiveMesh::adaptive_mesh` | stable preview | Normal static runtime; no ABI guarantee |
| `AdaptiveMesh::adaptive_mesh_k12_live` | experimental | Live K12 evaluator companion |

## Installed headers

| Header | Classification | Notes |
| --- | --- | --- |
| `interaction_observation.hpp` | stable preview | Validated observation value |
| `bridge_confidence.hpp` | stable preview | Validated confidence value |
| `adaptive_bridge_policy.hpp` | stable preview | Policy and restricted-origin evidence |
| `bridge_persistence.hpp` | stable preview | Caller-owned persistence state machine |
| `bridge_transition_authorization.hpp` | stable preview | Permission-to-intent policy seam; no execution authority |
| `soam_domain.hpp` | stable preview | Domain umbrella header |
| `system_architecture.hpp` | mixed; see below | Generated normal runtime declarations |
| `api_access.hpp` | experimental | Validated mutation and snapshot helpers; pending mutation-surface audit |
| `production_transition_eligibility.hpp` | experimental | K12 production-evidence vocabulary and evaluator |
| `production_transition_evaluator.hpp` | mixed; see below | Generated K12 live evaluator declarations |

The build currently installs the source header directory as a whole. This
classification does not make that mechanism an install-policy guarantee;
explicit install-surface policy remains package/readiness work.

## Stable-preview symbol families

The following public type families, their documented public constructors and
methods, and their enum values are stable preview:

- `InteractionObservation`;
- `BridgeConfidence`;
- `BridgePolicyEvidence` and `AdaptiveBridgePolicy`;
- `PersistentBridgeRecommendation` and `BridgePersistence`;
- `BridgeTransitionPermissions`, `BridgeTransitionIntent`, and
  `BridgeTransitionAuthorizationPolicy`;
- `Vector3D`, `IdentityInvariant`, `SignalCategory`, `BridgeStatus`,
  `MetaEvaluator`, `SpatialBridge`, `AutopoieticNode`, and
  `SpatialAdaptiveMesh`, except for members classified separately below.

Public fields in these types remain part of the preview source surface. Their
memory layout, size, alignment, and binary representation are not guaranteed.

## Compatibility-only declarations

- `SpatialBridge::getEffectiveTransmission()` is retained as an alias for
  `getEffectiveCoupling()`;
- `SpatialAdaptiveMesh::simulationStepAsync()` is retained as a synchronous
  wrapper for `simulationStep()`.

These declarations are not currently deprecated. No removal version or removal
schedule is established by this classification.

## Experimental declarations

All public declarations in `AdaptiveMesh::api` are experimental, including
`NodeSnapshot`, the validated node mutation helpers, bridge mutation helpers,
snapshot access, and bridge validation.

The K12 eligibility family is experimental:

- `RequestedTransitionDirection`, `ProductionTransitionEligibility`, and
  `EligibilityRejectionReason`;
- production relationship, state-version, transition-class, request-binding,
  prerequisite-evidence, prerequisite-set, decision, and eligibility-evaluator
  types.

The K12 live evaluator family is experimental:

- `ProductionTransitionEvaluationLocator`;
- `ProductionTransitionEvaluation`;
- `ProductionTransitionEvaluator` and its `evaluate()` method;
- `SpatialAdaptiveMesh::productionTransitionEvaluator()`.

Eligibility for authority consideration is not authority, execution, mutation,
or permission to change topology.

## Internal and configuration-specific declarations

All declarations in `AdaptiveMesh::detail` are unsupported internal machinery,
including binding handles, binding access, construction access, backend,
lease, and binding-state declarations. Their textual presence in an installed
generated header does not make them public extension points.

`requireFinite()` is an internal runtime validation helper despite its current
namespace-level visibility and is not a supported consumer API.

`SOAM_PHASE_PROFILE_ENABLED`, `SimulationPhaseProfile`, and
`getLastSimulationPhaseProfile()` are build-owned profile configuration
surfaces, not supported normal consumer API. Scenario friendship,
scenario-generated declarations, scenario support, tests, benchmarks,
experiments, and `src/detail` declarations are internal/test-only and are not
installed or exported.

Consumer-defined macros cannot select profile or scenario access topology.

## Change policy

Changes to stable-preview source or documented behavior require explicit API
review and migration notes. Experimental declarations may change
incompatibly, but such changes must still be deliberate and documented.
Compatibility-only declarations require a separate deprecation decision before
any removal schedule is established. Internal/test-only declarations carry no
consumer compatibility promise.
