#pragma once

#include "production_transition_evaluator.hpp"

#include <utility>

namespace AdaptiveMesh::detail {

class ProductionTransitionEvaluatorBindingAccess final {
public:
    [[nodiscard]] static ProductionTransitionEvaluator evaluator(
        ProductionTransitionEvaluationBindingHandle binding) noexcept {
        return ProductionTransitionEvaluator{std::move(binding)};
    }
};

} // namespace AdaptiveMesh::detail
