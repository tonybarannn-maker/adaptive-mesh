#include "internal/production_transition_evaluator_live_scenarios.hpp"

#include "detail/spatial_adaptive_mesh_impl.hpp"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace AdaptiveMesh::detail {

class ProductionTransitionEvaluatorScenarioAccess final {
    class FailClosedBackend final : public ProductionTransitionEvaluationBackend {
    public:
        explicit FailClosedBackend(std::shared_ptr<std::atomic<bool>> destroyed = {})
            : destroyed_(std::move(destroyed)) {}
        ~FailClosedBackend() override {
            if (destroyed_) destroyed_->store(true);
        }
        RelationshipResolution resolveCurrentRelationship(
            const ProductionTransitionEvaluationLocator&) override {
            return relationshipResolutionFailed();
        }
        SnapshotCaptureResult captureSnapshot(const CapturedRelationshipIdentity&) override {
            return snapshotCaptureFailed();
        }
        RequestDerivationResult deriveDirection(
            const CoherentProductionTransitionSnapshot&) override {
            return requestDerivationFailed();
        }
        DomainValidationOutcome validatePermission(
            const CoherentProductionTransitionSnapshot&, const ProductionDerivedDirection&) override {
            return DomainValidationOutcome::unavailable_or_failed;
        }
        DomainValidationOutcome validateInvariant(
            const CoherentProductionTransitionSnapshot&, const ProductionDerivedDirection&) override {
            return DomainValidationOutcome::unavailable_or_failed;
        }
        DomainValidationOutcome validateResilience(
            const CoherentProductionTransitionSnapshot&, const ProductionDerivedDirection&) override {
            return DomainValidationOutcome::unavailable_or_failed;
        }
        DomainValidationOutcome validateFreshness(
            const CoherentProductionTransitionSnapshot&, const ProductionDerivedDirection&) override {
            return DomainValidationOutcome::unavailable_or_failed;
        }
        FinalRevalidationOutcome revalidate(
            const CoherentProductionTransitionSnapshot&, const ProductionDerivedDirection&) override {
            return FinalRevalidationOutcome::unavailable_or_failed;
        }
    private:
        std::shared_ptr<std::atomic<bool>> destroyed_;
    };

    static void populate(SpatialAdaptiveMesh& mesh) {
        mesh.addNode(0, {0.0, 0.0, 0.0}, 1.0);
        mesh.addNode(1, {1.0, 0.0, 0.0}, 1.0);
        mesh.addNode(2, {2.0, 0.0, 0.0}, 1.0);
        mesh.addNode(3, {3.0, 0.0, 0.0}, 1.0);
        mesh.connectNodes(0, 1);
        mesh.connectNodes(2, 3);
    }

    static SnapshotCaptureResult capture(
        SpatialAdaptiveMesh& mesh, std::size_t source, std::size_t target) {
        SpatialAdaptiveMesh::Impl::LiveProductionTransitionEvaluationBackend backend{*mesh.impl_};
        const auto resolution = backend.resolveCurrentRelationship({source, target});
        return backend.captureSnapshot(*resolution.relationship());
    }

    static FinalRevalidationOutcome revalidate(
        SpatialAdaptiveMesh& mesh,
        const CoherentProductionTransitionSnapshot& snapshot) {
        SpatialAdaptiveMesh::Impl::LiveProductionTransitionEvaluationBackend backend{*mesh.impl_};
        const ProductionDerivedDirection direction{
            RequestedTransitionDirection::support, snapshot.lineage()};
        return backend.revalidate(snapshot, direction);
    }

    static BridgePolicyEvidence supportEvidence() {
        return AdaptiveBridgePolicy{}.evaluate(
            InteractionObservation(1.0), BridgeConfidence(1.0));
    }

    static void evolve(
        SpatialAdaptiveMesh& mesh, std::size_t source, std::size_t target) {
        std::unique_lock lock(mesh.impl_->topologyMutex);
        const auto found = mesh.impl_->productionRelationships_.find({source, target});
        if (found != mesh.impl_->productionRelationships_.end()) {
            static_cast<void>(ProductionPersistenceAccess::evolve(
                found->second.persistence, supportEvidence()));
        }
    }

