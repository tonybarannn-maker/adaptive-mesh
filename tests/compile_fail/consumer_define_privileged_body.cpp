#include "production_transition_evaluator.hpp"

AdaptiveMesh::ProductionTransitionEvaluator
AdaptiveMesh::detail::ProductionTransitionEvaluatorBindingAccess::evaluator(
    const ProductionTransitionEvaluationBinding&) noexcept {
    return *static_cast<AdaptiveMesh::ProductionTransitionEvaluator*>(nullptr);
}
