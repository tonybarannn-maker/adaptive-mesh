#include "system_architecture.hpp"

#include <cassert>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Exception, typename Function>
void requireThrows(Function&& function, const char* message) {
    try {
        function();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error(message);
}

void test_topology_contract() {
    using namespace AdaptiveMesh;

    SpatialAdaptiveMesh mesh;
    mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);

    requireThrows<std::invalid_argument>(
        [&mesh] { mesh.addNode(42, {1.0, 0.0, 0.0}, 0.0); },
        "node ID must match insertion index"
    );

    mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);

    requireThrows<std::out_of_range>([&mesh] { mesh.connectNodes(-1, 0); },
                                     "negative node ID must throw out_of_range");
    requireThrows<std::out_of_range>([&mesh] { mesh.connectNodes(0, 2); },
                                     "out-of-range node ID must throw out_of_range");
    requireThrows<std::invalid_argument>([&mesh] { mesh.connectNodes(0, 0); },
                                          "self-connection must throw invalid_argument");

    requireThrows<std::out_of_range>([&mesh] { mesh.injectExternalShock(-1, 1.0); },
                                     "negative shock target must throw out_of_range");
    requireThrows<std::out_of_range>([&mesh] { mesh.injectExternalShock(2, 1.0); },
                                     "out-of-range shock target must throw out_of_range");

    mesh.connectNodes(0, 1);
    require(mesh.getNodeBridgesCount(0) == 1, "connection must create a forward bridge");
    require(mesh.getNodeBridgesCount(1) == 1, "connection must create a reverse bridge");

    requireThrows<std::invalid_argument>([&mesh] { mesh.connectNodes(0, 1); },
                                          "duplicate bridge pair must throw invalid_argument");
    require(mesh.getNodeBridgesCount(0) == 1,
            "duplicate connection must not add a second forward bridge");
    require(mesh.getNodeBridgesCount(1) == 1,
            "duplicate connection must not add a second reverse bridge");

    mesh.injectExternalShock(0, 25.0);
    mesh.injectExternalShock(0, 25.0);
    mesh.injectExternalShock(0, 25.0);
    mesh.simulationStep();
    mesh.pruneIsolatedBridges();
    require(mesh.getNodeBridgesCount(0) == 0, "pruning must remove forward bridge with its pair");
    require(mesh.getNodeBridgesCount(1) == 0, "pruning must remove reverse bridge with its pair");
}

void test_bulk_connection_contract() {
    using namespace AdaptiveMesh;
    const std::vector<std::pair<int, int>> pairs{{0, 1}, {1, 2}};
    SpatialAdaptiveMesh bulkMesh;
    SpatialAdaptiveMesh individualMesh;
    for (size_t nodeId = 0; nodeId < 3; ++nodeId) {
        const Vector3D position{static_cast<double>(nodeId), 0.0, 0.0};
        bulkMesh.addNode(nodeId, position, 0.0);
        individualMesh.addNode(nodeId, position, 0.0);
    }

    bulkMesh.connectNodePairs(pairs);
    individualMesh.connectNodes(0, 1);
    individualMesh.connectNodes(1, 2);
    bulkMesh.injectExternalShock(0, 2.0);
    individualMesh.injectExternalShock(0, 2.0);
    bulkMesh.simulationStep();
    individualMesh.simulationStep();
    for (size_t nodeId = 0; nodeId < 3; ++nodeId) {
        require(bulkMesh.getNodeBridgesCount(nodeId) == individualMesh.getNodeBridgesCount(nodeId),
                "bulk and individual topology bridge counts must match");
        require(std::abs(bulkMesh.getNodeState(nodeId) - individualMesh.getNodeState(nodeId)) < 1e-12,
                "bulk and individual topology state must match");
    }

    SpatialAdaptiveMesh invalidMesh;
    invalidMesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
    invalidMesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
    invalidMesh.addNode(2, {2.0, 0.0, 0.0}, 0.0);
    requireThrows<std::out_of_range>([&invalidMesh] {
        invalidMesh.connectNodePairs({{0, 1}, {1, 3}});
    }, "invalid pair must reject the entire batch");
    require(invalidMesh.getNodeBridgesCount(0) == 0 &&
                invalidMesh.getNodeBridgesCount(1) == 0 &&
                invalidMesh.getNodeBridgesCount(2) == 0,
            "invalid batch must not partially mutate topology");
    requireThrows<std::invalid_argument>([&invalidMesh] {
        invalidMesh.connectNodePairs({{0, 1}, {1, 0}});
    }, "duplicate pair in batch must be rejected");
    invalidMesh.connectNodes(0, 1);
    requireThrows<std::invalid_argument>([&invalidMesh] {
        invalidMesh.connectNodePairs({{1, 0}});
    }, "existing pair in batch must be rejected");
}

