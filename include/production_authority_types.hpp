#pragma once

#include "production_transition_eligibility.hpp"

#include <cstdint>
#include <optional>

namespace AdaptiveMesh {

namespace detail {
class MeshInitializationAccess;
class ProductionAuthorityDerivationAccess;
class ProductionTransitionCommitAccess;
}

enum class ProductionAuthorityDecision {
    denied,
    capability_issued
};

enum class ProductionAuthorityRejectionReason {
    none,
    eligibility_not_satisfied,
    binding_mismatch,
    stale_state,
    transition_class_mismatch,
    freshness_invalid,
    revalidation_failed,
    authority_policy_denied
};

enum class ProductionTransitionCommitResult {
    committed,
    stale_state,
    target_mismatch,
    capability_consumed,
    capability_invalid,
    transition_rejected
};

enum class ProductionCapabilityLifecycleState {
    issued,
    consumed,
    expired,
    invalidated
};

class AuthorityDomainIdentity final {
public:
    friend bool operator==(
        const AuthorityDomainIdentity&,
        const AuthorityDomainIdentity&) noexcept = default;

private:
    explicit constexpr AuthorityDomainIdentity(std::uint64_t token) noexcept
        : token_(token) {}

    std::uint64_t token_;

    friend class detail::MeshInitializationAccess;
    friend class detail::ProductionAuthorityDerivationAccess;
    friend class detail::ProductionTransitionCommitAccess;
};

class CapabilityId final {
public:
    friend bool operator==(
        const CapabilityId&,
        const CapabilityId&) noexcept = default;

private:
    explicit constexpr CapabilityId(std::uint64_t token) noexcept
        : token_(token) {}

    std::uint64_t token_;

    friend class detail::ProductionAuthorityDerivationAccess;
    friend class detail::ProductionTransitionCommitAccess;
};

class ProductionExecutionCapability final {
public:
    ProductionExecutionCapability(const ProductionExecutionCapability&) = delete;
    ProductionExecutionCapability& operator=(
        const ProductionExecutionCapability&) = delete;

    ProductionExecutionCapability(
        ProductionExecutionCapability&&) noexcept = default;
    ProductionExecutionCapability& operator=(
        ProductionExecutionCapability&&) noexcept = delete;

    [[nodiscard]]
    const ProductionTransitionRequestBinding& binding() const noexcept {
        return binding_;
    }

private:
    constexpr ProductionExecutionCapability(
        CapabilityId id,
        AuthorityDomainIdentity domain,
        ProductionTransitionRequestBinding binding,
        std::uint64_t issuanceEpoch) noexcept
        : id_(id),
          domain_(domain),
          binding_(binding),
          issuanceEpoch_(issuanceEpoch) {}

    CapabilityId id_;
    AuthorityDomainIdentity domain_;
    ProductionTransitionRequestBinding binding_;
    std::uint64_t issuanceEpoch_;

    friend class detail::ProductionAuthorityDerivationAccess;
    friend class detail::ProductionTransitionCommitAccess;
};

} // namespace AdaptiveMesh
