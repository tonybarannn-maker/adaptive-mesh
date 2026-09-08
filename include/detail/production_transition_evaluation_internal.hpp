#pragma once

#include "production_transition_eligibility.hpp"
#include "production_transition_evaluator.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace AdaptiveMesh::detail {

class CapturedRelationshipIdentity final {
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

private:
    constexpr CapturedRelationshipIdentity(
        std::size_t sourceNodeId,
        std::size_t targetNodeId,
        std::uint64_t generation) noexcept
        : sourceNodeId_(sourceNodeId),
          targetNodeId_(targetNodeId),
          generation_(generation)
    {
    }

    std::size_t sourceNodeId_;
    std::size_t targetNodeId_;
    std::uint64_t generation_;

    friend class ProductionTransitionEvaluationBackend;
};

class CapturedProductionStateVersion final {
public:
    [[nodiscard]] std::uint64_t opaqueValueForConstruction() const noexcept {
        return opaqueValue_;
    }

    friend bool operator==(
        const CapturedProductionStateVersion&,
        const CapturedProductionStateVersion&) noexcept = default;

private:
    explicit constexpr CapturedProductionStateVersion(
        std::uint64_t opaqueValue) noexcept
        : opaqueValue_(opaqueValue)
    {
    }

    std::uint64_t opaqueValue_;

    friend class ProductionTransitionEvaluationBackend;
};

class ResolvedTransitionClass final {
public:
    [[nodiscard]] std::uint64_t opaqueValueForConstruction() const noexcept {
        return opaqueValue_;
    }

private:
    explicit constexpr ResolvedTransitionClass(
        std::uint64_t opaqueValue) noexcept
        : opaqueValue_(opaqueValue)
    {
    }

    std::uint64_t opaqueValue_;

    friend class ProductionTransitionEvaluationBackend;
};

class CoherentProductionTransitionSnapshot final {
public:
    [[nodiscard]]
    const CapturedRelationshipIdentity& relationship() const noexcept {
        return relationship_;
    }

    [[nodiscard]]
    const CapturedProductionStateVersion& stateVersion() const noexcept {
        return stateVersion_;
    }

    [[nodiscard]]
    const ResolvedTransitionClass& transitionClass() const noexcept {
        return transitionClass_;
    }

    [[nodiscard]] std::uint64_t lineage() const noexcept {
        return lineage_;
    }

private:
    constexpr CoherentProductionTransitionSnapshot(
        CapturedRelationshipIdentity relationship,
        CapturedProductionStateVersion stateVersion,
        ResolvedTransitionClass transitionClass,
        std::uint64_t lineage) noexcept
        : relationship_(relationship),
          stateVersion_(stateVersion),
          transitionClass_(transitionClass),
          lineage_(lineage)
    {
    }

    CapturedRelationshipIdentity relationship_;
    CapturedProductionStateVersion stateVersion_;
    ResolvedTransitionClass transitionClass_;
    std::uint64_t lineage_;

    friend class ProductionTransitionEvaluationBackend;
};

enum class RelationshipResolutionStatus {
    resolved,
    absent,
    unavailable_or_failed
};

class RelationshipResolution final {
public:
    [[nodiscard]] RelationshipResolutionStatus status() const noexcept {
        return status_;
    }

    [[nodiscard]]
    const std::optional<CapturedRelationshipIdentity>& relationship()
        const noexcept {
        return relationship_;
    }

private:
    constexpr RelationshipResolution(
        RelationshipResolutionStatus status,
        std::optional<CapturedRelationshipIdentity> relationship) noexcept
        : status_(status),
          relationship_(relationship)
    {
    }

    RelationshipResolutionStatus status_;
    std::optional<CapturedRelationshipIdentity> relationship_;

    friend class ProductionTransitionEvaluationBackend;
};

enum class SnapshotCaptureStatus {
    complete,
    unavailable_or_failed
};

