#include "internal/production_transition_evaluator_live_test_access.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

namespace {

using Access =
    AdaptiveMesh::detail::ProductionTransitionEvaluatorLiveTestAccess;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    auto destroyed = std::make_shared<std::atomic<bool>>(false);
    auto backend = std::make_shared<Access::FailClosedBackend>(destroyed);
    auto binding = Access::binding(backend);
    backend.reset();

    const auto handle = Access::handle(*binding);
    auto activeLease = Access::acquire(handle);
    require(static_cast<bool>(activeLease),
            "lease acquisition must succeed before invalidation");

    std::atomic<bool> drainCompleted{false};
    std::thread invalidator([&] {
        binding->invalidateAndDrain();
        drainCompleted.store(true);
    });

    Access::waitUntilInvalidated(*binding);
    require(!static_cast<bool>(Access::acquire(handle)),
            "new lease must fail after invalidation becomes visible");
    require(!drainCompleted.load(),
            "drain must not complete while the old lease remains active");
    require(!destroyed->load(),
            "backend must remain alive while the old lease is active");

    activeLease = {};
    invalidator.join();

    require(drainCompleted.load(),
            "lease release must permit deterministic drain completion");
    require(destroyed->load(),
            "backend must be destroyed after the final lease drains");
    require(!static_cast<bool>(Access::acquire(handle)),
            "lease acquisition must remain rejected after drain");

    AdaptiveMesh::SpatialAdaptiveMesh mesh;
    mesh.addNode(0, {0.0, 0.0, 0.0}, 1.0);
    mesh.addNode(1, {1.0, 0.0, 0.0}, 1.0);
    mesh.connectNodes(0, 1);
    const auto capture = Access::captureSnapshot(mesh, 0, 1);
    require(capture.snapshot().has_value(),
            "concurrency snapshot must complete");
    const auto evidence = AdaptiveMesh::AdaptiveBridgePolicy{}.evaluate(
        AdaptiveMesh::InteractionObservation(1.0),
        AdaptiveMesh::BridgeConfidence(1.0));

    std::mutex startMutex;
    std::condition_variable startedCondition;
    bool started = false;
    AdaptiveMesh::detail::FinalRevalidationOutcome result =
        AdaptiveMesh::detail::FinalRevalidationOutcome::unavailable_or_failed;

    auto topologyLock = Access::lockTopology(mesh);
    std::thread revalidator([&] {
        {
            std::lock_guard lock(startMutex);
            started = true;
        }
        startedCondition.notify_one();
        result = Access::revalidate(mesh, *capture.snapshot());
    });
    {
        std::unique_lock lock(startMutex);
        startedCondition.wait(lock, [&] { return started; });
    }

    Access::evolvePersistenceUnderHeldLock(mesh, 0, 1, evidence);
    topologyLock.unlock();
    revalidator.join();

    require(result == AdaptiveMesh::detail::FinalRevalidationOutcome::stale,
            "coherent new persistence state and lineage must stale old snapshot");
}
