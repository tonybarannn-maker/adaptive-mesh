#pragma once

#include "bridge_persistence.hpp"
#include "production_transition_eligibility.hpp"
#include "production_transition_evaluator.hpp"

#include <cstddef>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

namespace AdaptiveMesh::detail {

class ProductionPersistenceAccess;
class ProductionPersistenceRecord;
class ProductionTransitionEvaluatorScenarioAccess;

struct ProductionPersistenceState final {
    double activationThreshold;
    double releaseThreshold;
    std::size_t activationSamples;
    std::size_t releaseSamples;
    std::size_t consecutiveSamples;
    std::uint8_t recommendation;
    std::uint8_t pendingDirection;

    friend bool operator==(
        const ProductionPersistenceState&,
        const ProductionPersistenceState&) noexcept = default;
};

class ProductionPersistenceLineage final {
public:
    friend bool operator==(
        const ProductionPersistenceLineage&,
        const ProductionPersistenceLineage&) noexcept = default;

private:
    struct Identity final {};

    ProductionPersistenceLineage()
        : identity_(std::make_shared<const Identity>())
    {
    }

    void advance() { identity_ = std::make_shared<const Identity>(); }

    std::shared_ptr<const Identity> identity_;

    friend class ProductionPersistenceAccess;
    friend class ProductionPersistenceRecord;
    friend class ProductionTransitionEvaluationBackend;
    friend class ProductionTransitionEvaluatorScenarioAccess;
};

enum class ProductionD7PublicationStatus : std::uint8_t {
    unpublished = 0,
    unavailable = 1,
    authoritative = 2
};

class ProductionD7PublicationRecord;

class ProductionD7PublicationLineage final {
public:
    friend bool operator==(
        const ProductionD7PublicationLineage&,
        const ProductionD7PublicationLineage&) noexcept = default;

private:
    struct Identity final {};

    ProductionD7PublicationLineage()
        : identity_(std::make_shared<const Identity>())
    {
    }

    void advance() { identity_ = std::make_shared<const Identity>(); }

    std::shared_ptr<const Identity> identity_;

    friend class ProductionD7PublicationRecord;
};

class ProductionD7PublicationRecord final {
public:
    ProductionD7PublicationRecord() = default;

    [[nodiscard]] ProductionD7PublicationStatus status() const noexcept {
        return status_;
    }

    [[nodiscard]] const ProductionD7PublicationLineage& lineage()
        const noexcept {
        return lineage_;
    }

private:
    ProductionD7PublicationStatus status_ =
        ProductionD7PublicationStatus::unpublished;
    ProductionD7PublicationLineage lineage_;
};

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

    [[nodiscard]] std::uint64_t sourceNodeIncarnation() const noexcept {
        return sourceNodeIncarnation_;
    }

    [[nodiscard]] std::uint64_t targetNodeIncarnation() const noexcept {
        return targetNodeIncarnation_;
    }

private:
    constexpr CapturedRelationshipIdentity(
        std::size_t sourceNodeId,
        std::size_t targetNodeId,
        std::uint64_t generation,
        std::uint64_t sourceNodeIncarnation,
        std::uint64_t targetNodeIncarnation) noexcept
        : sourceNodeId_(sourceNodeId),
          targetNodeId_(targetNodeId),
          generation_(generation),
          sourceNodeIncarnation_(sourceNodeIncarnation),
          targetNodeIncarnation_(targetNodeIncarnation)
    {
    }

    std::size_t sourceNodeId_;
    std::size_t targetNodeId_;
    std::uint64_t generation_;
    std::uint64_t sourceNodeIncarnation_;
    std::uint64_t targetNodeIncarnation_;

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

    [[nodiscard]] const ProductionPersistenceState& persistenceState()
        const noexcept {
        return persistenceState_;
    }

    [[nodiscard]] const ProductionPersistenceLineage& persistenceLineage()
        const noexcept {
        return persistenceLineage_;
    }

    [[nodiscard]] ProductionD7PublicationStatus d7PublicationStatus()
        const noexcept {
        return d7PublicationStatus_;
    }

