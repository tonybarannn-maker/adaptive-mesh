#include "system_architecture.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

enum class Topology {
    Linear,
    DegreeFour,
};

const char* topologyName(Topology topology) {
    return topology == Topology::Linear ? "linear" : "degree4";
}

void addNodes(AdaptiveMesh::SpatialAdaptiveMesh& mesh, size_t nodeCount) {
    for (size_t nodeId = 0; nodeId < nodeCount; ++nodeId) {
        mesh.addNode(nodeId, {static_cast<double>(nodeId), 0.0, 0.0}, 0.0);
    }
}

void populateLinearMesh(AdaptiveMesh::SpatialAdaptiveMesh& mesh, size_t nodeCount) {
    addNodes(mesh, nodeCount);
    for (size_t nodeId = 1; nodeId < nodeCount; ++nodeId) {
        mesh.connectNodes(static_cast<int>(nodeId - 1), static_cast<int>(nodeId));
    }
}

void populateDegreeFourMesh(AdaptiveMesh::SpatialAdaptiveMesh& mesh, size_t nodeCount) {
    addNodes(mesh, nodeCount);
    if (nodeCount < 5) {
        throw std::invalid_argument("degree4 topology requires at least 5 nodes");
    }

    // Ring lattice: each node has four neighbors (i-2, i-1, i+1, i+2).
    for (size_t nodeId = 0; nodeId < nodeCount; ++nodeId) {
        mesh.connectNodes(
            static_cast<int>(nodeId),
            static_cast<int>((nodeId + 1) % nodeCount));
        mesh.connectNodes(
            static_cast<int>(nodeId),
            static_cast<int>((nodeId + 2) % nodeCount));
    }
}

void populateMesh(
    AdaptiveMesh::SpatialAdaptiveMesh& mesh,
    size_t nodeCount,
    Topology topology)
{
    if (topology == Topology::Linear) {
        populateLinearMesh(mesh, nodeCount);
    } else {
        populateDegreeFourMesh(mesh, nodeCount);
    }
}

struct BenchmarkResult {
    double totalMilliseconds = 0.0;
    double averageMicroseconds = 0.0;
    double minimumMicroseconds = 0.0;
    double maximumMicroseconds = 0.0;
    double stepsPerSecond = 0.0;
    double finalState = 0.0;
};

BenchmarkResult runBenchmark(
    size_t nodeCount,
    size_t workerCount,
    size_t warmupSteps,
    size_t measuredSteps,
    Topology topology)
{
    AdaptiveMesh::SpatialAdaptiveMesh mesh(workerCount);
    populateMesh(mesh, nodeCount, topology);
    mesh.injectExternalShock(0, 2.0);

    for (size_t step = 0; step < warmupSteps; ++step) {
        mesh.simulationStep();
    }

    double totalMicroseconds = 0.0;
    double minimumMicroseconds = std::numeric_limits<double>::infinity();
    double maximumMicroseconds = 0.0;

    for (size_t step = 0; step < measuredSteps; ++step) {
        const auto start = std::chrono::steady_clock::now();
        mesh.simulationStep();
        const auto elapsed = std::chrono::steady_clock::now() - start;
        const double microseconds =
            std::chrono::duration<double, std::micro>(elapsed).count();

        totalMicroseconds += microseconds;
        minimumMicroseconds = std::min(minimumMicroseconds, microseconds);
        maximumMicroseconds = std::max(maximumMicroseconds, microseconds);
    }

    const double totalMilliseconds = totalMicroseconds / 1000.0;
    const double averageMicroseconds =
        totalMicroseconds / static_cast<double>(measuredSteps);
    const double stepsPerSecond =
        1'000'000.0 / averageMicroseconds;
    const double finalState = mesh.getNodeState(nodeCount / 2);

    if (!std::isfinite(totalMilliseconds) ||
        !std::isfinite(averageMicroseconds) ||
        !std::isfinite(minimumMicroseconds) ||
        !std::isfinite(maximumMicroseconds) ||
        !std::isfinite(stepsPerSecond) ||
        !std::isfinite(finalState)) {
        throw std::runtime_error("benchmark produced a non-finite result");
    }

    return {
        totalMilliseconds,
        averageMicroseconds,
        minimumMicroseconds,
        maximumMicroseconds,
        stepsPerSecond,
        finalState,
    };
}

size_t parsePositiveSize(const char* text, const char* name) {
    try {
        const auto value = std::stoull(text);
        if (value == 0) {
            throw std::invalid_argument("must be positive");
        }
        return static_cast<size_t>(value);
    } catch (const std::exception&) {
        throw std::invalid_argument(std::string{name} + " must be a positive integer");
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        const size_t warmupSteps = argc > 1 ? parsePositiveSize(argv[1], "warmupSteps") : 5;
        const size_t measuredSteps = argc > 2 ? parsePositiveSize(argv[2], "measuredSteps") : 20;

        constexpr size_t nodeCounts[] = {1000, 10000, 25000, 50000, 100000};
        constexpr size_t workerCounts[] = {1, 2, 4, 8};
        constexpr Topology topologies[] = {Topology::Linear, Topology::DegreeFour};

        std::cout << "Adaptive Mesh scaling benchmark\n";
        std::cout << "warmup_steps=" << warmupSteps
                  << " measured_steps=" << measuredSteps << "\n";
        std::cout << "topology,nodes,workers,total_ms,avg_us,min_us,max_us,steps_per_sec,final_state\n";
        std::cout << std::fixed << std::setprecision(3);

        for (const Topology topology : topologies) {
            for (const size_t nodeCount : nodeCounts) {
                for (const size_t workerCount : workerCounts) {
                    const BenchmarkResult result = runBenchmark(
                        nodeCount,
                        workerCount,
                        warmupSteps,
                        measuredSteps,
                        topology);
                    std::cout << topologyName(topology) << ','
                              << nodeCount << ','
                              << workerCount << ','
                              << result.totalMilliseconds << ','
                              << result.averageMicroseconds << ','
                              << result.minimumMicroseconds << ','
                              << result.maximumMicroseconds << ','
                              << result.stepsPerSecond << ','
                              << result.finalState << '\n';
                }
            }
        }

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Adaptive Mesh benchmark failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
