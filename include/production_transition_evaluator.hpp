#pragma once

#include <cstddef>
#include <memory>

namespace AdaptiveMesh {

class ProductionTransitionEvaluator;
class SpatialAdaptiveMesh;

namespace detail {
class ProductionTransitionEvaluationBackend;
class ProductionTransitionEvaluatorBindingAccess;
class ProductionTransitionEvaluatorLiveTestAccess;
class ProductionTransitionEvaluationBinding;
class EvaluationLease;
class BindingState;

class ProductionTransitionEvaluationBindingHandle final {
public:
    ProductionTransitionEvaluationBindingHandle(
        const ProductionTransitionEvaluationBindingHandle&) noexcept = default;
    ProductionTransitionEvaluationBindingHandle& operator=(
        const ProductionTransitionEvaluationBindingHandle&) noexcept = default;

private:
    explicit ProductionTransitionEvaluationBindingHandle(
        std::shared_ptr<BindingState> state) noexcept
        : state_(std::move(state))
    {
    }

    [[nodiscard]] EvaluationLease acquireEvaluationLease() const noexcept;

    std::shared_ptr<BindingState> state_;

    friend class ProductionTransitionEvaluationBinding;
    friend class ProductionTransitionEvaluatorBindingAccess;
    friend class ::AdaptiveMesh::ProductionTransitionEvaluator;
};
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
        detail::ProductionTransitionEvaluationBindingHandle binding) noexcept
        : binding_(std::move(binding)) {}

    // Compatibility seam for the pre-I2 synthetic contract tests only. It is
    // private in supported production compilation.
    explicit ProductionTransitionEvaluator(
        detail::ProductionTransitionEvaluationBackend& backend);

    detail::ProductionTransitionEvaluationBindingHandle binding_;

    friend class detail::ProductionTransitionEvaluatorBindingAccess;
};

} // namespace AdaptiveMesh

#include "detail/production_transition_evaluation_internal.hpp"
