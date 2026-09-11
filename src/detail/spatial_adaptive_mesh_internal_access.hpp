#pragma once

#include "detail/simulation_phase_profile.hpp"
#include "detail/spatial_adaptive_mesh_impl.hpp"

#include <utility>

namespace AdaptiveMesh::detail {

class SpatialAdaptiveMeshInternalAccess final {
    class BackendFactoryAccess
        : public ProductionTransitionEvaluationBackend {
    public:
        [[nodiscard]] static SnapshotCaptureResult snapshotFailure() noexcept {
            return snapshotCaptureFailed();
        }

        [[nodiscard]] static RequestDerivationResult supportRequest(
            const CoherentProductionTransitionSnapshot& snapshot) noexcept
        {
            return derivedRequest(
                snapshot,
                RequestedTransitionDirection::support);
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
        const auto request = BackendFactoryAccess::supportRequest(snapshot);
        const auto& direction = request.direction();
        if (!direction) return FinalRevalidationOutcome::unavailable_or_failed;
        return backend.revalidate(snapshot, *direction);
    }

    [[nodiscard]] static auto prepareD7Publication(
        SpatialAdaptiveMesh& mesh,
        const ProductionTransitionEvaluationLocator& locator,
        const ProductionD7PublicationInput& input)
    {
        return mesh.impl_->prepareD7Publication(locator, input);
    }

    [[nodiscard]] static ProductionD7PublicationCommitStatus commitD7Publication(
        SpatialAdaptiveMesh& mesh,
        SpatialAdaptiveMesh::Impl::PreparedD7PublicationTransaction&& transaction)
    {
        return mesh.impl_->commitD7Publication(std::move(transaction));
    }
};

} // namespace AdaptiveMesh::detail
