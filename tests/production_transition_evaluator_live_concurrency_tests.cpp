#include "internal/production_transition_evaluator_live_test_access.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <memory>
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
}
