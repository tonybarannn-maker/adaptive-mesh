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

bool sameRelationship(
    const detail::CapturedRelationshipIdentity& lhs,
    const detail::CapturedRelationshipIdentity& rhs) noexcept {
    return lhs.sourceNodeId() == rhs.sourceNodeId() &&
           lhs.targetNodeId() == rhs.targetNodeId() &&
           lhs.generation() == rhs.generation() &&
           lhs.sourceNodeIncarnation() == rhs.sourceNodeIncarnation() &&
           lhs.targetNodeIncarnation() == rhs.targetNodeIncarnation();
}

bool sameCoherentPublicationState(
    const detail::CoherentProductionTransitionSnapshot& lhs,
    const detail::CoherentProductionTransitionSnapshot& rhs) noexcept {
    return sameRelationship(lhs.relationship(), rhs.relationship()) &&
           lhs.lineage() == rhs.lineage() &&
           lhs.d7PublicationStatus() == rhs.d7PublicationStatus() &&
           lhs.d7PublicationLineage() == rhs.d7PublicationLineage() &&
           lhs.persistenceState() == rhs.persistenceState() &&
           lhs.persistenceLineage() == rhs.persistenceLineage();
}

void addConnectedPair(SpatialAdaptiveMesh& mesh) {
    mesh.addNode(0, {0.0, 0.0, 0.0}, 1.0);
    mesh.addNode(1, {1.0, 0.0, 0.0}, 1.0);
    mesh.connectNodes(0, 1);
}

void testBridgePersistenceInterruptPendingPrimitives() {
    BridgePersistence pending(0.5, 0.25, 2, 2);
    const auto initial = detail::ProductionPersistenceAccess::state(pending);
    static_cast<void>(detail::ProductionPersistenceAccess::evolve(
        pending, supportEvidence()));
    const auto beforeInterrupt = detail::ProductionPersistenceAccess::state(pending);
    require(beforeInterrupt.consecutiveSamples == 1);
    require(beforeInterrupt.pendingDirection != initial.pendingDirection);
    require(beforeInterrupt.recommendation ==
        static_cast<std::uint8_t>(PersistentBridgeRecommendation::PRESERVE));
    require(detail::ProductionPersistenceAccess::interruptPending(pending));
    const auto afterInterrupt = detail::ProductionPersistenceAccess::state(pending);
    require(afterInterrupt.consecutiveSamples == 0);
    require(afterInterrupt.pendingDirection == initial.pendingDirection);
    require(afterInterrupt.recommendation ==
        static_cast<std::uint8_t>(PersistentBridgeRecommendation::PRESERVE));

    BridgePersistence stable(0.5, 0.25, 2, 2);
    static_cast<void>(detail::ProductionPersistenceAccess::evolve(
        stable, supportEvidence()));
    static_cast<void>(detail::ProductionPersistenceAccess::evolve(
        stable, supportEvidence()));
    const auto stableBefore = detail::ProductionPersistenceAccess::state(stable);
    require(stableBefore.consecutiveSamples == 0);
    require(stableBefore.recommendation ==
        static_cast<std::uint8_t>(PersistentBridgeRecommendation::SUPPORT));
    require(!detail::ProductionPersistenceAccess::interruptPending(stable));
    require(detail::ProductionPersistenceAccess::state(stable) == stableBefore);
}

void testProductionD7PublicationCasAxesPure() {
    SpatialAdaptiveMesh mesh;
    addConnectedPair(mesh);
    const auto forwardResult =
        detail::SpatialAdaptiveMeshInternalAccess::captureProductionSnapshot(
            mesh, 0, 1);
    const auto reverseResult =
        detail::SpatialAdaptiveMeshInternalAccess::captureProductionSnapshot(
            mesh, 1, 0);
    require(forwardResult.snapshot().has_value());
    require(reverseResult.snapshot().has_value());
    const auto& forward = *forwardResult.snapshot();
    const auto& reverse = *reverseResult.snapshot();

    const detail::ProductionD7PublicationCasState base{
        forward.relationship(),
        forward.lineage(),
        forward.d7PublicationLineage(),
        forward.persistenceLineage()};
    const detail::ProductionD7PublicationCasState exact{
        forward.relationship(),
        forward.lineage(),
        forward.d7PublicationLineage(),
        forward.persistenceLineage()};
    require(detail::publicationCasMatches(base, exact));

    const detail::ProductionD7PublicationCasState identityMismatch{
        reverse.relationship(),
        forward.lineage(),
        forward.d7PublicationLineage(),
        forward.persistenceLineage()};
    require(!detail::publicationCasMatches(base, identityMismatch));

    const detail::ProductionD7PublicationCasState contextMismatch{
        forward.relationship(),
        forward.lineage() + 1,
        forward.d7PublicationLineage(),
        forward.persistenceLineage()};
    require(!detail::publicationCasMatches(base, contextMismatch));

    const detail::ProductionD7PublicationRecord distinctPublication;
    const detail::ProductionD7PublicationCasState d7Mismatch{
        forward.relationship(),
        forward.lineage(),
        distinctPublication.lineage(),
        forward.persistenceLineage()};
    require(!detail::publicationCasMatches(base, d7Mismatch));

    require(reverse.persistenceLineage() != forward.persistenceLineage());
    const detail::ProductionD7PublicationCasState persistenceMismatch{
        forward.relationship(),
        forward.lineage(),
        forward.d7PublicationLineage(),
        reverse.persistenceLineage()};
    require(!detail::publicationCasMatches(base, persistenceMismatch));
}

