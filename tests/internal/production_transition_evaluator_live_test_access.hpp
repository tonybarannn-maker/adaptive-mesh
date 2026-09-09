#pragma once

#include "system_architecture.hpp"

#include <atomic>
#include <memory>
#include <optional>
#include <shared_mutex>

namespace AdaptiveMesh::detail {

class ProductionTransitionEvaluatorLiveTestAccess final {
public:
    class FailClosedBackend final
        : public ProductionTransitionEvaluationBackend {
    public:
        explicit FailClosedBackend(
            std::shared_ptr<std::atomic<bool>> destroyed = {}) noexcept
            : destroyed_(std::move(destroyed))
        {
        }

        ~FailClosedBackend() override {
            if (destroyed_) destroyed_->store(true);
        }

        [[nodiscard]] RelationshipResolution resolveCurrentRelationship(
            const ProductionTransitionEvaluationLocator&) override {
            return relationshipResolutionFailed();
        }

        [[nodiscard]] SnapshotCaptureResult captureSnapshot(
            const CapturedRelationshipIdentity&) override {
            return snapshotCaptureFailed();
        }

        [[nodiscard]] RequestDerivationResult deriveDirection(
            const CoherentProductionTransitionSnapshot&) override {
            return requestDerivationFailed();
        }

        [[nodiscard]] DomainValidationOutcome validatePermission(
            const CoherentProductionTransitionSnapshot&,
            const ProductionDerivedDirection&) override {
            return DomainValidationOutcome::unavailable_or_failed;
        }

        [[nodiscard]] DomainValidationOutcome validateInvariant(
            const CoherentProductionTransitionSnapshot&,
            const ProductionDerivedDirection&) override {
            return DomainValidationOutcome::unavailable_or_failed;
        }

        [[nodiscard]] DomainValidationOutcome validateResilience(
            const CoherentProductionTransitionSnapshot&,
            const ProductionDerivedDirection&) override {
            return DomainValidationOutcome::unavailable_or_failed;
        }

        [[nodiscard]] DomainValidationOutcome validateFreshness(
            const CoherentProductionTransitionSnapshot&,
            const ProductionDerivedDirection&) override {
            return DomainValidationOutcome::unavailable_or_failed;
        }

        [[nodiscard]] FinalRevalidationOutcome revalidate(
            const CoherentProductionTransitionSnapshot&,
            const ProductionDerivedDirection&) override {
            return FinalRevalidationOutcome::unavailable_or_failed;
        }

    private:
        std::shared_ptr<std::atomic<bool>> destroyed_;
    };

    [[nodiscard]] static std::unique_ptr<ProductionTransitionEvaluationBinding>
    binding(std::shared_ptr<ProductionTransitionEvaluationBackend> backend) {
        return ProductionTransitionEvaluatorBindingAccess::bindingForTest(
            std::move(backend));
    }

    [[nodiscard]] static ProductionTransitionEvaluator evaluator(
        const ProductionTransitionEvaluationBinding& binding) noexcept {
        return ProductionTransitionEvaluatorBindingAccess::evaluator(binding);
    }

    [[nodiscard]] static ProductionTransitionEvaluationBindingHandle handle(
        const ProductionTransitionEvaluationBinding& binding) noexcept {
        return ProductionTransitionEvaluatorBindingAccess::handleForTest(
            binding);
    }

    [[nodiscard]] static EvaluationLease acquire(
        const ProductionTransitionEvaluationBindingHandle& handle) noexcept {
        return ProductionTransitionEvaluatorBindingAccess::acquireLeaseForTest(
            handle);
    }

    static void waitUntilInvalidated(
        const ProductionTransitionEvaluationBinding& binding) noexcept {
        ProductionTransitionEvaluatorBindingAccess::
            waitUntilInvalidatedForTest(binding);
    }

    static ProductionPersistenceEvolutionResult evolve(
        BridgePersistence& persistence,
        const BridgePolicyEvidence& evidence) noexcept {
        return ProductionPersistenceAccess::evolve(persistence, evidence);
    }

    static bool reset(BridgePersistence& persistence) noexcept {
        return ProductionPersistenceAccess::reset(persistence);
    }

    [[nodiscard]] static std::unique_ptr<ProductionPersistenceRecord>
    persistenceRecord() {
        return std::unique_ptr<ProductionPersistenceRecord>(
            new ProductionPersistenceRecord());
    }

