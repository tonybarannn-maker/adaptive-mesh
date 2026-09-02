#include "system_architecture.hpp"

#include <cassert>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <iostream>
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
    mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);

    requireThrows<std::out_of_range>([&mesh] { mesh.connectNodes(-1, 0); },
                                     "negative node ID must throw out_of_range");
    requireThrows<std::out_of_range>([&mesh] { mesh.connectNodes(0, 2); },
                                     "out-of-range node ID must throw out_of_range");
    requireThrows<std::invalid_argument>([&mesh] { mesh.connectNodes(0, 0); },
                                          "self-connection must throw invalid_argument");

    mesh.connectNodes(0, 1);
    require(mesh.getNodeBridgesCount(0) == 1, "connection must create a forward bridge");
    require(mesh.getNodeBridgesCount(1) == 1, "connection must create a reverse bridge");

    mesh.injectExternalShock(0, 25.0);
    mesh.injectExternalShock(0, 25.0);
    mesh.injectExternalShock(0, 25.0);
    mesh.simulationStepAsync();
    mesh.pruneIsolatedBridges();
    require(mesh.getNodeBridgesCount(0) == 0, "pruning must remove forward bridge with its pair");
    require(mesh.getNodeBridgesCount(1) == 0, "pruning must remove reverse bridge with its pair");
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

    mesh.simulationStepAsync();

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
        test_stability_and_shock();
        std::cout << "Adaptive Mesh smoke test passed." << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Adaptive Mesh smoke test failed: " << error.what() << std::endl;
        return 1;
    }
}