void test_numeric_input_contract() {
    using namespace AdaptiveMesh;

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    const double negativeInfinity = -std::numeric_limits<double>::infinity();
    SpatialAdaptiveMesh mesh;

    requireThrows<std::invalid_argument>([&mesh, nan] {
        mesh.addNode(0, {nan, 0.0, 0.0}, 0.0);
    }, "NaN coordinate must be rejected");
    requireThrows<std::invalid_argument>([&mesh, infinity] {
        mesh.addNode(0, {0.0, 0.0, 0.0}, infinity);
    }, "infinite baseline must be rejected");

    mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
    mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
    requireThrows<std::invalid_argument>([&mesh, nan] {
        mesh.autoConnectNearbyNodes(nan);
    }, "NaN radius must be rejected");
    requireThrows<std::invalid_argument>([&mesh, infinity] {
        mesh.injectExternalShock(0, infinity);
    }, "infinite shock must be rejected");
    requireThrows<std::invalid_argument>([&mesh, negativeInfinity] {
        mesh.injectExternalShock(0, negativeInfinity);
    }, "negative infinite shock must be rejected");
    requireThrows<std::invalid_argument>([&mesh, nan] {
        mesh.pruneIsolatedBridges(nan);
    }, "NaN pruning threshold must be rejected");

    SpatialBridge bridge{1, 1.0, 0.5};
    bridge.updateBridgeState(SignalCategory::NOISE);
    require(bridge.status == BridgeStatus::DAMPING,
            "bridge state update must accept SignalCategory");
    SpatialBridge invalidBridge{1, infinity, 0.5};
    requireThrows<std::invalid_argument>([&invalidBridge] {
        static_cast<void>(invalidBridge.getEffectiveTransmission());
    }, "infinite bridge distance must be rejected");

    SpatialBridge negativeDistanceBridge{1, -1.0, 0.5};
    requireThrows<std::invalid_argument>([&negativeDistanceBridge] {
        static_cast<void>(negativeDistanceBridge.getEffectiveTransmission());
    }, "negative bridge distance must be rejected");

    SpatialBridge invalidOrientationLow{1, 1.0, -0.01};
    requireThrows<std::invalid_argument>([&invalidOrientationLow] {
        static_cast<void>(invalidOrientationLow.getEffectiveTransmission());
    }, "negative bridge orientationWeight must be rejected");

    SpatialBridge invalidOrientationHigh{1, 1.0, 1.01};
    requireThrows<std::invalid_argument>([&invalidOrientationHigh] {
        static_cast<void>(invalidOrientationHigh.getEffectiveTransmission());
    }, "bridge orientationWeight above one must be rejected");

    SpatialBridge invalidCapacityLow{1, 1.0, 0.5, -0.01};
    requireThrows<std::invalid_argument>([&invalidCapacityLow] {
        static_cast<void>(invalidCapacityLow.getEffectiveTransmission());
    }, "negative bridge capacity must be rejected");

    SpatialBridge invalidCapacityHigh{1, 1.0, 0.5, 1.01};
    requireThrows<std::invalid_argument>([&invalidCapacityHigh] {
        static_cast<void>(invalidCapacityHigh.getEffectiveTransmission());
    }, "bridge capacity above one must be rejected");

    SpatialBridge validBridge{1, 2.0, 0.5, 0.8};
    const double transmission = validBridge.getEffectiveTransmission();
    require(std::isfinite(transmission), "valid bridge transmission must be finite");
}

void populateLinearMesh(AdaptiveMesh::SpatialAdaptiveMesh& mesh, size_t nodeCount) {
    for (size_t nodeId = 0; nodeId < nodeCount; ++nodeId) {
        mesh.addNode(nodeId, {static_cast<double>(nodeId), 0.0, 0.0}, 0.0);
    }
    for (size_t nodeId = 1; nodeId < nodeCount; ++nodeId) {
        mesh.connectNodes(static_cast<int>(nodeId - 1), static_cast<int>(nodeId));
    }
}

void test_worker_configuration_is_deterministic() {
    using namespace AdaptiveMesh;

    SpatialAdaptiveMesh singleWorkerMesh(1);
    SpatialAdaptiveMesh multiWorkerMesh(2);
    populateLinearMesh(singleWorkerMesh, 3);
    populateLinearMesh(multiWorkerMesh, 3);
    singleWorkerMesh.injectExternalShock(0, 2.0);
    multiWorkerMesh.injectExternalShock(0, 2.0);
    singleWorkerMesh.simulationStep();
    multiWorkerMesh.simulationStep();

    for (size_t nodeId = 0; nodeId < 3; ++nodeId) {
        require(std::abs(singleWorkerMesh.getNodeState(nodeId) -
                         multiWorkerMesh.getNodeState(nodeId)) < 1e-12,
                "worker configuration must preserve node state");
        require(std::abs(singleWorkerMesh.getNodeHealth(nodeId) -
                         multiWorkerMesh.getNodeHealth(nodeId)) < 1e-12,
                "worker configuration must preserve node health");
        require(singleWorkerMesh.getNodeBridgesCount(nodeId) ==
                    multiWorkerMesh.getNodeBridgesCount(nodeId),
                "worker configuration must preserve topology");
    }
}

