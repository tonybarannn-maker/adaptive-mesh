#pragma once

#include "detail/spatial_adaptive_mesh_impl.hpp"

namespace AdaptiveMesh::detail {

class SpatialAdaptiveMeshInternalAccess final {
    class BackendFactoryAccess
        : public ProductionTransitionEvaluationBackend {
    public:
        [[nodiscard]] static SnapshotCaptureResult snapshotFailure() noexcept {
            return snapshotCaptureFailed();
        }

        [[nodiscard]] static ProductionDerivedDirection supportDirection(
            const CoherentProductionTransitionSnapshot& snapshot) noexcept
        {
            const auto request = derivedRequest(
                snapshot,
                RequestedTransitionDirection::support);
            return *request.direction();
        }
    };

public:
#if SOAM_PHASE_PROFILE_ENABLED
    [[nodiscard]] static SimulationPhaseProfile simulationProfile(
        const SpatialAdaptiveMesh& mesh)
    {
        return mesh.impl_->getLastSimulationPhaseProfile();
    }
#endif

    [[nodiscard]] static SnapshotCaptureResult captureProductionSnapshot(
        SpatialAdaptiveMesh& mesh,
        std::size_t source,
        std::size_t target)
    {
        SpatialAdaptiveMesh::Impl::LiveProductionTransitionEvaluationBackend
            backend{*mesh.impl_};
        const auto resolution =
            backend.resolveCurrentRelationship({source, target});
        const auto& relationship = resolution.relationship();
        if (!relationship) return BackendFactoryAccess::snapshotFailure();
        return backend.captureSnapshot(*relationship);
    }

    [[nodiscard]] static FinalRevalidationOutcome revalidateProductionSnapshot(
        SpatialAdaptiveMesh& mesh,
        const CoherentProductionTransitionSnapshot& snapshot)
    {
        SpatialAdaptiveMesh::Impl::LiveProductionTransitionEvaluationBackend
            backend{*mesh.impl_};
        const auto direction = BackendFactoryAccess::supportDirection(snapshot);
        return backend.revalidate(snapshot, direction);
    }
};

} // namespace AdaptiveMesh::detail
