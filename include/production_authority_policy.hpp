#pragma once

#include "production_authority_context.hpp"

#include <optional>
#include <utility>

namespace AdaptiveMesh {

namespace detail {
class ProductionAuthorityLedgerAccess;
}

class ProductionAuthorityDerivationResult final {
public:
    [[nodiscard]] ProductionAuthorityDecision decision() const noexcept {
        return decision_;
    }

    [[nodiscard]]
    ProductionAuthorityRejectionReason rejectionReason() const noexcept {
        return rejectionReason_;
    }

    [[nodiscard]] bool hasCapability() const noexcept {
        return capability_.has_value();
    }

    [[nodiscard]]
    std::optional<ProductionExecutionCapability> takeCapability() && noexcept {
        return std::move(capability_);
    }

private:
    ProductionAuthorityDecision decision_ = ProductionAuthorityDecision::denied;
    ProductionAuthorityRejectionReason rejectionReason_ =
        ProductionAuthorityRejectionReason::authority_policy_denied;
    std::optional<ProductionExecutionCapability> capability_;

    friend class ProductionAuthorityDerivationPolicy;
    friend class detail::ProductionAuthorityLedgerAccess;
};

class ProductionAuthorityDerivationPolicy final {
public:
    [[nodiscard]]
    ProductionAuthorityDerivationResult evaluate(
        const ProductionTransitionEligibilityDecision& eligibility,
        const ProductionAuthorityContext& authorityContext) const;
};

} // namespace AdaptiveMesh
