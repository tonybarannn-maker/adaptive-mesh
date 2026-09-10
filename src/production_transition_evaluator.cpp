#include "detail/production_transition_evaluation_internal.hpp"

namespace AdaptiveMesh {

ProductionTransitionEvaluation ProductionTransitionEvaluator::evaluate(
    const ProductionTransitionEvaluationLocator& locator) {
    auto lease = binding_.acquireEvaluationLease();
    if (!lease) return ProductionTransitionEvaluation::not_eligible;
    return detail::ProductionTransitionEvaluationOrchestrator::evaluate(
        lease.backend(), locator);
}

} // namespace AdaptiveMesh
