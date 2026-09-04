#include "system_architecture.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

enum class Topology { Linear, DegreeFour };
enum class SetupMode { Individual, Bulk };

const char* topologyName(Topology topology) {
    return topology == Topology::Linear ? "linear" : "degree4";
}

const char* setupModeName(SetupMode mode) {
    return mode == SetupMode::Individual ? "individual" : "bulk";
}

size_t parsePositiveSize(const char* text) {
    try {
        const auto value = std::stoull(text);
        if (value == 0) throw std::invalid_argument("must be positive");
        return static_cast<size_t>(value);
    } catch (const std::exception&) {
        throw std::invalid_argument("max_nodes must be a positive integer");
    }
}

std::vector<std::pair<int, int>> makePairs(size_t nodeCount, Topology topology) {
    std::vector<std::pair<int, int>> pairs;
    if (topology == Topology::Linear) {
        pairs.reserve(nodeCount > 0 ? nodeCount - 1 : 0);
        for (size_t nodeId = 1; nodeId < nodeCount; ++nodeId) {
            pairs.emplace_back(static_cast<int>(nodeId - 1), static_cast<int>(nodeId));
        }
    } else {
        if (nodeCount < 5) throw std::invalid_argument("degree4 requires at least 5 nodes");
        pairs.reserve(nodeCount * 2);
        for (size_t nodeId = 0; nodeId < nodeCount; ++nodeId) {
            pairs.emplace_back(static_cast<int>(nodeId),
                               static_cast<int>((nodeId + 1) % nodeCount));
            pairs.emplace_back(static_cast<int>(nodeId),
                               static_cast<int>((nodeId + 2) % nodeCount));
        }
    }
    return pairs;
}

void addNodes(AdaptiveMesh::SpatialAdaptiveMesh& mesh, size_t nodeCount) {
    for (size_t nodeId = 0; nodeId < nodeCount; ++nodeId) {
        mesh.addNode(nodeId, {static_cast<double>(nodeId), 0.0, 0.0}, 0.0);
    }
}

struct Result {
    double setupMs;
    double warmupMs;
    double measuredMs;
    double avgStepUs;
    double finalState;
};

Result run(size_t nodeCount, SetupMode mode,
           const std::vector<std::pair<int, int>>& pairs) {
    const auto setupStart = std::chrono::steady_clock::now();
    AdaptiveMesh::SpatialAdaptiveMesh mesh(1);
    addNodes(mesh, nodeCount);
    if (mode == SetupMode::Individual) {
        for (const auto& [nodeA, nodeB] : pairs) mesh.connectNodes(nodeA, nodeB);
    } else {
        mesh.connectNodePairs(pairs);
    }
    mesh.injectExternalShock(0, 2.0);
    const auto setupEnd = std::chrono::steady_clock::now();

    constexpr size_t warmupSteps = 5;
    constexpr size_t measuredSteps = 20;
    const auto warmupStart = std::chrono::steady_clock::now();
    for (size_t step = 0; step < warmupSteps; ++step) mesh.simulationStep();
    const auto warmupEnd = std::chrono::steady_clock::now();

    const auto measuredStart = std::chrono::steady_clock::now();
    for (size_t step = 0; step < measuredSteps; ++step) mesh.simulationStep();
    const auto measuredEnd = std::chrono::steady_clock::now();

    const double setupMs = std::chrono::duration<double, std::milli>(setupEnd - setupStart).count();
    const double warmupMs = std::chrono::duration<double, std::milli>(warmupEnd - warmupStart).count();
    const double measuredMs = std::chrono::duration<double, std::milli>(measuredEnd - measuredStart).count();
    const double avgStepUs = measuredMs * 1000.0 / static_cast<double>(measuredSteps);
    const double finalState = mesh.getNodeState(nodeCount / 2);
    if (!std::isfinite(setupMs) || !std::isfinite(warmupMs) ||
        !std::isfinite(measuredMs) || !std::isfinite(avgStepUs) ||
        !std::isfinite(finalState)) {
        throw std::runtime_error("benchmark produced a non-finite result");
    }
    return {setupMs, warmupMs, measuredMs, avgStepUs, finalState};
}

} // namespace

int main(int argc, char** argv) {
    try {
        constexpr size_t nodeCounts[] = {10000, 25000, 50000};
        constexpr Topology topologies[] = {Topology::Linear, Topology::DegreeFour};
        constexpr SetupMode modes[] = {SetupMode::Individual, SetupMode::Bulk};
        const size_t maxNodes = argc > 1 ? parsePositiveSize(argv[1]) : 50000;

        std::cout << "setup_mode,topology,nodes,workers,setup_ms,warmup_ms,measured_ms,"
                     "avg_step_us,speedup_vs_1w,parallel_efficiency,final_state\n";
        std::cout << std::fixed << std::setprecision(3) << std::flush;
        for (const Topology topology : topologies) {
            for (const size_t nodeCount : nodeCounts) {
                if (nodeCount > maxNodes) continue;
                const auto pairs = makePairs(nodeCount, topology);
                for (const SetupMode mode : modes) {
                    const Result result = run(nodeCount, mode, pairs);
                    std::cout << setupModeName(mode) << ',' << topologyName(topology) << ','
                              << nodeCount << ",1," << result.setupMs << ',' << result.warmupMs
                              << ',' << result.measuredMs << ',' << result.avgStepUs
                              << ",1.000,1.000," << result.finalState << '\n' << std::flush;
                }
            }
        }
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Adaptive Mesh setup A/B failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
