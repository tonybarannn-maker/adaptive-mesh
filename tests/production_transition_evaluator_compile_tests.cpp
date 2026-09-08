#include "bridge_transition_authorization.hpp"
#include "production_transition_evaluator.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

using namespace AdaptiveMesh;

static_assert(std::is_aggregate_v<ProductionTransitionEvaluationLocator>);
static_assert(std::is_constructible_v<ProductionTransitionEvaluationLocator, std::size_t, std::size_t>);
static_assert(!std::is_default_constructible_v<ProductionTransitionEvaluator>);
static_assert(!std::is_copy_constructible_v<ProductionTransitionEvaluator>);
static_assert(!std::is_copy_assignable_v<ProductionTransitionEvaluator>);
static_assert(!std::is_constructible_v<ProductionRelationshipIdentity, std::size_t, std::size_t, std::uint64_t>);
static_assert(!std::is_constructible_v<ProductionStateVersion, std::uint64_t>);
static_assert(!std::is_constructible_v<ProductionTransitionClassId, std::uint64_t>);
static_assert(!std::is_default_constructible_v<ProductionTransitionRequestBinding>);
static_assert(!std::is_constructible_v<PermissionPrerequisiteEvidence, bool>);
static_assert(!std::is_constructible_v<InvariantPrerequisiteEvidence, bool>);
static_assert(!std::is_constructible_v<ResiliencePrerequisiteEvidence, bool>);
static_assert(!std::is_constructible_v<FreshnessPrerequisiteEvidence, bool>);
static_assert(!std::is_constructible_v<RevalidationPrerequisiteEvidence, bool>);
static_assert(!std::is_constructible_v<detail::PermissionValidationResult, bool, std::uint64_t>);
static_assert(!std::is_constructible_v<detail::InvariantValidationResult, bool, std::uint64_t>);
static_assert(!std::is_constructible_v<detail::ResilienceValidationResult, bool, std::uint64_t>);
static_assert(!std::is_constructible_v<detail::FreshnessValidationResult, bool, std::uint64_t>);
static_assert(!std::is_convertible_v<ProductionTransitionEvaluationLocator, ProductionRelationshipIdentity>);
static_assert(!std::is_convertible_v<ProductionTransitionEvaluationLocator, ProductionTransitionRequestBinding>);
static_assert(!std::is_convertible_v<ProductionTransitionEvaluation, BridgeTransitionPermissions>);
static_assert(!std::is_convertible_v<ProductionTransitionEvaluation, BridgeTransitionIntent>);
static_assert(std::is_same_v<decltype(std::declval<ProductionTransitionEvaluator&>().evaluate(std::declval<const ProductionTransitionEvaluationLocator&>())), ProductionTransitionEvaluation>);

int main() { return 0; }
