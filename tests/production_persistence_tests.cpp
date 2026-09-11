#include "adaptive_bridge_policy.hpp"
#include "bridge_persistence.hpp"
#include "detail/production_transition_evaluation_internal.hpp"

#include <cstdlib>

using namespace AdaptiveMesh;

namespace {

[[noreturn]] void failTest() noexcept {
    std::abort();
}

void require(bool condition) noexcept {
    if (!condition) failTest();
}

BridgePolicyEvidence supportEvidence() {
    return AdaptiveBridgePolicy{}.evaluate(
        InteractionObservation(1.0), BridgeConfidence(1.0));
}

BridgePolicyEvidence constrainEvidence() {
    return AdaptiveBridgePolicy{}.evaluate(
        InteractionObservation(0.0), BridgeConfidence(1.0));
}

void testCompleteStateChangeDetection() {
    BridgePersistence persistence(0.5, 0.25, 2, 2);
    const auto evidence = supportEvidence();

    const auto first = detail::ProductionPersistenceAccess::evolve(
        persistence, evidence);
    const auto second = detail::ProductionPersistenceAccess::evolve(
        persistence, evidence);
    const auto stable = detail::ProductionPersistenceAccess::evolve(
        persistence, evidence);

    require(first.completeStateChanged());
    require(first.recommendation() == PersistentBridgeRecommendation::PRESERVE);
    require(second.completeStateChanged());
    require(second.recommendation() == PersistentBridgeRecommendation::SUPPORT);
    require(!stable.completeStateChanged());
    require(stable.recommendation() == PersistentBridgeRecommendation::SUPPORT);
}

void testDirectionReversalRestartsAccumulation() {
    BridgePersistence persistence(0.5, 0.25, 2, 2);

    const auto firstSupport = detail::ProductionPersistenceAccess::evolve(
        persistence, supportEvidence());
    const auto firstConstrain = detail::ProductionPersistenceAccess::evolve(
        persistence, constrainEvidence());
    const auto secondConstrain = detail::ProductionPersistenceAccess::evolve(
        persistence, constrainEvidence());

    require(firstSupport.recommendation() ==
        PersistentBridgeRecommendation::PRESERVE);
    require(firstConstrain.recommendation() ==
        PersistentBridgeRecommendation::PRESERVE);
    require(secondConstrain.recommendation() ==
        PersistentBridgeRecommendation::CONSTRAIN);
}

void testResetClearsCompleteState() {
    BridgePersistence persistence(0.5, 0.25, 2, 2);

    static_cast<void>(detail::ProductionPersistenceAccess::evolve(
        persistence, supportEvidence()));
    static_cast<void>(detail::ProductionPersistenceAccess::evolve(
        persistence, supportEvidence()));

    const auto beforeReset = detail::ProductionPersistenceAccess::state(
        persistence);
    require(
        beforeReset.recommendation ==
        static_cast<std::uint8_t>(PersistentBridgeRecommendation::SUPPORT));

    require(detail::ProductionPersistenceAccess::reset(persistence));

    const auto afterReset = detail::ProductionPersistenceAccess::state(
        persistence);
    require(
        afterReset.recommendation ==
        static_cast<std::uint8_t>(PersistentBridgeRecommendation::PRESERVE));
    require(afterReset.consecutiveSamples == 0);
    require(!detail::ProductionPersistenceAccess::reset(persistence));
}

} // namespace

int main() {
    testCompleteStateChangeDetection();
    testDirectionReversalRestartsAccumulation();
    testResetClearsCompleteState();
    return EXIT_SUCCESS;
}