    [[nodiscard]] const ProductionD7PublicationLineage& d7PublicationLineage()
        const noexcept {
        return d7PublicationLineage_;
    }

private:
    CoherentProductionTransitionSnapshot(
        CapturedRelationshipIdentity relationship,
        CapturedProductionStateVersion stateVersion,
        ResolvedTransitionClass transitionClass,
        std::uint64_t lineage,
        ProductionD7PublicationStatus d7PublicationStatus,
        ProductionD7PublicationLineage d7PublicationLineage,
        ProductionPersistenceState persistenceState,
        ProductionPersistenceLineage persistenceLineage) noexcept
        : relationship_(relationship),
          stateVersion_(stateVersion),
          transitionClass_(transitionClass),
          lineage_(lineage),
          d7PublicationStatus_(d7PublicationStatus),
          d7PublicationLineage_(std::move(d7PublicationLineage)),
          persistenceState_(persistenceState),
          persistenceLineage_(std::move(persistenceLineage))
    {
    }

    CapturedRelationshipIdentity relationship_;
    CapturedProductionStateVersion stateVersion_;
    ResolvedTransitionClass transitionClass_;
    std::uint64_t lineage_;
    ProductionD7PublicationStatus d7PublicationStatus_;
    ProductionD7PublicationLineage d7PublicationLineage_;
    ProductionPersistenceState persistenceState_;
    ProductionPersistenceLineage persistenceLineage_;

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
    friend class ProductionTransitionEvaluatorScenarioAccess;
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
        std::uint64_t generation,
        std::uint64_t sourceNodeIncarnation = 0,
        std::uint64_t targetNodeIncarnation = 0) noexcept {
        return {
            RelationshipResolutionStatus::resolved,
            CapturedRelationshipIdentity{
                sourceNodeId,
                targetNodeId,
                generation,
                sourceNodeIncarnation,
                targetNodeIncarnation
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
    static SnapshotCaptureResult completeSnapshot(
        const CapturedRelationshipIdentity& relationship,
        std::uint64_t stateVersion,
        std::uint64_t transitionClass,
        std::uint64_t lineage,
        ProductionD7PublicationStatus d7PublicationStatus,
        ProductionD7PublicationLineage d7PublicationLineage,
        ProductionPersistenceState persistenceState = {},
        ProductionPersistenceLineage persistenceLineage = {}) noexcept {
        return {
            SnapshotCaptureStatus::complete,
            CoherentProductionTransitionSnapshot{
                relationship,
                CapturedProductionStateVersion{stateVersion},
                ResolvedTransitionClass{transitionClass},
                lineage,
                d7PublicationStatus,
                std::move(d7PublicationLineage),
                persistenceState,
                std::move(persistenceLineage)
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

class BindingState final {
public:
    explicit BindingState(
        std::shared_ptr<ProductionTransitionEvaluationBackend> backend) noexcept
        : backend_(std::move(backend))
    {
    }

private:
    std::mutex mutex_;
    std::condition_variable drained_;
    std::shared_ptr<ProductionTransitionEvaluationBackend> backend_;
    std::size_t activeLeases_ = 0;
    bool acceptingLeases_ = true;

    friend class EvaluationLease;
    friend class ProductionTransitionEvaluationBindingHandle;
    friend class ProductionTransitionEvaluationBinding;
    friend class ProductionTransitionEvaluatorBindingAccess;
};

class EvaluationLease final {
public:
    EvaluationLease() noexcept = default;
    EvaluationLease(const EvaluationLease&) = delete;
    EvaluationLease& operator=(const EvaluationLease&) = delete;

    EvaluationLease(EvaluationLease&& other) noexcept
        : state_(std::move(other.state_)),
          backend_(std::move(other.backend_))
    {
    }

    EvaluationLease& operator=(EvaluationLease&& other) noexcept {
        if (this != &other) {
            release();
            state_ = std::move(other.state_);
            backend_ = std::move(other.backend_);
        }
        return *this;
    }

    ~EvaluationLease() { release(); }

    [[nodiscard]] explicit operator bool() const noexcept {
        return backend_ != nullptr;
    }

    [[nodiscard]] ProductionTransitionEvaluationBackend& backend() const {
        return *backend_;
    }

private:
    EvaluationLease(
        std::shared_ptr<BindingState> state,
        std::shared_ptr<ProductionTransitionEvaluationBackend> backend) noexcept
        : state_(std::move(state)), backend_(std::move(backend))
    {
    }

    void release() noexcept {
        if (!state_) return;
        {
            std::lock_guard lock(state_->mutex_);
            if (state_->activeLeases_ > 0) {
                --state_->activeLeases_;
            }
            if (!state_->acceptingLeases_ && state_->activeLeases_ == 0) {
                state_->drained_.notify_all();
            }
        }
        backend_.reset();
        state_.reset();
    }

    std::shared_ptr<BindingState> state_;
    std::shared_ptr<ProductionTransitionEvaluationBackend> backend_;

    friend class ProductionTransitionEvaluationBindingHandle;
};

inline EvaluationLease
ProductionTransitionEvaluationBindingHandle::acquireEvaluationLease()
    const noexcept {
    if (!state_) return {};
    std::lock_guard lock(state_->mutex_);
    if (!state_->acceptingLeases_ || !state_->backend_) return {};
    ++state_->activeLeases_;
    return EvaluationLease{state_, state_->backend_};
}

class ProductionTransitionEvaluationBinding final {
public:
    explicit ProductionTransitionEvaluationBinding(
        std::shared_ptr<ProductionTransitionEvaluationBackend> backend)
        : state_(std::make_shared<BindingState>(std::move(backend)))
    {
    }

    ProductionTransitionEvaluationBinding(
        const ProductionTransitionEvaluationBinding&) = delete;
    ProductionTransitionEvaluationBinding& operator=(
        const ProductionTransitionEvaluationBinding&) = delete;

    ~ProductionTransitionEvaluationBinding() { invalidateAndDrain(); }

    void invalidateAndDrain() noexcept {
        if (!state_) return;
        std::unique_lock lock(state_->mutex_);
        state_->acceptingLeases_ = false;
        state_->drained_.notify_all();
        state_->drained_.wait(lock, [this] {
            return state_->activeLeases_ == 0;
        });
        state_->backend_.reset();
    }

    [[nodiscard]] ProductionTransitionEvaluationBindingHandle handle()
        const noexcept {
        return ProductionTransitionEvaluationBindingHandle{state_};
    }

private:
    std::shared_ptr<BindingState> state_;

    friend class ProductionTransitionEvaluatorBindingAccess;
};

class ProductionPersistenceEvolutionResult final {
public:
    [[nodiscard]] PersistentBridgeRecommendation recommendation()
        const noexcept {
        return recommendation_;
    }

    [[nodiscard]] bool completeStateChanged() const noexcept {
        return completeStateChanged_;
    }

private:
    ProductionPersistenceEvolutionResult(
        PersistentBridgeRecommendation recommendation,
        bool completeStateChanged) noexcept
        : recommendation_(recommendation),
          completeStateChanged_(completeStateChanged)
    {
    }

    PersistentBridgeRecommendation recommendation_;
    bool completeStateChanged_;

    friend class ProductionPersistenceAccess;
};

class ProductionPersistenceRecord final {
private:
    ProductionPersistenceRecord()
        : persistence_(0.5, 0.25, 2, 2)
    {
    }

    BridgePersistence persistence_;
    ProductionPersistenceLineage lineage_;

    friend class ProductionPersistenceAccess;
    friend class ProductionTransitionEvaluatorScenarioAccess;
    friend class ::AdaptiveMesh::SpatialAdaptiveMesh;
};

class ProductionPersistenceAccess final {
public:
    [[nodiscard]] static ProductionPersistenceState state(
        const BridgePersistence& persistence) noexcept {
        const auto complete = persistence.completeState();
        return {
            complete.activationThreshold,
            complete.releaseThreshold,
            complete.activationSamples,
            complete.releaseSamples,
            complete.consecutiveSamples,
            static_cast<std::uint8_t>(complete.recommendation),
            static_cast<std::uint8_t>(complete.pendingDirection)
        };
    }

    [[nodiscard]] static ProductionPersistenceEvolutionResult evolve(
        BridgePersistence& persistence,
        const BridgePolicyEvidence& evidence) noexcept {
        const auto before = persistence.completeState();
        const auto recommendation = persistence.observe(evidence);
        return {recommendation, persistence.completeState() != before};
    }

    [[nodiscard]] static bool reset(BridgePersistence& persistence) noexcept {
        const auto before = persistence.completeState();
        persistence.reset();
        return persistence.completeState() != before;
    }

    [[nodiscard]] static ProductionPersistenceEvolutionResult evolve(
        ProductionPersistenceRecord& record,
        const BridgePolicyEvidence& evidence) {
        const auto result = evolve(record.persistence_, evidence);
        if (result.completeStateChanged()) record.lineage_.advance();
        return result;
    }

    [[nodiscard]] static ProductionPersistenceLineage lineage(
        const ProductionPersistenceRecord& record) noexcept {
        return record.lineage_;
    }

    [[nodiscard]] static ProductionPersistenceState state(
        const ProductionPersistenceRecord& record) noexcept {
        return state(record.persistence_);
    }

    friend class ProductionTransitionEvaluatorScenarioAccess;
    friend class ::AdaptiveMesh::SpatialAdaptiveMesh;
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