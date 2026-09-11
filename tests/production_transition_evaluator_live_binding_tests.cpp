#include "internal/production_transition_evaluator_live_scenarios.hpp"
#include <array>
#include <cstdlib>
int main() {
    using AdaptiveMesh::test_support::LiveScenario;
    using AdaptiveMesh::test_support::runLiveScenario;
    constexpr std::array scenarios{
        LiveScenario::fail_closed_binding,
        LiveScenario::unchanged_persistence_current,
        LiveScenario::changed_persistence_stale,
        LiveScenario::persistence_anti_resurrection,
        LiveScenario::unrelated_relationship_isolation,
        LiveScenario::reverse_direction_isolation};
    for (const auto scenario : scenarios) {
        if (!runLiveScenario(scenario)) return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
