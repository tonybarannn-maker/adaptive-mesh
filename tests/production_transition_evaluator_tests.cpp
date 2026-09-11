#include "internal/production_transition_evaluator_live_scenarios.hpp"
#include <cstdlib>
using AdaptiveMesh::test_support::LiveScenario;
using AdaptiveMesh::test_support::runLiveScenario;
int main() {
    return runLiveScenario(LiveScenario::fail_closed_binding)
        ? EXIT_SUCCESS : EXIT_FAILURE;
}