void test_legacy_simulation_step_wrapper() {
    using namespace AdaptiveMesh;

    SpatialAdaptiveMesh mesh(1);
    populateLinearMesh(mesh, 2);
    mesh.injectExternalShock(0, 2.0);
    mesh.simulationStepAsync();

    require(std::isfinite(mesh.getNodeState(0)),
            "legacy simulationStepAsync wrapper must complete synchronously");
}

void test_post_commit_health_remains_finite_for_large_drift() {
    using namespace AdaptiveMesh;

    SpatialAdaptiveMesh mesh(1);
    constexpr double largeBaseline = 1.0e308;
    mesh.addNode(0, {0.0, 0.0, 0.0}, largeBaseline);
    mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
    mesh.connectNodes(0, 1);

    mesh.simulationStep();

    require(std::isfinite(mesh.getNodeState(0)),
            "large finite drift must not produce a non-finite state");
    require(std::isfinite(mesh.getNodeHealth(0)),
            "post-commit health must remain finite for large drift");
    require(std::isfinite(mesh.getNodeHealth(1)),
            "neighbor health must remain finite for large drift");
}

void test_bounded_worker_scale_smoke(size_t nodeCount) {
    using namespace AdaptiveMesh;

    SpatialAdaptiveMesh mesh(4);
    populateLinearMesh(mesh, nodeCount);
    mesh.injectExternalShock(0, 2.0);
    mesh.simulationStep();

    require(std::isfinite(mesh.getNodeState(0)), "scaled simulation state must be finite");
    require(mesh.getNodeBridgesCount(nodeCount / 2) == 2,
            "scaled simulation must preserve interior topology");
}

void test_worker_pool_expands_between_steps() {
    using namespace AdaptiveMesh;

    SpatialAdaptiveMesh mesh(4);
    mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
    mesh.simulationStep();

    mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
    mesh.addNode(2, {2.0, 0.0, 0.0}, 0.0);
    mesh.connectNodes(0, 1);
    mesh.connectNodes(1, 2);
    mesh.injectExternalShock(0, 2.0);
    mesh.simulationStep();

    require(std::isfinite(mesh.getNodeState(0)),
            "expanded worker pool must complete a simulation step");
}

void test_stability_and_shock() {
    using namespace AdaptiveMesh;

    SpatialAdaptiveMesh mesh;
    mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
    mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
    mesh.addNode(2, {2.0, 0.0, 0.0}, 0.0);
    mesh.connectNodes(0, 1);
    mesh.connectNodes(1, 2);

    require(mesh.getNodeBridgesCount(0) == 1, "node 0 must have one bridge");
    require(mesh.getNodeBridgesCount(1) == 2, "node 1 must have two bridges");
    assert(mesh.getNodeBridgesCount(2) == 1);

    mesh.injectExternalShock(0, 2.0);
    const double stateAfterShock = mesh.getNodeState(0);
    require(std::abs(stateAfterShock - 2.0) < 1e-12, "shock must update node state");
    require(mesh.getNodeHealth(0) < 1.0, "shock must reduce node health");
    assert(mesh.getNodeHealth(0) < 1.0);

    mesh.simulationStep();

    const double stateAfterStep = mesh.getNodeState(0);
    const double healthAfterStep = mesh.getNodeHealth(0);
    require(std::isfinite(stateAfterStep), "simulation state must be finite");
    require(std::isfinite(healthAfterStep), "simulation health must be finite");
    require(stateAfterStep < stateAfterShock, "diffusion must reduce shocked node state");
    assert(stateAfterStep < stateAfterShock);

    const std::filesystem::path resultsDirectory =
        std::filesystem::current_path() / "results";
    std::filesystem::create_directories(resultsDirectory);
    std::ofstream trace(resultsDirectory / "runtime_trace.jsonl", std::ios::out | std::ios::trunc);
    require(trace.is_open(), "unable to open telemetry trace");

    trace << "{\"event\":\"smoke_test\",\"nodes\":3,\"shock\":2.0,"
             "\"state_after_step\":" << stateAfterStep
          << ",\"health_after_step\":" << healthAfterStep << "}\n";
    require(trace.good(), "unable to write telemetry trace");
    trace.close();
    require(!trace.fail(), "unable to close telemetry trace");
}

} // namespace

int main() {
    try {
        test_topology_contract();
        test_bulk_connection_contract();
        test_numeric_input_contract();
        test_worker_configuration_is_deterministic();
        test_legacy_simulation_step_wrapper();
        test_post_commit_health_remains_finite_for_large_drift();
        test_bounded_worker_scale_smoke(100);
        test_bounded_worker_scale_smoke(1000);
        test_worker_pool_expands_between_steps();
        test_stability_and_shock();
        std::cout << "Adaptive Mesh smoke test passed." << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Adaptive Mesh smoke test failed: " << error.what() << std::endl;
        return 1;
    }
}
