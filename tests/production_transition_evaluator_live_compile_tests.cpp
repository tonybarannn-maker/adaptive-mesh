#include "production_transition_evaluator.hpp"
#include "system_architecture.hpp"

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

using namespace AdaptiveMesh;

static_assert(std::is_aggregate_v<ProductionTransitionEvaluationLocator>);
static_assert(std::is_constructible_v<
    ProductionTransitionEvaluationLocator,
    std::size_t,
    std::size_t>);
static_assert(!std::is_default_constructible_v<ProductionTransitionEvaluator>);
static_assert(!std::is_copy_constructible_v<ProductionTransitionEvaluator>);
static_assert(!std::is_copy_assignable_v<ProductionTransitionEvaluator>);
static_assert(!std::is_constructible_v<
    ProductionTransitionEvaluator,
    detail::ProductionTransitionEvaluationBackend&>);
static_assert(!std::is_default_constructible_v<
    detail::ProductionTransitionEvaluationBindingHandle>);
static_assert(!std::is_constructible_v<
    detail::ProductionTransitionEvaluationBinding,
    std::shared_ptr<detail::ProductionTransitionEvaluationBackend>>);
static_assert(std::is_same_v<
    decltype(std::declval<const SpatialAdaptiveMesh&>().
        productionTransitionEvaluator()),
    ProductionTransitionEvaluator>);

int main() { return 0; }
