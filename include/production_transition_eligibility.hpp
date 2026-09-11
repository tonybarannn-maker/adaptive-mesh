#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

namespace AdaptiveMesh {

namespace detail {
class ProductionTransitionConstructionAccess;
class ProductionAuthorityDerivationAccess;
}

enum class RequestedTransitionDirection {
    constrain,
    support
};

enum class ProductionTransitionEligibility {
    not_eligible,
    eligible_for_authority_consideration
};

enum class EligibilityRejectionReason {
    none,
    invalid_input,
    missing_prerequisite,
    binding_mismatch,
    permission_not_satisfied,
    invariant_not_satisfied,
    resilience_not_satisfied,
    freshness_not_satisfied,
    revalidation_failed
};

class ProductionRelationshipIdentity final {
public:
    [[nodiscard]] std::size_t sourceNodeId() const noexcept {
        return sourceNodeId_;
    }

    [[nodiscard]] std::size_t targetNodeId() const noexcept {
        return targetNodeId_;
    }

    [[nodiscard]] std::uint64_t generation() const noexcept {
        return generation_;
    }

    friend bool operator==(
        const ProductionRelationshipIdentity& lhs,
        const ProductionRelationshipIdentity& rhs) noexcept = default;

private:
    constexpr ProductionRelationshipIdentity(
        std::size_t sourceNodeId,
        std::size_t targetNodeId,
        std::uint64_t generation) noexcept
        : sourceNodeId_(sourceNodeId),
          targetNodeId_(targetNodeId),
          generation_(generation) {}

    std::size_t sourceNodeId_;
    std::size_t targetNodeId_;
    std::uint64_t generation_;

    friend class detail::ProductionTransitionConstructionAccess;
};

class ProductionStateVersion final {
public:
    friend bool operator==(
        const ProductionStateVersion& lhs,
        const ProductionStateVersion& rhs) noexcept = default;

private:
    explicit constexpr ProductionStateVersion(
        std::uint64_t opaqueVersion) noexcept
        : opaqueVersion_(opaqueVersion) {}

    std::uint64_t opaqueVersion_;

    friend class detail::ProductionTransitionConstructionAccess;
    friend class detail::ProductionAuthorityDerivationAccess;
};

class ProductionTransitionClassId final {
public:
    friend bool operator==(
        const ProductionTransitionClassId& lhs,
        const ProductionTransitionClassId& rhs) noexcept = default;

private:
    explicit constexpr ProductionTransitionClassId(
        std::uint64_t opaqueClassId) noexcept
        : opaqueClassId_(opaqueClassId) {}

    std::uint64_t opaqueClassId_;

    friend class detail::ProductionTransitionConstructionAccess;
    friend class detail::ProductionAuthorityDerivationAccess;
};

class ProductionTransitionRequestBinding final {
public:
    [[nodiscard]]
    const ProductionRelationshipIdentity& relationship() const noexcept {
        return relationship_;
    }

    [[nodiscard]]
    RequestedTransitionDirection direction() const noexcept {
        return direction_;
    }

    [[nodiscard]]
    const ProductionTransitionClassId& transitionClass() const noexcept {
        return transitionClass_;
    }

    [[nodiscard]]
    const ProductionStateVersion& stateVersion() const noexcept {
        return stateVersion_;
    }

    friend bool operator==(
        const ProductionTransitionRequestBinding& lhs,
        const ProductionTransitionRequestBinding& rhs) noexcept = default;

private:
    constexpr ProductionTransitionRequestBinding(
        ProductionRelationshipIdentity relationship,
        RequestedTransitionDirection direction,
        ProductionTransitionClassId transitionClass,
        ProductionStateVersion stateVersion) noexcept
        : relationship_(relationship),
          direction_(direction),
          transitionClass_(transitionClass),
          stateVersion_(stateVersion) {}

    ProductionRelationshipIdentity relationship_;
    RequestedTransitionDirection direction_;
    ProductionTransitionClassId transitionClass_;
    ProductionStateVersion stateVersion_;

    friend class detail::ProductionTransitionConstructionAccess;
};

class PermissionPrerequisiteEvidence final {
public:
    [[nodiscard]]
    const ProductionTransitionRequestBinding& context() const noexcept {
        return context_;
    }

    [[nodiscard]] bool satisfied() const noexcept {
        return satisfied_;
    }

private:
    constexpr PermissionPrerequisiteEvidence(
        ProductionTransitionRequestBinding context,
        bool satisfied) noexcept
        : context_(context),
          satisfied_(satisfied) {}

    ProductionTransitionRequestBinding context_;
    bool satisfied_;

    friend class detail::ProductionTransitionConstructionAccess;
};

class InvariantPrerequisiteEvidence final {
public:
    [[nodiscard]]
    const ProductionTransitionRequestBinding& context() const noexcept {
        return context_;
    }

    [[nodiscard]] bool satisfied() const noexcept {
        return satisfied_;
    }

private:
    constexpr InvariantPrerequisiteEvidence(
        ProductionTransitionRequestBinding context,
        bool satisfied) noexcept
        : context_(context),
          satisfied_(satisfied) {}

    ProductionTransitionRequestBinding context_;
    bool satisfied_;

    friend class detail::ProductionTransitionConstructionAccess;
};

class ResiliencePrerequisiteEvidence final {
public:
    [[nodiscard]]
    const ProductionTransitionRequestBinding& context() const noexcept {
        return context_;
    }

