#include "system_architecture.hpp"
#include "detail/simulation_phase_profile.hpp"

#include <cmath>
#include <exception>
#include <limits>

int main() {
    using AdaptiveMesh::SpatialAdaptiveMesh;
    using AdaptiveMesh::detail::SimulationPhaseProfileAccessor;

    {
        SpatialAdaptiveMesh emptyMesh(1);
        if (SimulationPhaseProfileAccessor::current(emptyMesh).has_value()) return 1;
        emptyMesh.simulationStep();
        if (SimulationPhaseProfileAccessor::current(emptyMesh).has_value()) return 1;
    }

    {
        SpatialAdaptiveMesh mesh(1);
        mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
        mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
        mesh.connectNodes(0, 1);
        mesh.injectExternalShock(0, 2.0);

        if (SimulationPhaseProfileAccessor::current(mesh).has_value()) return 1;
        mesh.simulationStep();
        const auto profile = SimulationPhaseProfileAccessor::current(mesh);
        if (!profile.has_value()) return 1;
        if (!std::isfinite(profile->preValidationMicroseconds)) return 1;
        if (!std::isfinite(profile->workerPoolReadyMicroseconds)) return 1;
        if (!std::isfinite(profile->bufferPreparationMicroseconds)) return 1;
        if (!std::isfinite(profile->workerDispatchWaitMicroseconds)) return 1;
        if (!std::isfinite(profile->resultValidationMicroseconds)) return 1;
        if (!std::isfinite(profile->commitMicroseconds)) return 1;
        if (profile->commitMicroseconds < 0.0) return 1;
    }

    {
        SpatialAdaptiveMesh mesh(1);
        mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
        mesh.simulationStep();
        if (!SimulationPhaseProfileAccessor::current(mesh).has_value()) return 1;

        constexpr double huge = std::numeric_limits<double>::max();
        mesh.addNode(1, {1.0, 0.0, 0.0}, huge);
        mesh.addNode(2, {2.0, 0.0, 0.0}, -huge);
        mesh.connectNodes(1, 2);

        bool threw = false;
        try {
            mesh.simulationStep();
        } catch (const std::exception&) {
            threw = true;
        }
        if (!threw) return 1;
        if (SimulationPhaseProfileAccessor::current(mesh).has_value()) return 1;
    }

    return 0;
}
