#pragma once

#include "production_authority_types.hpp"

#include <cstdint>

namespace AdaptiveMesh {

namespace detail {
class ProductionAuthorityDerivationAccess;
class ProductionTransitionCommitAccess;
}

class ProductionAuthorityContext final {
public:
    ProductionAuthorityContext(const ProductionAuthorityContext&) = delete;
    ProductionAuthorityContext& operator=(
        const ProductionAuthorityContext&) = delete;

    ProductionAuthorityContext(ProductionAuthorityContext&&) noexcept = default;
    ProductionAuthorityContext& operator=(
        ProductionAuthorityContext&&) noexcept = delete;

private:
    constexpr ProductionAuthorityContext(
        AuthorityDomainIdentity domain,
        ProductionTransitionRequestBinding binding,
        std::uint64_t authoritativeEpoch,
        bool freshnessSatisfied,
        bool revalidationSatisfied,
        bool authorityPolicySatisfied) noexcept
        : domain_(domain),
          binding_(binding),
          authoritativeEpoch_(authoritativeEpoch),
          freshnessSatisfied_(freshnessSatisfied),
          revalidationSatisfied_(revalidationSatisfied),
          authorityPolicySatisfied_(authorityPolicySatisfied) {}

    AuthorityDomainIdentity domain_;
    ProductionTransitionRequestBinding binding_;
    std::uint64_t authoritativeEpoch_;
    bool freshnessSatisfied_;
    bool revalidationSatisfied_;
    bool authorityPolicySatisfied_;

    friend class detail::ProductionAuthorityDerivationAccess;
    friend class detail::ProductionTransitionCommitAccess;
};

} // namespace AdaptiveMesh