void testLiveD7AuthoritativePublicationAdvancesLineage() {
    SpatialAdaptiveMesh mesh;
    addConnectedPair(mesh);
    const auto beforeResult =
        detail::SpatialAdaptiveMeshInternalAccess::captureProductionSnapshot(
            mesh, 0, 1);
    require(beforeResult.snapshot().has_value());
    const auto before = *beforeResult.snapshot();

    auto prepared = detail::SpatialAdaptiveMeshInternalAccess::prepareD7Publication(
        mesh,
        ProductionTransitionEvaluationLocator{0, 1},
        detail::ProductionD7PublicationInput::authoritative(supportEvidence()));
    require(prepared.has_value());
    require(
        detail::SpatialAdaptiveMeshInternalAccess::commitD7Publication(
            mesh, std::move(*prepared)) ==
        detail::ProductionD7PublicationCommitStatus::committed);

    const auto afterResult =
        detail::SpatialAdaptiveMeshInternalAccess::captureProductionSnapshot(
            mesh, 0, 1);
    require(afterResult.snapshot().has_value());
    const auto& after = *afterResult.snapshot();
    require(after.d7PublicationStatus() ==
        detail::ProductionD7PublicationStatus::authoritative);
    require(after.d7PublicationLineage() != before.d7PublicationLineage());
    require(after.persistenceLineage() != before.persistenceLineage());
    require(after.persistenceState().consecutiveSamples == 1);
    require(after.persistenceState().recommendation ==
        static_cast<std::uint8_t>(PersistentBridgeRecommendation::PRESERVE));
}