class SnapshotCaptureResult final {
public:
    [[nodiscard]] SnapshotCaptureStatus status() const noexcept {
        return status_;
    }

    [[nodiscard]]
    const std::optional<CoherentProductionTransitionSnapshot>& snapshot()
        const noexcept {
        return snapshot_;
    }

private:
    constexpr SnapshotCaptureResult(
        SnapshotCaptureStatus status,
        std::optional<CoherentProductionTransitionSnapshot> snapshot) noexcept
        : status_(status),
          snapshot_(snapshot)
    {
    }

    SnapshotCaptureStatus status_;
    std::optional<CoherentProductionTransitionSnapshot> snapshot_;

    friend class ProductionTransitionEvaluationBackend;
};

class ProductionDerivedDirection final {
public:
    [[nodiscard]] RequestedTransitionDirection direction() const noexcept {
        return direction_;
    }

    [[nodiscard]] std::uint64_t lineage() const noexcept {
        return lineage_;
    }

private:
    constexpr ProductionDerivedDirection(
        RequestedTransitionDirection direction,
        std::uint64_t lineage) noexcept
        : direction_(direction),
          lineage_(lineage)
    {
    }

    RequestedTransitionDirection direction_;
    std::uint64_t lineage_;

    friend class ProductionTransitionEvaluationBackend;
};

enum class RequestDerivationStatus {
    no_request,
    derived,
    unavailable_or_failed
};

class RequestDerivationResult final {
public:
    [[nodiscard]] RequestDerivationStatus status() const noexcept {
        return status_;
    }

    [[nodiscard]]
    const std::optional<ProductionDerivedDirection>& direction()
        const noexcept {
        return direction_;
    }

private:
    constexpr RequestDerivationResult(
        RequestDerivationStatus status,
        std::optional<ProductionDerivedDirection> direction) noexcept
        : status_(status),
          direction_(direction)
    {
    }

    RequestDerivationStatus status_;
    std::optional<ProductionDerivedDirection> direction_;

    friend class ProductionTransitionEvaluationBackend;
};

enum class DomainValidationOutcome {
    satisfied,
    not_satisfied,
    unavailable_or_failed
};

enum class FinalRevalidationOutcome {
    revalidated,
    stale,
    unavailable_or_failed
};

class ProductionTransitionEvaluationBackend {
public:
    virtual ~ProductionTransitionEvaluationBackend() = default;

    [[nodiscard]]
    virtual RelationshipResolution resolveCurrentRelationship(
        const ProductionTransitionEvaluationLocator& locator) = 0;

    [[nodiscard]]
    virtual SnapshotCaptureResult captureSnapshot(
        const CapturedRelationshipIdentity& relationship) = 0;

    [[nodiscard]]
    virtual RequestDerivationResult deriveDirection(
        const CoherentProductionTransitionSnapshot& snapshot) = 0;

    [[nodiscard]]
    virtual DomainValidationOutcome validatePermission(
        const CoherentProductionTransitionSnapshot& snapshot,
        const ProductionDerivedDirection& direction) = 0;

    [[nodiscard]]
    virtual DomainValidationOutcome validateInvariant(
        const CoherentProductionTransitionSnapshot& snapshot,
        const ProductionDerivedDirection& direction) = 0;

    [[nodiscard]]
    virtual DomainValidationOutcome validateResilience(
        const CoherentProductionTransitionSnapshot& snapshot,
        const ProductionDerivedDirection& direction) = 0;

    [[nodiscard]]
    virtual DomainValidationOutcome validateFreshness(
        const CoherentProductionTransitionSnapshot& snapshot,
        const ProductionDerivedDirection& direction) = 0;

    [[nodiscard]]
    virtual FinalRevalidationOutcome revalidate(
        const CoherentProductionTransitionSnapshot& snapshot,
        const ProductionDerivedDirection& direction) = 0;

protected:
    [[nodiscard]]
    static constexpr RelationshipResolution resolvedRelationship(
        std::size_t sourceNodeId,
        std::size_t targetNodeId,
        std::uint64_t generation) noexcept {
        return {
            RelationshipResolutionStatus::resolved,
            CapturedRelationshipIdentity{
                sourceNodeId,
                targetNodeId,
                generation
            }
        };
    }

