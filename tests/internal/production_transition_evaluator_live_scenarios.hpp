#pragma once

#include <cstdint>

namespace AdaptiveMesh::test_support {

enum class LiveScenario : std::uint8_t {
    fail_closed_binding,
    persistence_change_detection,
    unchanged_persistence_current,
    changed_persistence_stale,
    persistence_anti_resurrection,
    unrelated_relationship_isolation,
    reverse_direction_isolation,
    invalidate_and_drain
};

[[nodiscard]] bool runLiveScenario(LiveScenario scenario);

} // namespace AdaptiveMesh::test_support