    static ProductionPersistenceEvolutionResult evolve(
        ProductionPersistenceRecord& record,
        const BridgePolicyEvidence& evidence) {
        return ProductionPersistenceAccess::evolve(record, evidence);
    }

    [[nodiscard]] static ProductionPersistenceLineage lineage(
        const ProductionPersistenceRecord& record) noexcept {
        return ProductionPersistenceAccess::lineage(record);
    }

    [[nodiscard]] static std::optional<std::uint64_t> relationshipGeneration(
        const SpatialAdaptiveMesh& mesh,
        std::size_t sourceNodeId,
        std::size_t targetNodeId) {
        std::shared_lock lock(mesh.topologyMutex);
        const auto found = mesh.productionRelationships_.find(
            SpatialAdaptiveMesh::EdgeKey{sourceNodeId, targetNodeId});
        if (found == mesh.productionRelationships_.end()) return std::nullopt;
        return found->second.relationshipGeneration;
    }

    [[nodiscard]] static std::optional<std::uint64_t> contextLineage(
        const SpatialAdaptiveMesh& mesh,
        std::size_t sourceNodeId,
        std::size_t targetNodeId) {
        std::shared_lock lock(mesh.topologyMutex);
        const auto found = mesh.productionRelationships_.find(
            SpatialAdaptiveMesh::EdgeKey{sourceNodeId, targetNodeId});
        if (found == mesh.productionRelationships_.end()) return std::nullopt;
        return found->second.authorityRelevantContextLineage;
    }

    [[nodiscard]] static detail::SnapshotCaptureResult captureSnapshot(
        const SpatialAdaptiveMesh& mesh,
        std::size_t sourceNodeId,
        std::size_t targetNodeId) {
        SpatialAdaptiveMesh::LiveProductionTransitionEvaluationBackend backend{
            const_cast<SpatialAdaptiveMesh&>(mesh)};
        const auto resolution = backend.resolveCurrentRelationship(
            {sourceNodeId, targetNodeId});
        return backend.captureSnapshot(*resolution.relationship());
    }

    [[nodiscard]] static detail::FinalRevalidationOutcome revalidate(
        const SpatialAdaptiveMesh& mesh,
        const detail::CoherentProductionTransitionSnapshot& snapshot) {
        SpatialAdaptiveMesh::LiveProductionTransitionEvaluationBackend backend{
            const_cast<SpatialAdaptiveMesh&>(mesh)};
        const detail::ProductionDerivedDirection direction{
            RequestedTransitionDirection::support,
            snapshot.lineage()};
        return backend.revalidate(snapshot, direction);
    }

    static void evolvePersistence(
        SpatialAdaptiveMesh& mesh,
        std::size_t sourceNodeId,
        std::size_t targetNodeId,
        const BridgePolicyEvidence& evidence) {
        std::unique_lock lock(mesh.topologyMutex);
        auto found = mesh.productionRelationships_.find(
            SpatialAdaptiveMesh::EdgeKey{sourceNodeId, targetNodeId});
        if (found == mesh.productionRelationships_.end()) return;
        static_cast<void>(ProductionPersistenceAccess::evolve(
            found->second.persistence, evidence));
    }

    static void evolvePersistenceUnderHeldLock(
        SpatialAdaptiveMesh& mesh,
        std::size_t sourceNodeId,
        std::size_t targetNodeId,
        const BridgePolicyEvidence& evidence) {
        auto found = mesh.productionRelationships_.find(
            SpatialAdaptiveMesh::EdgeKey{sourceNodeId, targetNodeId});
        if (found == mesh.productionRelationships_.end()) return;
        static_cast<void>(ProductionPersistenceAccess::evolve(
            found->second.persistence, evidence));
    }

    [[nodiscard]] static std::unique_lock<std::shared_mutex> lockTopology(
        SpatialAdaptiveMesh& mesh) {
        return std::unique_lock<std::shared_mutex>(mesh.topologyMutex);
    }

    static void resetPersistence(
        SpatialAdaptiveMesh& mesh,
        std::size_t sourceNodeId,
        std::size_t targetNodeId) {
        std::unique_lock lock(mesh.topologyMutex);
        auto found = mesh.productionRelationships_.find(
            SpatialAdaptiveMesh::EdgeKey{sourceNodeId, targetNodeId});
        if (found == mesh.productionRelationships_.end()) return;
        static_cast<void>(ProductionPersistenceAccess::reset(
            found->second.persistence.persistence_));
    }
};

} // namespace AdaptiveMesh::detail