    [[nodiscard]]
    static constexpr RelationshipResolution relationshipAbsent() noexcept {
        return {RelationshipResolutionStatus::absent, std::nullopt};
    }

    [[nodiscard]]
    static constexpr RelationshipResolution relationshipResolutionFailed()
        noexcept {
        return {
            RelationshipResolutionStatus::unavailable_or_failed,
            std::nullopt
        };
    }

    [[nodiscard]]
    static constexpr SnapshotCaptureResult completeSnapshot(
        const CapturedRelationshipIdentity& relationship,
        std::uint64_t stateVersion,
        std::uint64_t transitionClass,
        std::uint64_t lineage) noexcept {
        return {
            SnapshotCaptureStatus::complete,
            CoherentProductionTransitionSnapshot{
                relationship,
                CapturedProductionStateVersion{stateVersion},
                ResolvedTransitionClass{transitionClass},
                lineage
            }
        };
    }

    [[nodiscard]]
    static constexpr SnapshotCaptureResult snapshotCaptureFailed() noexcept {
        return {SnapshotCaptureStatus::unavailable_or_failed, std::nullopt};
    }

    [[nodiscard]]
    static constexpr RequestDerivationResult noRequest() noexcept {
        return {RequestDerivationStatus::no_request, std::nullopt};
    }

    [[nodiscard]]
    static RequestDerivationResult derivedRequest(
        const CoherentProductionTransitionSnapshot& snapshot,
        RequestedTransitionDirection direction) noexcept {
        return {
            RequestDerivationStatus::derived,
            ProductionDerivedDirection{direction, snapshot.lineage()}
        };
    }

    [[nodiscard]]
    static constexpr RequestDerivationResult requestDerivationFailed()
        noexcept {
        return {
            RequestDerivationStatus::unavailable_or_failed,
            std::nullopt
        };
    }
};

class PermissionValidationResult final {
public:
    [[nodiscard]] bool satisfied() const noexcept { return satisfied_; }
    [[nodiscard]] std::uint64_t lineage() const noexcept { return lineage_; }

private:
    constexpr PermissionValidationResult(
        bool satisfied,
        std::uint64_t lineage) noexcept
        : satisfied_(satisfied),
          lineage_(lineage)
    {
    }

    bool satisfied_;
    std::uint64_t lineage_;

    friend class PermissionDomainValidator;
    friend class ProductionTransitionConstructionAccess;
};

class InvariantValidationResult final {
public:
    [[nodiscard]] bool satisfied() const noexcept { return satisfied_; }
    [[nodiscard]] std::uint64_t lineage() const noexcept { return lineage_; }

private:
    constexpr InvariantValidationResult(
        bool satisfied,
        std::uint64_t lineage) noexcept
        : satisfied_(satisfied),
          lineage_(lineage)
    {
    }

    bool satisfied_;
    std::uint64_t lineage_;

    friend class InvariantDomainValidator;
    friend class ProductionTransitionConstructionAccess;
};

class ResilienceValidationResult final {
public:
    [[nodiscard]] bool satisfied() const noexcept { return satisfied_; }
    [[nodiscard]] std::uint64_t lineage() const noexcept { return lineage_; }

private:
    constexpr ResilienceValidationResult(
        bool satisfied,
        std::uint64_t lineage) noexcept
        : satisfied_(satisfied),
          lineage_(lineage)
    {
    }

    bool satisfied_;
    std::uint64_t lineage_;

    friend class ResilienceDomainValidator;
    friend class ProductionTransitionConstructionAccess;
};

