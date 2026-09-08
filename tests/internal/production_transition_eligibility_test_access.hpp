#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#define private public
#include "production_transition_eligibility.hpp"
#undef private

namespace AdaptiveMesh {

class ProductionTransitionEligibilityTestAccess final {
public:
    [[nodiscard]]
    static constexpr ProductionRelationshipIdentity relationship(
        std::size_t sourceNodeId,
        std::size_t targetNodeId,
        std::uint64_t generation) noexcept {
        return ProductionRelationshipIdentity{
            sourceNodeId,
            targetNodeId,
            generation
        };
    }

    [[nodiscard]]
    static constexpr ProductionStateVersion stateVersion(
        std::uint64_t value) noexcept {
        return ProductionStateVersion{value};
    }

    [[nodiscard]]
    static constexpr ProductionTransitionClassId transitionClass(
        std::uint64_t value) noexcept {
        return ProductionTransitionClassId{value};
    }

    [[nodiscard]]
    static constexpr ProductionTransitionRequestBinding binding(
        std::size_t sourceNodeId,
        std::size_t targetNodeId,
        std::uint64_t relationshipGeneration,
        RequestedTransitionDirection direction,
        std::uint64_t transitionClassId,
        std::uint64_t stateVersionValue) noexcept {
        return ProductionTransitionRequestBinding{
            relationship(
                sourceNodeId,
                targetNodeId,
                relationshipGeneration),
            direction,
            transitionClass(transitionClassId),
            stateVersion(stateVersionValue)
        };
    }

    [[nodiscard]]
    static constexpr PermissionPrerequisiteEvidence permission(
        ProductionTransitionRequestBinding context,
        bool satisfied) noexcept {
        return PermissionPrerequisiteEvidence{context, satisfied};
    }

    [[nodiscard]]
    static constexpr InvariantPrerequisiteEvidence invariant(
        ProductionTransitionRequestBinding context,
        bool satisfied) noexcept {
        return InvariantPrerequisiteEvidence{context, satisfied};
    }

    [[nodiscard]]
    static constexpr ResiliencePrerequisiteEvidence resilience(
        ProductionTransitionRequestBinding context,
        bool satisfied) noexcept {
        return ResiliencePrerequisiteEvidence{context, satisfied};
    }

    [[nodiscard]]
    static constexpr FreshnessPrerequisiteEvidence freshness(
        ProductionTransitionRequestBinding context,
        bool satisfied) noexcept {
        return FreshnessPrerequisiteEvidence{context, satisfied};
    }

    [[nodiscard]]
    static constexpr RevalidationPrerequisiteEvidence revalidation(
        ProductionTransitionRequestBinding context,
        bool satisfied) noexcept {
        return RevalidationPrerequisiteEvidence{context, satisfied};
    }
};

} // namespace AdaptiveMesh
