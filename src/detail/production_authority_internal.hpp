#pragma once

#include "production_authority_context.hpp"
#include "production_authority_types.hpp"

#include <atomic>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace AdaptiveMesh::detail {

namespace authority_detail {

inline std::uint64_t nextMonotonicToken(
    std::atomic<std::uint64_t>& counter,
    const char* exhaustionMessage)
{
    auto current = counter.load(std::memory_order_relaxed);
    for (;;) {
        if (current == 0 ||
            current == std::numeric_limits<std::uint64_t>::max()) {
            throw std::overflow_error(exhaustionMessage);
        }
        const auto next = current + 1;
        if (counter.compare_exchange_weak(
                current,
                next,
                std::memory_order_relaxed,
                std::memory_order_relaxed)) {
            return current;
        }
    }
}

} // namespace authority_detail

class MeshInitializationAccess final {
public:
    [[nodiscard]] static AuthorityDomainIdentity nextDomainIdentity() {
        static std::atomic<std::uint64_t> nextDomain{1};
        return AuthorityDomainIdentity{
            authority_detail::nextMonotonicToken(
                nextDomain,
                "authority domain identity space exhausted")};
    }
};

class ProductionAuthorityDerivationAccess final {
public:
    [[nodiscard]] static CapabilityId nextCapabilityId() {
        static std::atomic<std::uint64_t> nextCapability{1};
        return CapabilityId{
            authority_detail::nextMonotonicToken(
                nextCapability,
                "production capability identity space exhausted")};
    }

    [[nodiscard]] static ProductionAuthorityContext context(
        AuthorityDomainIdentity domain,
        ProductionTransitionRequestBinding binding,
        std::uint64_t authoritativeEpoch,
        bool freshnessSatisfied,
        bool revalidationSatisfied,
        bool authorityPolicySatisfied) noexcept
    {
        return ProductionAuthorityContext{
            domain,
            binding,
            authoritativeEpoch,
            freshnessSatisfied,
            revalidationSatisfied,
            authorityPolicySatisfied};
    }

    [[nodiscard]] static ProductionExecutionCapability capability(
        CapabilityId id,
        AuthorityDomainIdentity domain,
        ProductionTransitionRequestBinding binding,
        std::uint64_t issuanceEpoch) noexcept
    {
        return ProductionExecutionCapability{
            id,
            domain,
            binding,
            issuanceEpoch};
    }

    [[nodiscard]] static AuthorityDomainIdentity domain(
        const ProductionAuthorityContext& context) noexcept
    {
        return context.domain_;
    }

    [[nodiscard]] static const ProductionTransitionRequestBinding& binding(
        const ProductionAuthorityContext& context) noexcept
    {
        return context.binding_;
    }

    [[nodiscard]] static std::uint64_t authoritativeEpoch(
        const ProductionAuthorityContext& context) noexcept
    {
        return context.authoritativeEpoch_;
    }

    [[nodiscard]] static bool freshnessSatisfied(
        const ProductionAuthorityContext& context) noexcept
    {
        return context.freshnessSatisfied_;
    }

    [[nodiscard]] static bool revalidationSatisfied(
        const ProductionAuthorityContext& context) noexcept
    {
        return context.revalidationSatisfied_;
    }

    [[nodiscard]] static bool authorityPolicySatisfied(
        const ProductionAuthorityContext& context) noexcept
    {
        return context.authorityPolicySatisfied_;
    }
};

class ProductionTransitionCommitAccess final {
public:
    [[nodiscard]] static CapabilityId id(
        const ProductionExecutionCapability& capability) noexcept
    {
        return capability.id_;
    }

    [[nodiscard]] static AuthorityDomainIdentity domain(
        const ProductionExecutionCapability& capability) noexcept
    {
        return capability.domain_;
    }

    [[nodiscard]] static std::uint64_t issuanceEpoch(
        const ProductionExecutionCapability& capability) noexcept
    {
        return capability.issuanceEpoch_;
    }
};

} // namespace AdaptiveMesh::detail
