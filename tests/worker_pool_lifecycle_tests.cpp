#include "system_architecture.hpp"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void populateLinearMesh(
    AdaptiveMesh::SpatialAdaptiveMesh& mesh,
    std::size_t nodeCount)
{
    for (std::size_t nodeId = 0; nodeId < nodeCount; ++nodeId) {
        mesh.addNode(
            nodeId,
            {static_cast<double>(nodeId), 0.0, 0.0},
            0.0);
    }
    for (std::size_t nodeId = 1; nodeId < nodeCount; ++nodeId) {
        mesh.connectNodes(
            static_cast<int>(nodeId - 1),
            static_cast<int>(nodeId));
    }
}

void exerciseFixedPool(std::size_t workerCount) {
    AdaptiveMesh::SpatialAdaptiveMesh mesh(workerCount);
    populateLinearMesh(mesh, 4);
    mesh.injectExternalShock(0, 2.0);
    for (std::size_t step = 0; step < 3; ++step) {
        mesh.simulationStep();
    }
    require(
        std::isfinite(mesh.getNodeState(0)),
        "fixed worker pool must produce a finite state");
}

void exercisePoolExpansion() {
    AdaptiveMesh::SpatialAdaptiveMesh mesh(4);
    mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
    mesh.simulationStep();

    mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
    mesh.addNode(2, {2.0, 0.0, 0.0}, 0.0);
    mesh.connectNodes(0, 1);
    mesh.connectNodes(1, 2);
    mesh.injectExternalShock(0, 2.0);
    mesh.simulationStep();
    mesh.simulationStep();

    require(
        std::isfinite(mesh.getNodeState(0)),
        "expanded worker pool must produce a finite state");
}

void exerciseDefaultPool() {
    AdaptiveMesh::SpatialAdaptiveMesh mesh;
    populateLinearMesh(mesh, 4);
    mesh.injectExternalShock(0, 2.0);
    mesh.simulationStep();
    require(
        std::isfinite(mesh.getNodeState(0)),
        "default worker pool must produce a finite state");
}

} // namespace

int main() {
    try {
        constexpr std::size_t lifecycleIterations = 128;
        for (std::size_t iteration = 0;
             iteration < lifecycleIterations;
             ++iteration) {
            exerciseFixedPool(2);
            exerciseFixedPool(4);
            exercisePoolExpansion();
            exerciseDefaultPool();
        }
        std::cout << "Worker pool lifecycle test passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Worker pool lifecycle test failed: "
                  << error.what() << '\n';
        return 1;
    }
}
