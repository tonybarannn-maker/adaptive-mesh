#include "detail/spatial_adaptive_mesh_impl.hpp"

namespace AdaptiveMesh {

ProductionTransitionEvaluator
SpatialAdaptiveMesh::productionTransitionEvaluator() const noexcept {
    return detail::ProductionTransitionEvaluatorBindingAccess::evaluator(
        impl_->evaluationHandle());
}

} // namespace AdaptiveMesh
