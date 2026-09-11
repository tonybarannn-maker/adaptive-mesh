#include "detail/spatial_adaptive_mesh_impl.hpp"
#include "detail/production_transition_evaluator_binding_access.hpp"

namespace AdaptiveMesh {

ProductionTransitionEvaluator
SpatialAdaptiveMesh::productionTransitionEvaluator() const noexcept {
    return detail::ProductionTransitionEvaluatorBindingAccess::evaluator(
        impl_->evaluationHandle());
}

} // namespace AdaptiveMesh