class FreshnessValidationResult final {
public:
    [[nodiscard]] bool satisfied() const noexcept { return satisfied_; }
    [[nodiscard]] std::uint64_t lineage() const noexcept { return lineage_; }

private:
    constexpr FreshnessValidationResult(
        bool satisfied,
        std::uint64_t lineage) noexcept
        : satisfied_(satisfied),
          lineage_(lineage)
    {
    }

    bool satisfied_;
    std::uint64_t lineage_;

    friend class FreshnessDomainValidator;
    friend class ProductionTransitionConstructionAccess;
};

class RevalidationValidationResult final {
public:
    [[nodiscard]] std::uint64_t lineage() const noexcept { return lineage_; }

private:
    explicit constexpr RevalidationValidationResult(
        std::uint64_t lineage) noexcept
        : lineage_(lineage)
    {
    }

    std::uint64_t lineage_;

    friend class FinalRevalidationValidator;
    friend class ProductionTransitionConstructionAccess;
};

class PermissionDomainValidator final {
public:
    [[nodiscard]]
    static std::optional<PermissionValidationResult> evaluate(
        ProductionTransitionEvaluationBackend& backend,
        const CoherentProductionTransitionSnapshot& snapshot,
        const ProductionDerivedDirection& direction) {
        const auto outcome =
            backend.validatePermission(snapshot, direction);
        if (outcome == DomainValidationOutcome::unavailable_or_failed) {
            return std::nullopt;
        }
        return PermissionValidationResult{
            outcome == DomainValidationOutcome::satisfied,
            snapshot.lineage()
        };
    }
};

class InvariantDomainValidator final {
public:
    [[nodiscard]]
    static std::optional<InvariantValidationResult> evaluate(
        ProductionTransitionEvaluationBackend& backend,
        const CoherentProductionTransitionSnapshot& snapshot,
        const ProductionDerivedDirection& direction) {
        const auto outcome =
            backend.validateInvariant(snapshot, direction);
        if (outcome == DomainValidationOutcome::unavailable_or_failed) {
            return std::nullopt;
        }
        return InvariantValidationResult{
            outcome == DomainValidationOutcome::satisfied,
            snapshot.lineage()
        };
    }
};

class ResilienceDomainValidator final {
public:
    [[nodiscard]]
    static std::optional<ResilienceValidationResult> evaluate(
        ProductionTransitionEvaluationBackend& backend,
        const CoherentProductionTransitionSnapshot& snapshot,
        const ProductionDerivedDirection& direction) {
        const auto outcome =
            backend.validateResilience(snapshot, direction);
        if (outcome == DomainValidationOutcome::unavailable_or_failed) {
            return std::nullopt;
        }
        return ResilienceValidationResult{
            outcome == DomainValidationOutcome::satisfied,
            snapshot.lineage()
        };
    }
};

class FreshnessDomainValidator final {
public:
    [[nodiscard]]
    static std::optional<FreshnessValidationResult> evaluate(
        ProductionTransitionEvaluationBackend& backend,
        const CoherentProductionTransitionSnapshot& snapshot,
        const ProductionDerivedDirection& direction) {
        const auto outcome =
            backend.validateFreshness(snapshot, direction);
        if (outcome == DomainValidationOutcome::unavailable_or_failed) {
            return std::nullopt;
        }
        return FreshnessValidationResult{
            outcome == DomainValidationOutcome::satisfied,
            snapshot.lineage()
        };
    }
};

class FinalRevalidationValidator final {
public:
    enum class Status {
        revalidated,
        stale,
        unavailable_or_failed
    };

    class Result final {
    public:
        [[nodiscard]] Status status() const noexcept { return status_; }

        [[nodiscard]]
        const std::optional<RevalidationValidationResult>& validation()
            const noexcept {
            return validation_;
        }

    private:
        Result(
            Status status,
            std::optional<RevalidationValidationResult> validation) noexcept
            : status_(status),
              validation_(validation)
        {
        }

        Status status_;
        std::optional<RevalidationValidationResult> validation_;

        friend class FinalRevalidationValidator;
    };

