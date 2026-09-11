#include "detail/production_transition_evaluation_internal.hpp"
#include "detail/production_transition_evaluator_binding_access.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <cstddef>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

namespace {

struct BackendControl final {
    std::mutex mutex;
    std::condition_variable condition;
    bool firstEntered = false;
    bool releaseFirst = false;
    std::atomic<std::size_t> entries{0};
    std::atomic<bool> destroyed{false};
};

class BlockingFailClosedBackend final
    : public AdaptiveMesh::detail::ProductionTransitionEvaluationBackend {
public:
    explicit BlockingFailClosedBackend(std::shared_ptr<BackendControl> control)
        : control_(std::move(control)) {}

    ~BlockingFailClosedBackend() override {
        control_->destroyed.store(true);
    }

    [[nodiscard]] AdaptiveMesh::detail::RelationshipResolution
    resolveCurrentRelationship(
        const AdaptiveMesh::ProductionTransitionEvaluationLocator&) override {
        const auto entry = control_->entries.fetch_add(1);
        if (entry == 0) {
            std::unique_lock lock(control_->mutex);
            control_->firstEntered = true;
            control_->condition.notify_all();
            control_->condition.wait(lock, [this] {
                return control_->releaseFirst;
            });
        }
        return relationshipResolutionFailed();
    }

    [[nodiscard]] AdaptiveMesh::detail::SnapshotCaptureResult captureSnapshot(
        const AdaptiveMesh::detail::CapturedRelationshipIdentity&) override {
        return snapshotCaptureFailed();
    }

    [[nodiscard]] AdaptiveMesh::detail::RequestDerivationResult deriveDirection(
        const AdaptiveMesh::detail::CoherentProductionTransitionSnapshot&) override {
        return requestDerivationFailed();
    }

    [[nodiscard]] AdaptiveMesh::detail::DomainValidationOutcome validatePermission(
        const AdaptiveMesh::detail::CoherentProductionTransitionSnapshot&,
        const AdaptiveMesh::detail::ProductionDerivedDirection&) override {
        return AdaptiveMesh::detail::DomainValidationOutcome::unavailable_or_failed;
    }

    [[nodiscard]] AdaptiveMesh::detail::DomainValidationOutcome validateInvariant(
        const AdaptiveMesh::detail::CoherentProductionTransitionSnapshot&,
        const AdaptiveMesh::detail::ProductionDerivedDirection&) override {
        return AdaptiveMesh::detail::DomainValidationOutcome::unavailable_or_failed;
    }

    [[nodiscard]] AdaptiveMesh::detail::DomainValidationOutcome validateResilience(
        const AdaptiveMesh::detail::CoherentProductionTransitionSnapshot&,
        const AdaptiveMesh::detail::ProductionDerivedDirection&) override {
        return AdaptiveMesh::detail::DomainValidationOutcome::unavailable_or_failed;
    }

    [[nodiscard]] AdaptiveMesh::detail::DomainValidationOutcome validateFreshness(
        const AdaptiveMesh::detail::CoherentProductionTransitionSnapshot&,
        const AdaptiveMesh::detail::ProductionDerivedDirection&) override {
        return AdaptiveMesh::detail::DomainValidationOutcome::unavailable_or_failed;
    }

    [[nodiscard]] AdaptiveMesh::detail::FinalRevalidationOutcome revalidate(
        const AdaptiveMesh::detail::CoherentProductionTransitionSnapshot&,
        const AdaptiveMesh::detail::ProductionDerivedDirection&) override {
        return AdaptiveMesh::detail::FinalRevalidationOutcome::unavailable_or_failed;
    }

private:
    std::shared_ptr<BackendControl> control_;
};

} // namespace

int main() {
    using AdaptiveMesh::ProductionTransitionEvaluation;
    using AdaptiveMesh::detail::ProductionTransitionEvaluationBinding;
    using AdaptiveMesh::detail::ProductionTransitionEvaluatorBindingAccess;

    const auto control = std::make_shared<BackendControl>();
    auto backend = std::make_shared<BlockingFailClosedBackend>(control);
    ProductionTransitionEvaluationBinding binding{backend};
    backend.reset();

    auto firstEvaluator =
        ProductionTransitionEvaluatorBindingAccess::evaluator(binding.handle());
    auto probeEvaluator =
        ProductionTransitionEvaluatorBindingAccess::evaluator(binding.handle());

    std::atomic<bool> firstCompleted{false};
    ProductionTransitionEvaluation firstResult =
        ProductionTransitionEvaluation::eligible_for_authority_consideration;
    std::thread firstEvaluation([&] {
        firstResult = firstEvaluator.evaluate({0, 1});
        firstCompleted.store(true);
    });

    {
        std::unique_lock lock(control->mutex);
        control->condition.wait(lock, [&] { return control->firstEntered; });
    }

    std::atomic<bool> drainCompleted{false};
    std::mutex startMutex;
    std::condition_variable startCondition;
    bool drainStarted = false;
    std::thread invalidator([&] {
        {
            std::lock_guard lock(startMutex);
            drainStarted = true;
        }
        startCondition.notify_one();
        binding.invalidateAndDrain();
        drainCompleted.store(true);
    });

    {
        std::unique_lock lock(startMutex);
        startCondition.wait(lock, [&] { return drainStarted; });
    }

    bool admissionClosed = false;
    while (!admissionClosed) {
        const auto before = control->entries.load();
        const auto result = probeEvaluator.evaluate({0, 1});
        const auto after = control->entries.load();
        admissionClosed =
            result == ProductionTransitionEvaluation::not_eligible &&
            after == before;
    }

    const bool blockedWhileLeaseHeld =
        !firstCompleted.load() && !drainCompleted.load() &&
        !control->destroyed.load();

    {
        std::lock_guard lock(control->mutex);
        control->releaseFirst = true;
    }
    control->condition.notify_all();

    firstEvaluation.join();
    invalidator.join();

    const auto entriesBeforeFinalProbe = control->entries.load();
    const bool remainsClosed =
        probeEvaluator.evaluate({0, 1}) ==
            ProductionTransitionEvaluation::not_eligible &&
        control->entries.load() == entriesBeforeFinalProbe;

    const bool completedCorrectly =
        firstResult == ProductionTransitionEvaluation::not_eligible &&
        firstCompleted.load() && drainCompleted.load() &&
        control->destroyed.load();

    return blockedWhileLeaseHeld && completedCorrectly && remainsClosed
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}
