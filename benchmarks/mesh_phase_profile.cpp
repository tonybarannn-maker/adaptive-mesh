#define ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
#include "system_architecture.hpp"

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

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
    for (size_t nodeId = 0; nodeId < nodeCount; ++nodeId) {
        mesh.connectNodes(static_cast<int>(nodeId), static_cast<int>((nodeId + 1) % nodeCount));
        mesh.connectNodes(static_cast<int>(nodeId), static_cast<int>((nodeId + 2) % nodeCount));
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

struct PhaseTotals {
    double preValidationMicroseconds = 0.0;
    double workerPoolReadyMicroseconds = 0.0;
    double bufferPreparationMicroseconds = 0.0;
    double workerDispatchWaitMicroseconds = 0.0;
    double resultValidationMicroseconds = 0.0;
    double commitMicroseconds = 0.0;
    double postValidationMicroseconds = 0.0;
};

void accumulate(PhaseTotals& totals, const AdaptiveMesh::SimulationPhaseProfile& profile) {
    totals.preValidationMicroseconds += profile.preValidationMicroseconds;
    totals.workerPoolReadyMicroseconds += profile.workerPoolReadyMicroseconds;
    totals.bufferPreparationMicroseconds += profile.bufferPreparationMicroseconds;
    totals.workerDispatchWaitMicroseconds += profile.workerDispatchWaitMicroseconds;
    totals.resultValidationMicroseconds += profile.resultValidationMicroseconds;
    totals.commitMicroseconds += profile.commitMicroseconds;
    totals.postValidationMicroseconds += profile.postValidationMicroseconds;
}

} // namespace

int main() {
    try {
        constexpr size_t warmupSteps = 5;
        constexpr size_t measuredSteps = 20;
        constexpr size_t nodeCounts[] = {50000, 100000};
        constexpr size_t workerCounts[] = {1, 2, 4, 8};
        constexpr Topology topologies[] = {Topology::Linear, Topology::DegreeFour};

        std::cout << "Adaptive Mesh simulation phase profile\n";
        std::cout << "warmup_steps=" << warmupSteps
                  << " measured_steps=" << measuredSteps << "\n";
        std::cout << "topology,nodes,workers,pre_validation_us,worker_pool_ready_us,buffer_prep_us,worker_dispatch_wait_us,result_validation_us,commit_us,post_validation_us\n";
        std::cout << std::fixed << std::setprecision(3);

        for (const Topology topology : topologies) {
            for (const size_t nodeCount : nodeCounts) {
                for (const size_t workerCount : workerCounts) {
                    AdaptiveMesh::SpatialAdaptiveMesh mesh(workerCount);
                    populateMesh(mesh, nodeCount, topology);
                    mesh.injectExternalShock(0, 2.0);

                    for (size_t step = 0; step < warmupSteps; ++step) {
                        mesh.simulationStep();
                    }

                    PhaseTotals totals{};
                    for (size_t step = 0; step < measuredSteps; ++step) {
                        mesh.simulationStep();
                        accumulate(totals, mesh.getLastSimulationPhaseProfile());
                    }

                    const double divisor = static_cast<double>(measuredSteps);
                    std::cout << topologyName(topology) << ','
                              << nodeCount << ','
                              << workerCount << ','
                              << totals.preValidationMicroseconds / divisor << ','
                              << totals.workerPoolReadyMicroseconds / divisor << ','
                              << totals.bufferPreparationMicroseconds / divisor << ','
                              << totals.workerDispatchWaitMicroseconds / divisor << ','
                              << totals.resultValidationMicroseconds / divisor << ','
                              << totals.commitMicroseconds / divisor << ','
                              << totals.postValidationMicroseconds / divisor << '\n';
                }
            }
        }

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Adaptive Mesh phase profile failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}

