#include "internal/production_transition_eligibility_test_access.hpp"
#include "system_architecture.hpp"

#include <cstdlib>
#include <cstddef>
#include <vector>

using namespace AdaptiveMesh;

namespace {

[[noreturn]] void failTest() noexcept {
    std::abort();
}

void require(bool condition) noexcept {
    if (!condition) {
        failTest();
    }
}


using TestAccess = ProductionTransitionEligibilityTestAccess;

ProductionTransitionRequestBinding makeRequest(
    RequestedTransitionDirection direction =
        RequestedTransitionDirection::constrain,
    std::uint64_t relationshipGeneration = 7,
    std::uint64_t transitionClass = 11,
    std::uint64_t stateVersion = 21) {
    return TestAccess::binding(
        100,
        200,
        relationshipGeneration,
        direction,
        transitionClass,
        stateVersion);
}

ProductionTransitionPrerequisiteSet allSatisfied(
    const ProductionTransitionRequestBinding& request) {
    return {
        TestAccess::permission(request, true),
        TestAccess::invariant(request, true),
        TestAccess::resilience(request, true),
        TestAccess::freshness(request, true),
        TestAccess::revalidation(request, true)
    };
}

void expectDecision(
    const ProductionTransitionEligibilityDecision& decision,
    ProductionTransitionEligibility eligibility,
    EligibilityRejectionReason reason) {
    require(decision.eligibility() == eligibility);
    require(decision.rejectionReason() == reason);
}

struct BridgeSnapshot {
    double state;
    double health;
    std::size_t bridgeCount;
    std::vector<int> targets;
    std::vector<double> capacities;
    std::vector<BridgeStatus> statuses;

    friend bool operator==(
        const BridgeSnapshot&,
        const BridgeSnapshot&) = default;
};

BridgeSnapshot snapshot(const AutopoieticNode& node) {
    BridgeSnapshot result{
        node.state.load(),
        node.healthIndex.load(),
        node.bridges.size(),
        {},
        {},
        {}
    };

    result.targets.reserve(node.bridges.size());
    result.capacities.reserve(node.bridges.size());
    result.statuses.reserve(node.bridges.size());

    for (const auto& bridge : node.bridges) {
        result.targets.push_back(bridge.targetNodeId);
        result.capacities.push_back(bridge.capacity);
        result.statuses.push_back(bridge.status);
    }

    return result;
}

} // namespace