void testLiveD7UnavailablePendingMatrix() {
    {
        SpatialAdaptiveMesh mesh;
        addConnectedPair(mesh);
        auto support = detail::SpatialAdaptiveMeshInternalAccess::prepareD7Publication(
            mesh,
            ProductionTransitionEvaluationLocator{0, 1},
            detail::ProductionD7PublicationInput::authoritative(supportEvidence()));
        require(support.has_value());
        require(
            detail::SpatialAdaptiveMeshInternalAccess::commitD7Publication(
                mesh, std::move(*support)) ==
            detail::ProductionD7PublicationCommitStatus::committed);
        const auto pendingResult =
            detail::SpatialAdaptiveMeshInternalAccess::captureProductionSnapshot(
                mesh, 0, 1);
        require(pendingResult.snapshot().has_value());
        const auto pending = *pendingResult.snapshot();
        require(pending.persistenceState().consecutiveSamples == 1);

        auto unavailable = detail::SpatialAdaptiveMeshInternalAccess::prepareD7Publication(
            mesh,
            ProductionTransitionEvaluationLocator{0, 1},
            detail::ProductionD7PublicationInput::unavailable());
        require(unavailable.has_value());
        require(
            detail::SpatialAdaptiveMeshInternalAccess::commitD7Publication(
                mesh, std::move(*unavailable)) ==
            detail::ProductionD7PublicationCommitStatus::committed);
        const auto afterResult =
            detail::SpatialAdaptiveMeshInternalAccess::captureProductionSnapshot(
                mesh, 0, 1);
        require(afterResult.snapshot().has_value());
        const auto& after = *afterResult.snapshot();
        require(after.d7PublicationStatus() ==
            detail::ProductionD7PublicationStatus::unavailable);
        require(after.d7PublicationLineage() != pending.d7PublicationLineage());
        require(after.persistenceLineage() != pending.persistenceLineage());
        require(after.persistenceState().consecutiveSamples == 0);
        require(after.persistenceState().recommendation ==
            static_cast<std::uint8_t>(PersistentBridgeRecommendation::PRESERVE));
    }

    {
        SpatialAdaptiveMesh mesh;
        addConnectedPair(mesh);
        for (int i = 0; i < 2; ++i) {
            auto support = detail::SpatialAdaptiveMeshInternalAccess::prepareD7Publication(
                mesh,
                ProductionTransitionEvaluationLocator{0, 1},
                detail::ProductionD7PublicationInput::authoritative(supportEvidence()));
            require(support.has_value());
            require(
                detail::SpatialAdaptiveMeshInternalAccess::commitD7Publication(
                    mesh, std::move(*support)) ==
                detail::ProductionD7PublicationCommitStatus::committed);
        }
        const auto stableResult =
            detail::SpatialAdaptiveMeshInternalAccess::captureProductionSnapshot(
                mesh, 0, 1);
        require(stableResult.snapshot().has_value());
        const auto stable = *stableResult.snapshot();
        require(stable.persistenceState().consecutiveSamples == 0);
        require(stable.persistenceState().recommendation ==
            static_cast<std::uint8_t>(PersistentBridgeRecommendation::SUPPORT));

        auto unavailable = detail::SpatialAdaptiveMeshInternalAccess::prepareD7Publication(
            mesh,
            ProductionTransitionEvaluationLocator{0, 1},
            detail::ProductionD7PublicationInput::unavailable());
        require(unavailable.has_value());
        require(
            detail::SpatialAdaptiveMeshInternalAccess::commitD7Publication(
                mesh, std::move(*unavailable)) ==
            detail::ProductionD7PublicationCommitStatus::committed);
        const auto afterResult =
            detail::SpatialAdaptiveMeshInternalAccess::captureProductionSnapshot(
                mesh, 0, 1);
        require(afterResult.snapshot().has_value());
        const auto& after = *afterResult.snapshot();
        require(after.d7PublicationStatus() ==
            detail::ProductionD7PublicationStatus::unavailable);
        require(after.d7PublicationLineage() != stable.d7PublicationLineage());
        require(after.persistenceLineage() == stable.persistenceLineage());
        require(after.persistenceState() == stable.persistenceState());
    }
}

void testLiveD7DeterministicLostUpdateZeroMutation() {
    SpatialAdaptiveMesh mesh;
    addConnectedPair(mesh);
    auto prepT1 = detail::SpatialAdaptiveMeshInternalAccess::prepareD7Publication(
        mesh,
        ProductionTransitionEvaluationLocator{0, 1},
        detail::ProductionD7PublicationInput::authoritative(constrainEvidence()));
    auto prepT2 = detail::SpatialAdaptiveMeshInternalAccess::prepareD7Publication(
        mesh,
        ProductionTransitionEvaluationLocator{0, 1},
        detail::ProductionD7PublicationInput::authoritative(supportEvidence()));
    require(prepT1.has_value());
    require(prepT2.has_value());

    require(
        detail::SpatialAdaptiveMeshInternalAccess::commitD7Publication(
            mesh, std::move(*prepT2)) ==
        detail::ProductionD7PublicationCommitStatus::committed);
    const auto afterT2Result =
        detail::SpatialAdaptiveMeshInternalAccess::captureProductionSnapshot(
            mesh, 0, 1);
    require(afterT2Result.snapshot().has_value());
    const auto afterT2 = *afterT2Result.snapshot();

    require(
        detail::SpatialAdaptiveMeshInternalAccess::commitD7Publication(
            mesh, std::move(*prepT1)) ==
        detail::ProductionD7PublicationCommitStatus::stale_acquisition);
    const auto afterT1Result =
        detail::SpatialAdaptiveMeshInternalAccess::captureProductionSnapshot(
            mesh, 0, 1);
    require(afterT1Result.snapshot().has_value());
    const auto& afterT1 = *afterT1Result.snapshot();
    require(sameCoherentPublicationState(afterT2, afterT1));
}

} // namespace

int main() {
    testCompleteStateChangeDetection();
    testDirectionReversalRestartsAccumulation();
    testResetClearsCompleteState();
    testD7DirectionDerivationFromCapturedSnapshot();
    testD7PublicationSnapshotCoherence();
    testBridgePersistenceInterruptPendingPrimitives();
    testProductionD7PublicationCasAxesPure();
    testLiveD7AuthoritativePublicationAdvancesLineage();
    testLiveD7UnavailablePendingMatrix();
    testLiveD7DeterministicLostUpdateZeroMutation();
    return EXIT_SUCCESS;
}
