#include "adaptive_bridge_policy.hpp"
#include "bridge_persistence.hpp"
#include "detail/production_transition_evaluation_internal.hpp"
#include "detail/spatial_adaptive_mesh_internal_access.hpp"

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

class SnapshotFactory : public detail::ProductionTransitionEvaluationBackend {
public:
    [[nodiscard]] static detail::SnapshotCaptureResult rebuild(
        const detail::CoherentProductionTransitionSnapshot& snapshot,
        detail::ProductionD7PublicationStatus status,
        const detail::ProductionD7PublicationLineage& lineage) {
        return completeSnapshot(
            snapshot.relationship(),
            snapshot.stateVersion().opaqueValueForConstruction(),
            snapshot.transitionClass().opaqueValueForConstruction(),
            snapshot.lineage(),
            status,
            lineage,
            snapshot.persistenceState(),
            snapshot.persistenceLineage());
    }

    [[nodiscard]] static detail::SnapshotCaptureResult forDirection(
        const detail::CoherentProductionTransitionSnapshot& snapshot,
        detail::ProductionD7PublicationStatus status,
        PersistentBridgeRecommendation recommendation) {
        auto persistence = snapshot.persistenceState();
        persistence.recommendation = static_cast<std::uint8_t>(recommendation);
        return completeSnapshot(
            snapshot.relationship(),
            snapshot.stateVersion().opaqueValueForConstruction(),
            snapshot.transitionClass().opaqueValueForConstruction(),
            snapshot.lineage(),
            status,
            snapshot.d7PublicationLineage(),
            persistence,
            snapshot.persistenceLineage());
    }

    [[nodiscard]] static detail::RequestDerivationResult derive(
        const detail::CoherentProductionTransitionSnapshot& snapshot) noexcept {
        return deriveCapturedDirection(snapshot);
    }
};

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

void testD7DirectionDerivationFromCapturedSnapshot() {
    SpatialAdaptiveMesh mesh;
    mesh.addNode(0, {0.0, 0.0, 0.0}, 1.0);
    mesh.addNode(1, {1.0, 0.0, 0.0}, 1.0);
    mesh.connectNodes(0, 1);

    const auto captured =
        detail::SpatialAdaptiveMeshInternalAccess::captureProductionSnapshot(
            mesh, 0, 1);
    require(captured.snapshot().has_value());
    const auto& snapshot = *captured.snapshot();

    const auto unpublished = SnapshotFactory::derive(snapshot);
    require(
        unpublished.status() ==
        detail::RequestDerivationStatus::unavailable_or_failed);
    require(!unpublished.direction().has_value());

    const auto unavailableSnapshot = SnapshotFactory::forDirection(
        snapshot,
        detail::ProductionD7PublicationStatus::unavailable,
        PersistentBridgeRecommendation::PRESERVE);
    require(unavailableSnapshot.snapshot().has_value());
    const auto unavailable = SnapshotFactory::derive(
        *unavailableSnapshot.snapshot());
    require(
        unavailable.status() ==
        detail::RequestDerivationStatus::unavailable_or_failed);
    require(!unavailable.direction().has_value());

    const auto preserveSnapshot = SnapshotFactory::forDirection(
        snapshot,
        detail::ProductionD7PublicationStatus::authoritative,
        PersistentBridgeRecommendation::PRESERVE);
    require(preserveSnapshot.snapshot().has_value());
    const auto preserve = SnapshotFactory::derive(*preserveSnapshot.snapshot());
    require(preserve.status() == detail::RequestDerivationStatus::no_request);
    require(!preserve.direction().has_value());

    const auto constrainSnapshot = SnapshotFactory::forDirection(
        snapshot,
        detail::ProductionD7PublicationStatus::authoritative,
        PersistentBridgeRecommendation::CONSTRAIN);
    require(constrainSnapshot.snapshot().has_value());
    const auto constrain = SnapshotFactory::derive(*constrainSnapshot.snapshot());
    require(constrain.status() == detail::RequestDerivationStatus::derived);
    require(constrain.direction().has_value());
    require(
        constrain.direction()->direction() ==
        RequestedTransitionDirection::constrain);
    require(constrain.direction()->lineage() == snapshot.lineage());

    const auto supportSnapshot = SnapshotFactory::forDirection(
        snapshot,
        detail::ProductionD7PublicationStatus::authoritative,
        PersistentBridgeRecommendation::SUPPORT);
    require(supportSnapshot.snapshot().has_value());
    const auto support = SnapshotFactory::derive(*supportSnapshot.snapshot());
    require(support.status() == detail::RequestDerivationStatus::derived);
    require(support.direction().has_value());
    require(
        support.direction()->direction() ==
        RequestedTransitionDirection::support);
    require(support.direction()->lineage() == snapshot.lineage());
}

void testD7PublicationSnapshotCoherence() {
    SpatialAdaptiveMesh mesh;
    mesh.addNode(0, {0.0, 0.0, 0.0}, 1.0);
    mesh.addNode(1, {1.0, 0.0, 0.0}, 1.0);
    mesh.connectNodes(0, 1);

    const auto captured =
        detail::SpatialAdaptiveMeshInternalAccess::captureProductionSnapshot(
            mesh, 0, 1);
    require(captured.snapshot().has_value());
    const auto& snapshot = *captured.snapshot();

    require(
        snapshot.d7PublicationStatus() ==
        detail::ProductionD7PublicationStatus::unpublished);
    require(
        detail::SpatialAdaptiveMeshInternalAccess::revalidateProductionSnapshot(
            mesh, snapshot) == detail::FinalRevalidationOutcome::revalidated);

    const auto statusMismatch = SnapshotFactory::rebuild(
        snapshot,
        detail::ProductionD7PublicationStatus::unavailable,
        snapshot.d7PublicationLineage());
    require(statusMismatch.snapshot().has_value());
    require(
        detail::SpatialAdaptiveMeshInternalAccess::revalidateProductionSnapshot(
            mesh, *statusMismatch.snapshot()) ==
        detail::FinalRevalidationOutcome::stale);

    const detail::ProductionD7PublicationRecord distinctPublication;
    require(
        distinctPublication.lineage() != snapshot.d7PublicationLineage());
    const auto lineageMismatch = SnapshotFactory::rebuild(
        snapshot,
        snapshot.d7PublicationStatus(),
        distinctPublication.lineage());
    require(lineageMismatch.snapshot().has_value());
    require(
        lineageMismatch.snapshot()->persistenceState() ==
        snapshot.persistenceState());
    require(
        lineageMismatch.snapshot()->persistenceLineage() ==
        snapshot.persistenceLineage());
    require(
        lineageMismatch.snapshot()->d7PublicationStatus() ==
        snapshot.d7PublicationStatus());
    require(
        detail::SpatialAdaptiveMeshInternalAccess::revalidateProductionSnapshot(
            mesh, *lineageMismatch.snapshot()) ==
        detail::FinalRevalidationOutcome::stale);
}

} // namespace

int main() {
    testCompleteStateChangeDetection();
    testDirectionReversalRestartsAccumulation();
    testResetClearsCompleteState();
    testD7DirectionDerivationFromCapturedSnapshot();
    testD7PublicationSnapshotCoherence();
    return EXIT_SUCCESS;
}