#pragma once

#include "detail/production_transition_evaluation_internal.hpp"
#include "system_architecture.hpp"

#include <cstddef>
#include <cstdint>

namespace AdaptiveMesh::test_support {

struct G0P1LifecycleFingerprint final {
    std::size_t sourceNodeId;
    std::size_t targetNodeId;
    std::uint64_t relationshipGeneration;
    std::uint64_t sourceNodeIncarnation;
    std::uint64_t targetNodeIncarnation;
    std::uint64_t authorityRelevantContextLineage;
    detail::ProductionPersistenceState persistenceState;
    detail::ProductionPersistenceLineage persistenceLineage;
    detail::ProductionD7PublicationStatus d7Status;
    detail::ProductionD7PublicationLineage d7Lineage;
    double bridgeCapacity;
    BridgeStatus bridgeStatus;

    friend bool operator==(
        const G0P1LifecycleFingerprint&,
        const G0P1LifecycleFingerprint&) noexcept = default;
};

[[nodiscard]] G0P1LifecycleFingerprint captureG0P1LifecycleFingerprint(
    const SpatialAdaptiveMesh& mesh,
    std::size_t source,
    std::size_t target);

} // namespace AdaptiveMesh::test_support