    [[nodiscard]]
    static Result evaluate(
        ProductionTransitionEvaluationBackend& backend,
        const CoherentProductionTransitionSnapshot& snapshot,
        const ProductionDerivedDirection& direction) {
        switch (backend.revalidate(snapshot, direction)) {
        case FinalRevalidationOutcome::revalidated:
            return {
                Status::revalidated,
                RevalidationValidationResult{snapshot.lineage()}
            };
        case FinalRevalidationOutcome::stale:
            return {Status::stale, std::nullopt};
        case FinalRevalidationOutcome::unavailable_or_failed:
        default:
            return {Status::unavailable_or_failed, std::nullopt};
        }
    }
};

class ProductionTransitionEvaluationOrchestrator;

class ProductionTransitionConstructionAccess final {
private:
    [[nodiscard]]
    static ProductionRelationshipIdentity materializeRelationship(
        const CapturedRelationshipIdentity& relationship) noexcept {
        return ProductionRelationshipIdentity{
            relationship.sourceNodeId(),
            relationship.targetNodeId(),
            relationship.generation()
        };
    }

    [[nodiscard]]
    static ProductionStateVersion materializeStateVersion(
        const CapturedProductionStateVersion& version) noexcept {
        return ProductionStateVersion{version.opaqueValueForConstruction()};
    }

    [[nodiscard]]
    static ProductionTransitionClassId materializeTransitionClass(
        const ResolvedTransitionClass& transitionClass) noexcept {
        return ProductionTransitionClassId{
            transitionClass.opaqueValueForConstruction()
        };
    }

    [[nodiscard]]
    static ProductionTransitionRequestBinding materializeRequest(
        const CoherentProductionTransitionSnapshot& snapshot,
        const ProductionDerivedDirection& direction) noexcept {
        return ProductionTransitionRequestBinding{
            materializeRelationship(snapshot.relationship()),
            direction.direction(),
            materializeTransitionClass(snapshot.transitionClass()),
            materializeStateVersion(snapshot.stateVersion())
        };
    }

    [[nodiscard]]
    static PermissionPrerequisiteEvidence materialize(
        const ProductionTransitionRequestBinding& request,
        const PermissionValidationResult& result) noexcept {
        return PermissionPrerequisiteEvidence{request, result.satisfied()};
    }

    [[nodiscard]]
    static InvariantPrerequisiteEvidence materialize(
        const ProductionTransitionRequestBinding& request,
        const InvariantValidationResult& result) noexcept {
        return InvariantPrerequisiteEvidence{request, result.satisfied()};
    }

    [[nodiscard]]
    static ResiliencePrerequisiteEvidence materialize(
        const ProductionTransitionRequestBinding& request,
        const ResilienceValidationResult& result) noexcept {
        return ResiliencePrerequisiteEvidence{request, result.satisfied()};
    }

    [[nodiscard]]
    static FreshnessPrerequisiteEvidence materialize(
        const ProductionTransitionRequestBinding& request,
        const FreshnessValidationResult& result) noexcept {
        return FreshnessPrerequisiteEvidence{request, result.satisfied()};
    }

    [[nodiscard]]
    static RevalidationPrerequisiteEvidence materialize(
        const ProductionTransitionRequestBinding& request,
        const RevalidationValidationResult&) noexcept {
        return RevalidationPrerequisiteEvidence{request, true};
    }

    friend class ProductionTransitionEvaluationOrchestrator;
};

