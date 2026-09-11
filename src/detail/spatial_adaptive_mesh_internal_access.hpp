#pragma once

#include "detail/spatial_adaptive_mesh_impl.hpp"

namespace AdaptiveMesh::detail {

class SpatialAdaptiveMeshInternalAccess final {
public:
#if SOAM_PHASE_PROFILE_ENABLED
    [[nodiscard]] static SimulationPhaseProfile simulationProfile(
        const SpatialAdaptiveMesh& mesh)
    {
        return mesh.impl_->getLastSimulationPhaseProfile();
    }
#endif
};

} // namespace AdaptiveMesh::detail