    static void reset(SpatialAdaptiveMesh& mesh, std::size_t source, std::size_t target) {
        std::unique_lock lock(mesh.impl_->topologyMutex);
        const auto found = mesh.impl_->productionRelationships_.find({source, target});
        if (found != mesh.impl_->productionRelationships_.end()) {
            const bool changed = ProductionPersistenceAccess::reset(
                found->second.persistence.persistence_);
            if (changed) found->second.persistence.lineage_.advance();
        }
    }

public:
    static bool run(test_support::LiveScenario scenario) {
        using test_support::LiveScenario;
        using Outcome = FinalRevalidationOutcome;
        if (scenario == LiveScenario::fail_closed_binding) {
            SpatialAdaptiveMesh mesh;
            mesh.addNode(0, {0.0, 0.0, 0.0}, 1.0);
            mesh.addNode(1, {1.0, 0.0, 0.0}, 1.0);
            mesh.connectNodes(0, 1);
            auto evaluator = mesh.productionTransitionEvaluator();
            return evaluator.evaluate({0, 1}) == ProductionTransitionEvaluation::not_eligible;
        }
        if (scenario == LiveScenario::persistence_change_detection) {
            BridgePersistence persistence(0.5, 0.25, 2, 2);
            const auto evidence = supportEvidence();
            const auto first = ProductionPersistenceAccess::evolve(persistence, evidence);
            const auto second = ProductionPersistenceAccess::evolve(persistence, evidence);
            const auto stable = ProductionPersistenceAccess::evolve(persistence, evidence);
            return first.completeStateChanged() && second.completeStateChanged() &&
                !stable.completeStateChanged();
        }
        if (scenario == LiveScenario::invalidate_and_drain) {
            auto destroyed = std::make_shared<std::atomic<bool>>(false);
            auto backend = std::make_shared<FailClosedBackend>(destroyed);
            ProductionTransitionEvaluationBinding binding{backend};
            backend.reset();
            const ProductionTransitionEvaluationBindingHandle handle{binding.state_};
            auto lease = handle.acquireEvaluationLease();
            if (!lease) return false;
            std::atomic<bool> completed{false};
            std::mutex mutex;
            std::condition_variable condition;
            bool started = false;
            std::thread invalidator([&] {
                {
                    std::lock_guard lock(mutex);
                    started = true;
                }
                condition.notify_one();
                binding.invalidateAndDrain();
                completed.store(true);
            });
            {
                std::unique_lock lock(mutex);
                condition.wait(lock, [&] { return started; });
            }
            {
                std::unique_lock lock(binding.state_->mutex_);
                binding.state_->drained_.wait(lock, [&] {
                    return !binding.state_->acceptingLeases_;
                });
            }
            const bool blocked = !handle.acquireEvaluationLease() &&
                !completed.load() && !destroyed->load();
            lease = {};
            invalidator.join();
            return blocked && completed.load() && destroyed->load() &&
                !handle.acquireEvaluationLease();
        }

        SpatialAdaptiveMesh mesh;
        populate(mesh);
        const auto captured = capture(mesh, 0, 1);
        if (!captured.snapshot()) return false;
        if (scenario == LiveScenario::unchanged_persistence_current) {
            return revalidate(mesh, *captured.snapshot()) == Outcome::revalidated;
        }
        if (scenario == LiveScenario::changed_persistence_stale) {
            evolve(mesh, 0, 1);
            return revalidate(mesh, *captured.snapshot()) == Outcome::stale;
        }
        if (scenario == LiveScenario::persistence_anti_resurrection) {
            evolve(mesh, 0, 1);
            evolve(mesh, 0, 1);
            reset(mesh, 0, 1);
            return revalidate(mesh, *captured.snapshot()) == Outcome::stale;
        }
        if (scenario == LiveScenario::unrelated_relationship_isolation) {
            evolve(mesh, 2, 3);
            return revalidate(mesh, *captured.snapshot()) == Outcome::revalidated;
        }
        if (scenario == LiveScenario::reverse_direction_isolation) {
            evolve(mesh, 1, 0);
            return revalidate(mesh, *captured.snapshot()) == Outcome::revalidated;
        }
        return false;
    }
};

} // namespace AdaptiveMesh::detail

namespace AdaptiveMesh::test_support {

bool runLiveScenario(LiveScenario scenario) {
    return detail::ProductionTransitionEvaluatorScenarioAccess::run(scenario);
}

} // namespace AdaptiveMesh::test_support
