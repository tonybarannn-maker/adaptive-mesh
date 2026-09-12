#include "g0_p1_lifecycle_fingerprint.hpp"

#include "detail/spatial_adaptive_mesh_impl.hpp"

#include <algorithm>
#include <stdexcept>

namespace AdaptiveMesh::detail {

class ProductionTransitionEvaluatorScenarioAccess final {
public:
    [[nodiscard]] static test_support::G0P1LifecycleFingerprint capture(
        const SpatialAdaptiveMesh& mesh,
        std::size_t source,
        std::size_t target) {
        std::shared_lock lock(mesh.impl_->topologyMutex);
        const auto found = mesh.impl_->productionRelationships_.find({source, target});
        if (found == mesh.impl_->productionRelationships_.end()) {
            throw std::logic_error("fingerprint relationship is absent");
        }

        const auto& lifecycle = found->second;
        const auto& bridges = mesh.impl_->nodes.at(source).bridges;
        const auto bridge = std::find_if(
            bridges.begin(), bridges.end(),
            [target](const SpatialBridge& candidate) {
                return candidate.targetNodeId == static_cast<int>(target);
            });
        if (bridge == bridges.end()) {
            throw std::logic_error("fingerprint bridge is absent");
        }

        return {
            source,
            target,
            lifecycle.relationshipGeneration,
            lifecycle.sourceNodeIncarnation,
            lifecycle.targetNodeIncarnation,
            lifecycle.authorityRelevantContextLineage,
            ProductionPersistenceAccess::state(lifecycle.persistence),
            ProductionPersistenceAccess::lineage(lifecycle.persistence),
            lifecycle.d7Publication.status(),
            lifecycle.d7Publication.lineage(),
            bridge->capacity,
            bridge->status};
    }
};

} // namespace AdaptiveMesh::detail

namespace AdaptiveMesh::test_support {

G0P1LifecycleFingerprint captureG0P1LifecycleFingerprint(
    const SpatialAdaptiveMesh& mesh,
    std::size_t source,
    std::size_t target) {
    return detail::ProductionTransitionEvaluatorScenarioAccess::capture(
        mesh, source, target);
}

} // namespace AdaptiveMesh::test_support