int main() {
    const ProductionTransitionEligibilityEvaluator evaluator;
    const auto request = makeRequest();

    // C01: exact coherent context + all five satisfied.
    {
        const auto prerequisites = allSatisfied(request);
        const auto decision = evaluator.evaluate(request, prerequisites);
        expectDecision(
            decision,
            ProductionTransitionEligibility::
                eligible_for_authority_consideration,
            EligibilityRejectionReason::none);
        require(decision.binding() == request);
    }

    // C02-C06: each missing prerequisite fails closed.
    {
        auto prerequisites = allSatisfied(request);
        prerequisites.permission.reset();
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::missing_prerequisite);
    }
    {
        auto prerequisites = allSatisfied(request);
        prerequisites.invariant.reset();
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::missing_prerequisite);
    }
    {
        auto prerequisites = allSatisfied(request);
        prerequisites.resilience.reset();
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::missing_prerequisite);
    }
    {
        auto prerequisites = allSatisfied(request);
        prerequisites.freshness.reset();
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::missing_prerequisite);
    }
    {
        auto prerequisites = allSatisfied(request);
        prerequisites.revalidation.reset();
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::missing_prerequisite);
    }

    // C07-C11: present but negative evidence remains distinct from missing.
    {
        auto prerequisites = allSatisfied(request);
        prerequisites.permission =
            TestAccess::permission(request, false);
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::permission_not_satisfied);
    }
    {
        auto prerequisites = allSatisfied(request);
        prerequisites.invariant =
            TestAccess::invariant(request, false);
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::invariant_not_satisfied);
    }
    {
        auto prerequisites = allSatisfied(request);
        prerequisites.resilience =
            TestAccess::resilience(request, false);
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::resilience_not_satisfied);
    }
    {
        auto prerequisites = allSatisfied(request);
        prerequisites.freshness =
            TestAccess::freshness(request, false);
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::freshness_not_satisfied);
    }
    {
        auto prerequisites = allSatisfied(request);
        prerequisites.revalidation =
            TestAccess::revalidation(request, false);
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::revalidation_failed);
    }

    // C12: relationship generation mismatch.
    {
        auto prerequisites = allSatisfied(request);
        const auto other = makeRequest(
            RequestedTransitionDirection::constrain,
            8,
            11,
            21);
        prerequisites.permission =
            TestAccess::permission(other, true);
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::binding_mismatch);
    }

    // C13: direction mismatch.
    {
        auto prerequisites = allSatisfied(request);
        const auto other = makeRequest(
            RequestedTransitionDirection::support,
            7,
            11,
            21);
        prerequisites.invariant =
            TestAccess::invariant(other, true);
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::binding_mismatch);
    }

    // C14: transition class mismatch.
    {
        auto prerequisites = allSatisfied(request);
        const auto other = makeRequest(
            RequestedTransitionDirection::constrain,
            7,
            12,
            21);
        prerequisites.resilience =
            TestAccess::resilience(other, true);
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::binding_mismatch);
    }

    // C15: state version mismatch.
    {
        auto prerequisites = allSatisfied(request);
        const auto other = makeRequest(
            RequestedTransitionDirection::constrain,
            7,
            11,
            22);
        prerequisites.freshness =
            TestAccess::freshness(other, true);
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::binding_mismatch);
    }

    // C16: repeated identical coherent inputs classify identically.
    {
        const auto prerequisites = allSatisfied(request);
        const auto first = evaluator.evaluate(request, prerequisites);
        const auto second = evaluator.evaluate(request, prerequisites);

        require(first.eligibility() == second.eligibility());
        require(first.rejectionReason() == second.rejectionReason());
        require(first.binding() == second.binding());
    }

    // C17: all evidence satisfied but one state version mismatched.
    {
        auto prerequisites = allSatisfied(request);
        const auto other = makeRequest(
            RequestedTransitionDirection::constrain,
            7,
            11,
            99);
        prerequisites.revalidation =
            TestAccess::revalidation(other, true);

        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::binding_mismatch);
    }

    // Frozen rejection precedence: missing beats binding mismatch.
    {
        auto prerequisites = allSatisfied(request);
        prerequisites.invariant.reset();

        const auto other = makeRequest(
            RequestedTransitionDirection::support,
            7,
            11,
            21);
        prerequisites.permission =
            TestAccess::permission(other, true);

        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::missing_prerequisite);
    }

    // Frozen rejection precedence: binding mismatch beats negative evidence.
    {
        auto prerequisites = allSatisfied(request);

        const auto other = makeRequest(
            RequestedTransitionDirection::support,
            7,
            11,
            21);
        prerequisites.permission =
            TestAccess::permission(other, false);

        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::binding_mismatch);
    }

    // Frozen evidence rejection precedence.
    {
        auto prerequisites = allSatisfied(request);
        prerequisites.permission =
            TestAccess::permission(request, false);
        prerequisites.invariant =
            TestAccess::invariant(request, false);
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::permission_not_satisfied);
    }
    {
        auto prerequisites = allSatisfied(request);
        prerequisites.invariant =
            TestAccess::invariant(request, false);
        prerequisites.resilience =
            TestAccess::resilience(request, false);
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::invariant_not_satisfied);
    }
    {
        auto prerequisites = allSatisfied(request);
        prerequisites.resilience =
            TestAccess::resilience(request, false);
        prerequisites.freshness =
            TestAccess::freshness(request, false);
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::resilience_not_satisfied);
    }
    {
        auto prerequisites = allSatisfied(request);
        prerequisites.freshness =
            TestAccess::freshness(request, false);
        prerequisites.revalidation =
            TestAccess::revalidation(request, false);
        expectDecision(
            evaluator.evaluate(request, prerequisites),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::freshness_not_satisfied);
    }

    // Explicit information distinction.
    {
        auto missing = allSatisfied(request);
        missing.invariant.reset();

        auto negative = allSatisfied(request);
        negative.invariant =
            TestAccess::invariant(request, false);

        expectDecision(
            evaluator.evaluate(request, missing),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::missing_prerequisite);

        expectDecision(
            evaluator.evaluate(request, negative),
            ProductionTransitionEligibility::not_eligible,
            EligibilityRejectionReason::invariant_not_satisfied);
    }

    // Supplemental production-state non-mutation test.
    {
        AutopoieticNode node(700, Vector3D{0.0, 0.0, 0.0}, 1.6180339887);
        node.state.store(0.42);
        node.healthIndex.store(0.91);
        node.bridges.push_back(
            SpatialBridge{
                701,
                3.0,
                0.8,
                0.6,
                BridgeStatus::DAMPING
            });

        const auto before = snapshot(node);

        const auto prerequisites = allSatisfied(request);
        const auto decision = evaluator.evaluate(request, prerequisites);

        expectDecision(
            decision,
            ProductionTransitionEligibility::
                eligible_for_authority_consideration,
            EligibilityRejectionReason::none);

        const auto after = snapshot(node);
        require(before == after);
    }

    return 0;
}