class ProductionTransitionEvaluationOrchestrator final {
public:
    [[nodiscard]]
    static ProductionTransitionEvaluation evaluate(
        ProductionTransitionEvaluationBackend& backend,
        const ProductionTransitionEvaluationLocator& locator) {
        if (locator.sourceNodeId == locator.targetNodeId) {
            return ProductionTransitionEvaluation::not_eligible;
        }

        const auto resolution =
            backend.resolveCurrentRelationship(locator);

        if (resolution.status() == RelationshipResolutionStatus::absent) {
            return ProductionTransitionEvaluation::no_request;
        }
        if (resolution.status() != RelationshipResolutionStatus::resolved ||
            !resolution.relationship()) {
            return ProductionTransitionEvaluation::not_eligible;
        }

        const auto capture =
            backend.captureSnapshot(*resolution.relationship());
        if (capture.status() != SnapshotCaptureStatus::complete ||
            !capture.snapshot()) {
            return ProductionTransitionEvaluation::not_eligible;
        }

        const auto& snapshot = *capture.snapshot();
        const auto derivation = backend.deriveDirection(snapshot);

        if (derivation.status() == RequestDerivationStatus::no_request) {
            return ProductionTransitionEvaluation::no_request;
        }
        if (derivation.status() != RequestDerivationStatus::derived ||
            !derivation.direction() ||
            derivation.direction()->lineage() != snapshot.lineage()) {
            return ProductionTransitionEvaluation::not_eligible;
        }

        const auto& direction = *derivation.direction();

        const auto permission =
            PermissionDomainValidator::evaluate(
                backend, snapshot, direction);
        if (!permission) {
            return ProductionTransitionEvaluation::not_eligible;
        }

        const auto invariant =
            InvariantDomainValidator::evaluate(
                backend, snapshot, direction);
        if (!invariant) {
            return ProductionTransitionEvaluation::not_eligible;
        }

        const auto resilience =
            ResilienceDomainValidator::evaluate(
                backend, snapshot, direction);
        if (!resilience) {
            return ProductionTransitionEvaluation::not_eligible;
        }

        const auto freshness =
            FreshnessDomainValidator::evaluate(
                backend, snapshot, direction);
        if (!freshness) {
            return ProductionTransitionEvaluation::not_eligible;
        }

        if (permission->lineage() != snapshot.lineage() ||
            invariant->lineage() != snapshot.lineage() ||
            resilience->lineage() != snapshot.lineage() ||
            freshness->lineage() != snapshot.lineage()) {
            return ProductionTransitionEvaluation::not_eligible;
        }

        const auto finalRevalidation =
            FinalRevalidationValidator::evaluate(
                backend, snapshot, direction);
        if (finalRevalidation.status() !=
                FinalRevalidationValidator::Status::revalidated ||
            !finalRevalidation.validation() ||
            finalRevalidation.validation()->lineage() != snapshot.lineage()) {
            return ProductionTransitionEvaluation::not_eligible;
        }

        const auto request =
            ProductionTransitionConstructionAccess::materializeRequest(
                snapshot, direction);

        ProductionTransitionPrerequisiteSet prerequisites;
        prerequisites.permission =
            ProductionTransitionConstructionAccess::materialize(
                request, *permission);
        prerequisites.invariant =
            ProductionTransitionConstructionAccess::materialize(
                request, *invariant);
        prerequisites.resilience =
            ProductionTransitionConstructionAccess::materialize(
                request, *resilience);
        prerequisites.freshness =
            ProductionTransitionConstructionAccess::materialize(
                request, *freshness);
        prerequisites.revalidation =
            ProductionTransitionConstructionAccess::materialize(
                request, *finalRevalidation.validation());

        const auto decision =
            ProductionTransitionEligibilityEvaluator{}.evaluate(
                request, prerequisites);

        return decision.eligibility() ==
                ProductionTransitionEligibility::
                    eligible_for_authority_consideration
            ? ProductionTransitionEvaluation::
                eligible_for_authority_consideration
            : ProductionTransitionEvaluation::not_eligible;
    }
};

} // namespace AdaptiveMesh::detail

namespace AdaptiveMesh {

inline ProductionTransitionEvaluation ProductionTransitionEvaluator::evaluate(
    const ProductionTransitionEvaluationLocator& locator) {
    if (backend_ == nullptr) {
        return ProductionTransitionEvaluation::not_eligible;
    }
    return detail::ProductionTransitionEvaluationOrchestrator::evaluate(
        *backend_, locator);
}

} // namespace AdaptiveMesh
