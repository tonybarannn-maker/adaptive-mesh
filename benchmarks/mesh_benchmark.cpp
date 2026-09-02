#include "system_architecture.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void populateLinearMesh(AdaptiveMesh::SpatialAdaptiveMesh& mesh, size_t nodeCount) {
    for (size_t nodeId = 0; nodeId < nodeCount; ++nodeId) {
        mesh.addNode(nodeId, {static_cast<double>(nodeId), 0.0, 0.0}, 0.0);
    }
    for (size_t nodeId = 1; nodeId < nodeCount; ++nodeId) {
        mesh.connectNodes(static_cast<int>(nodeId - 1), static_cast<int>(nodeId));
    }
}

struct BenchmarkResult {
    double totalMilliseconds = 0.0;
    double averageMicroseconds = 0.0;
    double finalState = 0.0;
};

BenchmarkResult runBenchmark(
    size_t nodeCount,
    size_t workerCount,
    size_t warmupSteps,
    size_t measuredSteps)
{
    AdaptiveMesh::SpatialAdaptiveMesh mesh(workerCount);
    populateLinearMesh(mesh, nodeCount);
    mesh.injectExternalShock(0, 2.0);

    for (size_t step = 0; step < warmupSteps; ++step) {
        mesh.simulationStep();
    }

    const auto start = std::chrono::steady_clock::now();
    for (size_t step = 0; step < measuredSteps; ++step) {
        mesh.simulationStep();
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;

    const double totalMilliseconds =
        std::chrono::duration<double, std::milli>(elapsed).count();
    const double averageMicroseconds =
        totalMilliseconds * 1000.0 / static_cast<double>(measuredSteps);
    const double finalState = mesh.getNodeState(nodeCount / 2);

    if (!std::isfinite(totalMilliseconds) ||
        !std::isfinite(averageMicroseconds) ||
        !std::isfinite(finalState)) {
        throw std::runtime_error("benchmark produced a non-finite result");
    }

    return {totalMilliseconds, averageMicroseconds, finalState};
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
        const size_t warmupSteps = argc > 1 ? parsePositiveSize(argv[1], "warmupSteps") : 2;
        const size_t measuredSteps = argc > 2 ? parsePositiveSize(argv[2], "measuredSteps") : 10;

        constexpr size_t nodeCounts[] = {100, 1000};
        constexpr size_t workerCounts[] = {1, 2, 4};

        std::cout << "Adaptive Mesh benchmark\n";
        std::cout << "warmup_steps=" << warmupSteps
                  << " measured_steps=" << measuredSteps << "\n";
        std::cout << "nodes,workers,total_ms,avg_us,final_state\n";
        std::cout << std::fixed << std::setprecision(3);

        for (const size_t nodeCount : nodeCounts) {
            for (const size_t workerCount : workerCounts) {
                const BenchmarkResult result =
                    runBenchmark(nodeCount, workerCount, warmupSteps, measuredSteps);
                std::cout << nodeCount << ',' << workerCount << ','
                          << result.totalMilliseconds << ','
                          << result.averageMicroseconds << ','
                          << result.finalState << '\n';
            }
        }

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Adaptive Mesh benchmark failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
