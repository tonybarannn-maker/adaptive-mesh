#include "internal/production_transition_evaluator_live_test_access.hpp"
#include "system_architecture.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>

namespace {

using Access =
    AdaptiveMesh::detail::ProductionTransitionEvaluatorLiveTestAccess;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void test_fail_closed_binding() {
    auto backend = std::make_shared<Access::FailClosedBackend>();
    auto binding = Access::binding(backend);
    auto evaluator = Access::evaluator(*binding);

    require(
        evaluator.evaluate({1, 2}) ==
            AdaptiveMesh::ProductionTransitionEvaluation::not_eligible,
        "unavailable live provenance must fail closed");

    binding->invalidateAndDrain();
    require(
        evaluator.evaluate({1, 2}) ==
            AdaptiveMesh::ProductionTransitionEvaluation::not_eligible,
        "invalidated binding must reject later evaluation");
}

void test_complete_persistence_change_detection() {
    using AdaptiveMesh::AdaptiveBridgePolicy;
    using AdaptiveMesh::BridgeConfidence;
    using AdaptiveMesh::BridgePersistence;
    using AdaptiveMesh::InteractionObservation;
    using AdaptiveMesh::PersistentBridgeRecommendation;

    AdaptiveBridgePolicy policy;
    BridgeConfidence confidence(1.0);
    const auto support =
        policy.evaluate(InteractionObservation(1.0), confidence);

    BridgePersistence persistence(0.5, 0.25, 2, 2);
    const auto first = Access::evolve(persistence, support);
    require(first.completeStateChanged(),
            "pending-state evolution must count as complete-state change");
    require(first.recommendation() == PersistentBridgeRecommendation::PRESERVE,
            "first activation sample must preserve recommendation");

    const auto second = Access::evolve(persistence, support);
    require(second.completeStateChanged(),
            "recommendation confirmation must change complete state");
    require(second.recommendation() == PersistentBridgeRecommendation::SUPPORT,
            "second activation sample must confirm support");

    const auto stable = Access::evolve(persistence, support);
    require(!stable.completeStateChanged(),
            "observe without complete-state change must not report change");

    require(Access::reset(persistence),
            "reset from support must report complete-state change");
    require(!Access::reset(persistence),
            "idempotent reset must not report complete-state change");

    auto record = Access::persistenceRecord();
    const auto initialLineage = Access::lineage(*record);
    const auto recordFirst = Access::evolve(*record, support);
    const auto changedLineage = Access::lineage(*record);
    require(recordFirst.completeStateChanged(),
            "record evolution must detect pending-state change");
    require(!(changedLineage == initialLineage),
            "persistence lineage must advance when complete state changes");

    static_cast<void>(Access::evolve(*record, support));
    const auto confirmedLineage = Access::lineage(*record);
    const auto recordStable = Access::evolve(*record, support);
    require(!recordStable.completeStateChanged(),
            "stable persistence record must report unchanged state");
    require(Access::lineage(*record) == confirmedLineage,
            "persistence lineage must not advance without state change");
}

void test_library_owned_mesh_binding_fails_closed_without_d7() {
    AdaptiveMesh::SpatialAdaptiveMesh mesh;
    mesh.addNode(0, {0.0, 0.0, 0.0}, 1.0);
    mesh.addNode(1, {1.0, 0.0, 0.0}, 1.0);
    mesh.connectNodes(0, 1);

    const auto forwardGeneration = Access::relationshipGeneration(mesh, 0, 1);
    const auto reverseGeneration = Access::relationshipGeneration(mesh, 1, 0);
    require(forwardGeneration.has_value() && reverseGeneration.has_value(),
            "both directed relationship lifecycle records must exist");
    require(*forwardGeneration != *reverseGeneration,
            "opposite directions must have distinct lifecycle generations");
    require(Access::contextLineage(mesh, 0, 1).has_value(),
            "directed relationship must own a context lineage");

    auto evaluator = mesh.productionTransitionEvaluator();
    require(
        evaluator.evaluate({0, 1}) ==
            AdaptiveMesh::ProductionTransitionEvaluation::not_eligible,
        "live mesh binding must fail closed while D7 provenance is absent");
}

} // namespace

int main() {
    test_fail_closed_binding();
    test_complete_persistence_change_detection();
    test_library_owned_mesh_binding_fails_closed_without_d7();
    return 0;
}
