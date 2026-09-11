#include "internal/production_transition_eligibility_test_access.hpp"
#include "detail/production_authority_internal.hpp"

#include <cstdlib>
#include <cstdint>
#include <stdexcept>
#include <utility>

using namespace AdaptiveMesh;

namespace {

[[noreturn]] void failTest() noexcept {
    std::abort();
}

void require(bool condition) noexcept {
    if (!condition) failTest();
}

using TestAccess = ProductionTransitionEligibilityTestAccess;

ProductionTransitionRequestBinding makeRequest() {
    return TestAccess::binding(
        100,
        200,
        7,
        RequestedTransitionDirection::constrain,
        11,
        21);
}

ProductionTransitionPrerequisiteSet allSatisfied(
    const ProductionTransitionRequestBinding& request) {
    return {
        TestAccess::permission(request, true),
        TestAccess::invariant(request, true),
        TestAccess::resilience(request, true),
        TestAccess::freshness(request, true),
        TestAccess::revalidation(request, true)
    };
}

class SatisfiedAuthorityBackend final
    : public detail::ProductionTransitionEvaluationBackend {
public:
    [[nodiscard]] detail::RelationshipResolution resolveCurrentRelationship(
        const ProductionTransitionEvaluationLocator& locator) override {
        if (locator.sourceNodeId != 100 || locator.targetNodeId != 200) {
            return relationshipAbsent();
        }
        return resolvedRelationship(100, 200, 7, 1, 2);
    }

    [[nodiscard]] detail::SnapshotCaptureResult captureSnapshot(
        const detail::CapturedRelationshipIdentity& relationship) override {
        const detail::ProductionD7PublicationRecord publication;
        return completeSnapshot(
            relationship,
            21,
            11,
            77,
            detail::ProductionD7PublicationStatus::authoritative,
            publication.lineage());
    }

    [[nodiscard]] detail::RequestDerivationResult deriveDirection(
        const detail::CoherentProductionTransitionSnapshot& snapshot) override {
        return derivedRequest(
            snapshot,
            RequestedTransitionDirection::constrain);
    }

    [[nodiscard]] detail::DomainValidationOutcome validatePermission(
        const detail::CoherentProductionTransitionSnapshot&,
        const detail::ProductionDerivedDirection&) override {
        return detail::DomainValidationOutcome::satisfied;
    }

    [[nodiscard]] detail::DomainValidationOutcome validateInvariant(
        const detail::CoherentProductionTransitionSnapshot&,
        const detail::ProductionDerivedDirection&) override {
        return detail::DomainValidationOutcome::satisfied;
    }

    [[nodiscard]] detail::DomainValidationOutcome validateResilience(
        const detail::CoherentProductionTransitionSnapshot&,
        const detail::ProductionDerivedDirection&) override {
        return detail::DomainValidationOutcome::satisfied;
    }

    [[nodiscard]] detail::DomainValidationOutcome validateFreshness(
        const detail::CoherentProductionTransitionSnapshot&,
        const detail::ProductionDerivedDirection&) override {
        return detail::DomainValidationOutcome::satisfied;
    }

    [[nodiscard]] detail::FinalRevalidationOutcome revalidate(
        const detail::CoherentProductionTransitionSnapshot&,
        const detail::ProductionDerivedDirection&) override {
        return detail::FinalRevalidationOutcome::revalidated;
    }
};

ProductionExecutionCapability makeCapability(
    AuthorityDomainIdentity domain,
    const ProductionTransitionRequestBinding& binding,
    std::uint64_t epoch) {
    return detail::ProductionAuthorityDerivationAccess::capability(
        detail::ProductionAuthorityDerivationAccess::nextCapabilityId(),
        domain,
        binding,
        epoch);
}

} // namespace

