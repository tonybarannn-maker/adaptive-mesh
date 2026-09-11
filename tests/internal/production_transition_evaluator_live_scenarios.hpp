#pragma once

#include <cstdint>

namespace AdaptiveMesh::test_support {

enum class LiveScenario : std::uint8_t {
    fail_closed_binding,
    unchanged_persistence_current,
    changed_persistence_stale,
    persistence_anti_resurrection,
    unrelated_relationship_isolation,
    reverse_direction_isolation,
    invalidate_and_drain
};

enum class CommitScenario : std::uint8_t {
    successful_atomic_commit,
    wrong_target,
    stale_state,
    replay,
    wrong_transition_class,
    expired_capability,
    concurrent_distinct_same_version,
    concurrent_same_capability
};

[[nodiscard]] bool runLiveScenario(LiveScenario scenario);
[[nodiscard]] bool runCommitScenario(CommitScenario scenario);

} // namespace AdaptiveMesh::test_support
