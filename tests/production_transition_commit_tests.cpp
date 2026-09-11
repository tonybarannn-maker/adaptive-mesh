#include "internal/production_transition_evaluator_live_scenarios.hpp"

#include <cstdlib>

using namespace AdaptiveMesh;

namespace {

[[noreturn]] void failTest() noexcept {
    std::abort();
}

void require(bool condition) noexcept {
    if (!condition) failTest();
}

} // namespace

int main() {
    using test_support::CommitScenario;

    require(test_support::runCommitScenario(
        CommitScenario::successful_atomic_commit));
    require(test_support::runCommitScenario(
        CommitScenario::wrong_target));
    require(test_support::runCommitScenario(
        CommitScenario::stale_state));
    require(test_support::runCommitScenario(
        CommitScenario::replay));
    require(test_support::runCommitScenario(
        CommitScenario::wrong_transition_class));
    require(test_support::runCommitScenario(
        CommitScenario::expired_capability));
    require(test_support::runCommitScenario(
        CommitScenario::concurrent_distinct_same_version));
    require(test_support::runCommitScenario(
        CommitScenario::concurrent_same_capability));

    return 0;
}