int main() {
    const auto request = makeRequest();
    const auto prerequisites = allSatisfied(request);
    const auto eligibility =
        ProductionTransitionEligibilityEvaluator{}.evaluate(
            request,
            prerequisites);
    require(
        eligibility.eligibility() ==
        ProductionTransitionEligibility::eligible_for_authority_consideration);

    const auto domain = detail::MeshInitializationAccess::nextDomainIdentity();
    const auto otherDomain =
        detail::MeshInitializationAccess::nextDomainIdentity();
    require(domain != otherDomain);

    detail::ProductionAuthorityLedger ledger(domain);
    SatisfiedAuthorityBackend backend;

    // Successful derivation registers the newly issued capability atomically
    // with respect to the ledger's own lifecycle state.
    auto result = detail::ProductionAuthorityLiveDerivation::evaluate(
        backend,
        ledger,
        eligibility,
        true);
    require(result.decision() == ProductionAuthorityDecision::capability_issued);
    require(result.rejectionReason() == ProductionAuthorityRejectionReason::none);
    require(result.hasCapability());
    require(ledger.size() == 1);

    auto capability = std::move(result).takeCapability();
    require(capability.has_value());
    require(capability->binding() == request);
    const auto issuedId = detail::ProductionTransitionCommitAccess::id(*capability);
    require(
        ledger.lifecycleState(issuedId) ==
        ProductionCapabilityLifecycleState::issued);

    require(ledger.markConsumed(issuedId));
    require(
        ledger.lifecycleState(issuedId) ==
        ProductionCapabilityLifecycleState::consumed);
    require(!ledger.markConsumed(issuedId));
    require(!ledger.markExpired(issuedId));
    require(!ledger.invalidate(issuedId));

    // Other terminal lifecycle outcomes are one-way from ISSUED.
    auto expiring = makeCapability(domain, request, 78);
    const auto expiringId =
        detail::ProductionTransitionCommitAccess::id(expiring);
    ledger.registerIssued(expiring);
    require(ledger.markExpired(expiringId));
    require(
        ledger.lifecycleState(expiringId) ==
        ProductionCapabilityLifecycleState::expired);
    require(!ledger.invalidate(expiringId));

    auto invalidating = makeCapability(domain, request, 79);
    const auto invalidatingId =
        detail::ProductionTransitionCommitAccess::id(invalidating);
    ledger.registerIssued(invalidating);
    require(ledger.invalidate(invalidatingId));
    require(
        ledger.lifecycleState(invalidatingId) ==
        ProductionCapabilityLifecycleState::invalidated);
    require(!ledger.markConsumed(invalidatingId));

    // A capability from another mesh authority domain cannot enter this ledger.
    auto foreign = makeCapability(otherDomain, request, 80);
    bool domainRejected = false;
    try {
        ledger.registerIssued(foreign);
    } catch (const std::logic_error&) {
        domainRejected = true;
    }
    require(domainRejected);
    require(ledger.size() == 3);

    // Capability identity registration is unique and fail-closed.
    auto duplicate = makeCapability(domain, request, 81);
    ledger.registerIssued(duplicate);
    bool duplicateRejected = false;
    try {
        ledger.registerIssued(duplicate);
    } catch (const std::logic_error&) {
        duplicateRejected = true;
    }
    require(duplicateRejected);
    require(ledger.size() == 4);

    // Denial paths never create ledger records.
    auto deniedPrerequisites = allSatisfied(request);
    deniedPrerequisites.permission = TestAccess::permission(request, false);
    const auto deniedEligibility =
        ProductionTransitionEligibilityEvaluator{}.evaluate(
            request,
            deniedPrerequisites);
    const auto beforeDenied = ledger.size();
    auto denied = detail::ProductionAuthorityLiveDerivation::evaluate(
        backend,
        ledger,
        deniedEligibility,
        true);
    require(denied.decision() == ProductionAuthorityDecision::denied);
    require(!denied.hasCapability());
    require(ledger.size() == beforeDenied);

    // Authority-policy denial also leaves the ledger unchanged.
    auto policyDenied = detail::ProductionAuthorityLiveDerivation::evaluate(
        backend,
        ledger,
        eligibility,
        false);
    require(policyDenied.decision() == ProductionAuthorityDecision::denied);
    require(
        policyDenied.rejectionReason() ==
        ProductionAuthorityRejectionReason::authority_policy_denied);
    require(!policyDenied.hasCapability());
    require(ledger.size() == beforeDenied);

    return 0;
}