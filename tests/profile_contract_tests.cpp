#include "system_architecture.hpp"
#include "detail/simulation_phase_profile.hpp"

#include <cassert>
#include <cmath>

int main() {
    using AdaptiveMesh::SpatialAdaptiveMesh;
    using AdaptiveMesh::detail::SimulationPhaseProfileAccessor;

    {
        SpatialAdaptiveMesh emptyMesh(1);
        assert(!SimulationPhaseProfileAccessor::current(emptyMesh).has_value());
        emptyMesh.simulationStep();
        assert(!SimulationPhaseProfileAccessor::current(emptyMesh).has_value());
    }

    {
        SpatialAdaptiveMesh mesh(1);
        mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
        mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
        mesh.connectNodes(0, 1);
        mesh.injectExternalShock(0, 2.0);

        assert(!SimulationPhaseProfileAccessor::current(mesh).has_value());
        mesh.simulationStep();
        const auto profile = SimulationPhaseProfileAccessor::current(mesh);
        assert(profile.has_value());
        assert(std::isfinite(profile->preValidationMicroseconds));
        assert(std::isfinite(profile->workerPoolReadyMicroseconds));
        assert(std::isfinite(profile->bufferPreparationMicroseconds));
        assert(std::isfinite(profile->workerDispatchWaitMicroseconds));
        assert(std::isfinite(profile->resultValidationMicroseconds));
        assert(std::isfinite(profile->commitMicroseconds));
        assert(profile->commitMicroseconds >= 0.0);
    }

    return 0;
}
