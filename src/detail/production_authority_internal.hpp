#pragma once

#include "production_authority_context.hpp"
#include "production_authority_policy.hpp"
#include "production_authority_types.hpp"
#include "detail/production_transition_evaluation_internal.hpp"

#include <atomic>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

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
        bool liveBindingCurrent,
        bool stateCurrent,
        bool transitionClassCurrent,
        bool freshnessSatisfied,
        bool revalidationSatisfied,
        bool authorityPolicySatisfied) noexcept
    {
        return ProductionAuthorityContext{
            domain,
            binding,
            authoritativeEpoch,
            liveBindingCurrent,
            stateCurrent,
            transitionClassCurrent,
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

    [[nodiscard]] static bool liveBindingCurrent(
        const ProductionAuthorityContext& context) noexcept
    {
        return context.liveBindingCurrent_;
    }

    [[nodiscard]] static bool stateCurrent(
        const ProductionAuthorityContext& context) noexcept
    {
        return context.stateCurrent_;
    }

    [[nodiscard]] static bool transitionClassCurrent(
        const ProductionAuthorityContext& context) noexcept
    {
        return context.transitionClassCurrent_;
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

    [[nodiscard]] static std::uint64_t stateVersionValue(
        const ProductionStateVersion& version) noexcept
    {
        return version.opaqueVersion_;
    }

    [[nodiscard]] static std::uint64_t transitionClassValue(
        const ProductionTransitionClassId& transitionClass) noexcept
    {
        return transitionClass.opaqueClassId_;
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

    [[nodiscard]] static const ProductionTransitionRequestBinding& binding(
        const ProductionExecutionCapability& capability) noexcept
    {
        return capability.binding_;
    }

    [[nodiscard]] static std::uint64_t issuanceEpoch(
        const ProductionExecutionCapability& capability) noexcept
    {
        return capability.issuanceEpoch_;
    }

    [[nodiscard]] static std::uint64_t idValue(CapabilityId id) noexcept
    {
        return id.token_;
    }

    [[nodiscard]] static std::uint64_t domainValue(
        AuthorityDomainIdentity domain) noexcept
    {
        return domain.token_;
    }
};

struct ProductionAuthorityLedgerRecord final {
    ProductionTransitionRequestBinding binding;
    std::uint64_t issuanceEpoch;
    ProductionCapabilityLifecycleState state;
};

class ProductionAuthorityLedger final {
public:
    explicit ProductionAuthorityLedger(AuthorityDomainIdentity domain) noexcept
        : domain_(domain)
    {
    }

    ProductionAuthorityLedger(const ProductionAuthorityLedger&) = delete;
    ProductionAuthorityLedger& operator=(const ProductionAuthorityLedger&) = delete;
    ProductionAuthorityLedger(ProductionAuthorityLedger&&) = delete;
    ProductionAuthorityLedger& operator=(ProductionAuthorityLedger&&) = delete;

    [[nodiscard]] AuthorityDomainIdentity domain() const noexcept
    {
        return domain_;
    }

    void registerIssued(const ProductionExecutionCapability& capability)
    {
        if (ProductionTransitionCommitAccess::domain(capability) != domain_) {
            throw std::logic_error(
                "production capability authority domain mismatch");
        }

        std::lock_guard lock(mutex_);
        const auto key = ProductionTransitionCommitAccess::idValue(
            ProductionTransitionCommitAccess::id(capability));
        const auto inserted = records_.try_emplace(
            key,
            ProductionAuthorityLedgerRecord{
                ProductionTransitionCommitAccess::binding(capability),
                ProductionTransitionCommitAccess::issuanceEpoch(capability),
                ProductionCapabilityLifecycleState::issued});
        if (!inserted.second) {
            throw std::logic_error(
                "production capability identity already registered");
        }
    }

    [[nodiscard]] std::optional<ProductionCapabilityLifecycleState>
    lifecycleState(CapabilityId id) const
    {
        std::lock_guard lock(mutex_);
        const auto found = records_.find(
            ProductionTransitionCommitAccess::idValue(id));
        if (found == records_.end()) return std::nullopt;
        return found->second.state;
    }

    [[nodiscard]] std::size_t size() const
    {
        std::lock_guard lock(mutex_);
        return records_.size();
    }

    [[nodiscard]] bool markConsumed(CapabilityId id)
    {
        return transitionFromIssued(
            id,
            ProductionCapabilityLifecycleState::consumed);
    }

    [[nodiscard]] bool markExpired(CapabilityId id)
    {
        return transitionFromIssued(
            id,
            ProductionCapabilityLifecycleState::expired);
    }

    [[nodiscard]] bool invalidate(CapabilityId id)
    {
        return transitionFromIssued(
            id,
            ProductionCapabilityLifecycleState::invalidated);
    }

private:
    [[nodiscard]] bool transitionFromIssued(
        CapabilityId id,
        ProductionCapabilityLifecycleState terminalState)
    {
        std::lock_guard lock(mutex_);
        const auto found = records_.find(
            ProductionTransitionCommitAccess::idValue(id));
        if (found == records_.end() ||
            found->second.state != ProductionCapabilityLifecycleState::issued) {
            return false;
        }
        found->second.state = terminalState;
        return true;
    }

    AuthorityDomainIdentity domain_;
    mutable std::mutex mutex_;
    std::unordered_map<std::uint64_t, ProductionAuthorityLedgerRecord> records_;
};

class ProductionAuthorityLedgerAccess final {
public:
    static void registerIssued(
        ProductionAuthorityLedger& ledger,
        const ProductionAuthorityDerivationResult& result)
    {
        if (result.decision_ != ProductionAuthorityDecision::capability_issued ||
            !result.capability_) {
            return;
        }
        ledger.registerIssued(*result.capability_);
    }
};

class ProductionAuthorityLiveDerivation final {
public:
    [[nodiscard]] static ProductionAuthorityDerivationResult evaluate(
        ProductionTransitionEvaluationBackend& backend,
        ProductionAuthorityLedger& ledger,
        const ProductionTransitionEligibilityDecision& eligibility,
        bool authorityPolicySatisfied)
    {
        const auto& request = eligibility.binding();
        bool liveBindingCurrent = false;
        bool stateCurrent = false;
        bool transitionClassCurrent = false;
        bool freshnessSatisfied = false;
        bool revalidationSatisfied = false;
        std::uint64_t authoritativeEpoch = 0;

        if (eligibility.eligibility() ==
            ProductionTransitionEligibility::eligible_for_authority_consideration) {
            const auto& requestedRelationship = request.relationship();
            const ProductionTransitionEvaluationLocator locator{
                requestedRelationship.sourceNodeId(),
                requestedRelationship.targetNodeId()};

            const auto resolution = backend.resolveCurrentRelationship(locator);
            if (resolution.status() == RelationshipResolutionStatus::resolved &&
                resolution.relationship()) {
                const auto& liveRelationship = *resolution.relationship();
                liveBindingCurrent =
                    liveRelationship.sourceNodeId() ==
                        requestedRelationship.sourceNodeId() &&
                    liveRelationship.targetNodeId() ==
                        requestedRelationship.targetNodeId() &&
                    liveRelationship.generation() ==
                        requestedRelationship.generation();

                if (liveBindingCurrent) {
                    const auto capture = backend.captureSnapshot(liveRelationship);
                    if (capture.status() == SnapshotCaptureStatus::complete &&
                        capture.snapshot()) {
                        const auto& snapshot = *capture.snapshot();
                        authoritativeEpoch = snapshot.lineage();
                        stateCurrent =
                            snapshot.stateVersion().opaqueValueForConstruction() ==
                            ProductionAuthorityDerivationAccess::stateVersionValue(
                                request.stateVersion());
                        transitionClassCurrent =
                            snapshot.transitionClass().opaqueValueForConstruction() ==
                            ProductionAuthorityDerivationAccess::transitionClassValue(
                                request.transitionClass());

                        if (stateCurrent && transitionClassCurrent) {
                            const auto derivation = backend.deriveDirection(snapshot);
                            const bool directionCurrent =
                                derivation.status() == RequestDerivationStatus::derived &&
                                derivation.direction() &&
                                derivation.direction()->lineage() == snapshot.lineage() &&
                                derivation.direction()->direction() == request.direction();

                            if (!directionCurrent) {
                                liveBindingCurrent = false;
                            } else {
                                const auto& direction = *derivation.direction();
                                freshnessSatisfied =
                                    backend.validateFreshness(snapshot, direction) ==
                                    DomainValidationOutcome::satisfied;

                                if (freshnessSatisfied) {
                                    revalidationSatisfied =
                                        backend.revalidate(snapshot, direction) ==
                                        FinalRevalidationOutcome::revalidated;
                                }
                            }
                        }
                    }
                }
            }
        }

        auto authorityContext = ProductionAuthorityDerivationAccess::context(
            ledger.domain(),
            request,
            authoritativeEpoch,
            liveBindingCurrent,
            stateCurrent,
            transitionClassCurrent,
            freshnessSatisfied,
            revalidationSatisfied,
            authorityPolicySatisfied);

        auto result = ProductionAuthorityDerivationPolicy{}.evaluate(
            eligibility,
            authorityContext);
        ProductionAuthorityLedgerAccess::registerIssued(ledger, result);
        return result;
    }
};

} // namespace AdaptiveMesh::detail
