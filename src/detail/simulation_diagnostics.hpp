#pragma once

#include "detail/spatial_adaptive_mesh_internal_access.hpp"

namespace AdaptiveMesh::detail {

class SimulationDiagnosticsAccess final {
public:
#if SOAM_PHASE_PROFILE_ENABLED
    [[nodiscard]] static SimulationPhaseProfile simulationProfile(
        const SpatialAdaptiveMesh& mesh)
    {
        return SpatialAdaptiveMeshInternalAccess::simulationProfile(mesh);
    }
#endif
};

} // namespace AdaptiveMesh::detail
