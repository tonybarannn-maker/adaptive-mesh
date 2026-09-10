#include "internal/production_transition_evaluator_live_scenarios.hpp"
#include <cstdlib>
int main() {
    return AdaptiveMesh::test_support::runLiveScenario(
        AdaptiveMesh::test_support::LiveScenario::invalidate_and_drain)
        ? EXIT_SUCCESS : EXIT_FAILURE;
}
