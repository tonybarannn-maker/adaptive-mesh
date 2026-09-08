#include "production_transition_eligibility.hpp"

#include "bridge_transition_authorization.hpp"

#include <type_traits>

using namespace AdaptiveMesh;

static_assert(
    !std::is_default_constructible_v<
        ProductionRelationshipIdentity>);

static_assert(
    !std::is_constructible_v<
        ProductionRelationshipIdentity,
        std::size_t,
        std::size_t,
        std::uint64_t>);

static_assert(
    !std::is_default_constructible_v<
        ProductionStateVersion>);

static_assert(
    !std::is_constructible_v<
        ProductionStateVersion,
        std::uint64_t>);

static_assert(
    !std::is_default_constructible_v<
        ProductionTransitionClassId>);

static_assert(
    !std::is_constructible_v<
        ProductionTransitionClassId,
        std::uint64_t>);

static_assert(
    !std::is_default_constructible_v<
        ProductionTransitionRequestBinding>);

static_assert(
    !std::is_constructible_v<
        PermissionPrerequisiteEvidence,
        bool>);

static_assert(
    !std::is_constructible_v<
        InvariantPrerequisiteEvidence,
        bool>);

static_assert(
    !std::is_constructible_v<
        ResiliencePrerequisiteEvidence,
        bool>);

static_assert(
    !std::is_constructible_v<
        FreshnessPrerequisiteEvidence,
        bool>);

static_assert(
    !std::is_constructible_v<
        RevalidationPrerequisiteEvidence,
        bool>);

static_assert(
    !std::is_constructible_v<
        PermissionPrerequisiteEvidence,
        BridgeTransitionPermissions>);

static_assert(
    !std::is_convertible_v<
        ProductionTransitionEligibilityDecision,
        BridgeTransitionPermissions>);

static_assert(
    !std::is_convertible_v<
        ProductionTransitionEligibilityDecision,
        BridgeTransitionIntent>);

static_assert(
    noexcept(
        std::declval<const ProductionTransitionEligibilityEvaluator&>()
            .evaluate(
                std::declval<
                    const ProductionTransitionRequestBinding&>(),
                std::declval<
                    const ProductionTransitionPrerequisiteSet&>())));

int main() {
    return 0;
}