    [[nodiscard]] bool satisfied() const noexcept {
        return satisfied_;
    }

private:
    constexpr ResiliencePrerequisiteEvidence(
        ProductionTransitionRequestBinding context,
        bool satisfied) noexcept
        : context_(context),
          satisfied_(satisfied) {}

    ProductionTransitionRequestBinding context_;
    bool satisfied_;

    friend class detail::ProductionTransitionConstructionAccess;
};

class FreshnessPrerequisiteEvidence final {
public:
    [[nodiscard]]
    const ProductionTransitionRequestBinding& context() const noexcept {
        return context_;
    }

    [[nodiscard]] bool satisfied() const noexcept {
        return satisfied_;
    }

private:
    constexpr FreshnessPrerequisiteEvidence(
        ProductionTransitionRequestBinding context,
        bool satisfied) noexcept
        : context_(context),
          satisfied_(satisfied) {}

    ProductionTransitionRequestBinding context_;
    bool satisfied_;

    friend class detail::ProductionTransitionConstructionAccess;
};

class RevalidationPrerequisiteEvidence final {
public:
    [[nodiscard]]
    const ProductionTransitionRequestBinding& context() const noexcept {
        return context_;
    }

    [[nodiscard]] bool satisfied() const noexcept {
        return satisfied_;
    }

private:
    constexpr RevalidationPrerequisiteEvidence(
        ProductionTransitionRequestBinding context,
        bool satisfied) noexcept
        : context_(context),
          satisfied_(satisfied) {}

    ProductionTransitionRequestBinding context_;
    bool satisfied_;

    friend class detail::ProductionTransitionConstructionAccess;
};

struct ProductionTransitionPrerequisiteSet {
    std::optional<PermissionPrerequisiteEvidence> permission;
    std::optional<InvariantPrerequisiteEvidence> invariant;
    std::optional<ResiliencePrerequisiteEvidence> resilience;
    std::optional<FreshnessPrerequisiteEvidence> freshness;
    std::optional<RevalidationPrerequisiteEvidence> revalidation;
};

class ProductionTransitionEligibilityDecision final {
public:
    [[nodiscard]]
    ProductionTransitionEligibility eligibility() const noexcept {
        return eligibility_;
    }

    [[nodiscard]]
    EligibilityRejectionReason rejectionReason() const noexcept {
        return rejectionReason_;
    }

    [[nodiscard]]
    const ProductionTransitionRequestBinding& binding() const noexcept {
        return binding_;
    }

private:
    constexpr ProductionTransitionEligibilityDecision(
        ProductionTransitionEligibility eligibility,
        EligibilityRejectionReason rejectionReason,
        ProductionTransitionRequestBinding binding) noexcept
        : eligibility_(eligibility),
          rejectionReason_(rejectionReason),
          binding_(binding) {}

    ProductionTransitionEligibility eligibility_;
    EligibilityRejectionReason rejectionReason_;
    ProductionTransitionRequestBinding binding_;

    friend class ProductionTransitionEligibilityEvaluator;
};

class ProductionTransitionEligibilityEvaluator final {
public:
    [[nodiscard]]
    constexpr ProductionTransitionEligibilityDecision evaluate(
        const ProductionTransitionRequestBinding& request,
        const ProductionTransitionPrerequisiteSet& prerequisites) const noexcept {
        // Frozen D4 precedence:
        // 1 invalid_input
        // 2 missing_prerequisite
        // 3 binding_mismatch
        // 4 permission_not_satisfied
        // 5 invariant_not_satisfied
        // 6 resilience_not_satisfied
        // 7 freshness_not_satisfied
        // 8 revalidation_failed

        // No malformed request can be produced through the supported public API.
        // invalid_input is retained in the public diagnostic vocabulary for the
        // frozen contract and future restricted-origin validation boundaries.

        if (!prerequisites.permission ||
            !prerequisites.invariant ||
            !prerequisites.resilience ||
            !prerequisites.freshness ||
            !prerequisites.revalidation) {
            return {
                ProductionTransitionEligibility::not_eligible,
                EligibilityRejectionReason::missing_prerequisite,
                request
            };
        }

        if (prerequisites.permission->context() != request ||
            prerequisites.invariant->context() != request ||
            prerequisites.resilience->context() != request ||
            prerequisites.freshness->context() != request ||
            prerequisites.revalidation->context() != request) {
            return {
                ProductionTransitionEligibility::not_eligible,
                EligibilityRejectionReason::binding_mismatch,
                request
            };
        }

        if (!prerequisites.permission->satisfied()) {
            return {
                ProductionTransitionEligibility::not_eligible,
                EligibilityRejectionReason::permission_not_satisfied,
                request
            };
        }

        if (!prerequisites.invariant->satisfied()) {
            return {
                ProductionTransitionEligibility::not_eligible,
                EligibilityRejectionReason::invariant_not_satisfied,
                request
            };
        }

        if (!prerequisites.resilience->satisfied()) {
            return {
                ProductionTransitionEligibility::not_eligible,
                EligibilityRejectionReason::resilience_not_satisfied,
                request
            };
        }

        if (!prerequisites.freshness->satisfied()) {
            return {
                ProductionTransitionEligibility::not_eligible,
                EligibilityRejectionReason::freshness_not_satisfied,
                request
            };
        }

        if (!prerequisites.revalidation->satisfied()) {
            return {
                ProductionTransitionEligibility::not_eligible,
                EligibilityRejectionReason::revalidation_failed,
                request
            };
        }

        return {
            ProductionTransitionEligibility::
                eligible_for_authority_consideration,
            EligibilityRejectionReason::none,
            request
        };
    }
};

} // namespace AdaptiveMesh
