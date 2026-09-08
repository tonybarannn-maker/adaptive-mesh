#pragma once

#include <cstddef>

namespace AdaptiveMesh {

namespace detail {
class ProductionTransitionEvaluationBackend;
}

struct ProductionTransitionEvaluationLocator final {
    std::size_t sourceNodeId;
    std::size_t targetNodeId;
};

enum class ProductionTransitionEvaluation {
    no_request,
    not_eligible,
    eligible_for_authority_consideration
};

class ProductionTransitionEvaluator final {
public:
    ProductionTransitionEvaluator(
        const ProductionTransitionEvaluator&) = delete;

    ProductionTransitionEvaluator& operator=(
        const ProductionTransitionEvaluator&) = delete;

    [[nodiscard]]
    ProductionTransitionEvaluation evaluate(
        const ProductionTransitionEvaluationLocator& locator);

private:
    explicit ProductionTransitionEvaluator(
        detail::ProductionTransitionEvaluationBackend& backend) noexcept
        : backend_(&backend)
    {
    }

    detail::ProductionTransitionEvaluationBackend* backend_;
};

} // namespace AdaptiveMesh

#include "detail/production_transition_evaluation_internal.hpp"
