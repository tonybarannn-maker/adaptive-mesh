#define SOAM_SCENARIO_ACCESS_ENABLED 1
#define SOAM_ENABLE_SCENARIO_ACCESS 1
#define ADAPTIVE_MESH_ENABLE_SCENARIO_ACCESS 1

#include "production_transition_evaluator.hpp"

#include <memory>
#include <utility>

namespace AdaptiveMesh::detail {

class ProductionTransitionEvaluatorScenarioAccess final {
public:
    static ProductionTransitionEvaluator manufacture() {
        ProductionTransitionEvaluationBindingHandle handle{
            std::shared_ptr<BindingState>{}};
        return ProductionTransitionEvaluatorBindingAccess::evaluator(
            std::move(handle));
    }
};

} // namespace AdaptiveMesh::detail

int main() {
    auto evaluator =
        AdaptiveMesh::detail::ProductionTransitionEvaluatorScenarioAccess::manufacture();
    static_cast<void>(evaluator);
}
