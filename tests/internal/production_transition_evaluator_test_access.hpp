#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>

#define private public
#include "production_transition_evaluator.hpp"
#undef private

namespace AdaptiveMesh {

class ProductionTransitionEvaluatorTestAccess final {
public:
    class SyntheticBackend final
        : public detail::ProductionTransitionEvaluationBackend {
    public:
        std::size_t expectedSourceNodeId = 1;
        std::size_t expectedTargetNodeId = 2;

        bool resolutionFailure = false;
        bool relationshipExists = true;
        bool snapshotFailure = false;
        bool requestFailure = false;
        bool preserve = false;

        std::uint64_t relationshipGeneration = 1;
        std::uint64_t stateVersion = 1;
        std::uint64_t transitionClass = 1;
        std::uint64_t lineage = 1;

        RequestedTransitionDirection direction =
            RequestedTransitionDirection::constrain;

        detail::DomainValidationOutcome permission =
            detail::DomainValidationOutcome::satisfied;
        detail::DomainValidationOutcome invariant =
            detail::DomainValidationOutcome::satisfied;
        detail::DomainValidationOutcome resilience =
            detail::DomainValidationOutcome::satisfied;
        detail::DomainValidationOutcome freshness =
            detail::DomainValidationOutcome::satisfied;

        detail::FinalRevalidationOutcome finalRevalidation =
            detail::FinalRevalidationOutcome::revalidated;

        std::size_t resolutionCalls = 0;
        std::size_t captureCalls = 0;
        std::size_t directionCalls = 0;
        std::size_t permissionCalls = 0;
        std::size_t invariantCalls = 0;
        std::size_t resilienceCalls = 0;
        std::size_t freshnessCalls = 0;
        std::size_t revalidationCalls = 0;

        std::function<void()> beforeFinalRevalidation;

        [[nodiscard]]
        detail::RelationshipResolution resolveCurrentRelationship(
            const ProductionTransitionEvaluationLocator& locator) override {
            ++resolutionCalls;
            if (resolutionFailure ||
                locator.sourceNodeId != expectedSourceNodeId ||
                locator.targetNodeId != expectedTargetNodeId) {
                return relationshipResolutionFailed();
            }
            if (!relationshipExists) {
                return relationshipAbsent();
            }
            return resolvedRelationship(
                expectedSourceNodeId,
                expectedTargetNodeId,
                relationshipGeneration);
        }

        [[nodiscard]]
        detail::SnapshotCaptureResult captureSnapshot(
            const detail::CapturedRelationshipIdentity& relationship)
            override {
            ++captureCalls;
            if (snapshotFailure ||
                !relationshipExists ||
                relationship.sourceNodeId() != expectedSourceNodeId ||
                relationship.targetNodeId() != expectedTargetNodeId ||
                relationship.generation() != relationshipGeneration) {
                return snapshotCaptureFailed();
            }
            return completeSnapshot(
                relationship,
                stateVersion,
                transitionClass,
                lineage);
        }

        [[nodiscard]]
        detail::RequestDerivationResult deriveDirection(
            const detail::CoherentProductionTransitionSnapshot& snapshot)
            override {
            ++directionCalls;
            if (requestFailure) {
                return requestDerivationFailed();
            }
            if (preserve) {
                return noRequest();
            }
            return derivedRequest(snapshot, direction);
        }

        [[nodiscard]]
        detail::DomainValidationOutcome validatePermission(
            const detail::CoherentProductionTransitionSnapshot&,
            const detail::ProductionDerivedDirection&) override {
            ++permissionCalls;
            return permission;
        }

        [[nodiscard]]
        detail::DomainValidationOutcome validateInvariant(
            const detail::CoherentProductionTransitionSnapshot&,
            const detail::ProductionDerivedDirection&) override {
            ++invariantCalls;
            return invariant;
        }

        [[nodiscard]]
        detail::DomainValidationOutcome validateResilience(
            const detail::CoherentProductionTransitionSnapshot&,
            const detail::ProductionDerivedDirection&) override {
            ++resilienceCalls;
            return resilience;
        }

        [[nodiscard]]
        detail::DomainValidationOutcome validateFreshness(
            const detail::CoherentProductionTransitionSnapshot&,
            const detail::ProductionDerivedDirection&) override {
            ++freshnessCalls;
            return freshness;
        }

        [[nodiscard]]
        detail::FinalRevalidationOutcome revalidate(
            const detail::CoherentProductionTransitionSnapshot& snapshot,
            const detail::ProductionDerivedDirection&) override {
            ++revalidationCalls;
            if (beforeFinalRevalidation) {
                beforeFinalRevalidation();
            }

            if (finalRevalidation !=
                detail::FinalRevalidationOutcome::revalidated) {
                return finalRevalidation;
            }

            if (!relationshipExists ||
                snapshot.relationship().generation() !=
                    relationshipGeneration ||
                snapshot.stateVersion().opaqueValueForConstruction() !=
                    stateVersion ||
                snapshot.transitionClass().opaqueValueForConstruction() !=
                    transitionClass ||
                snapshot.lineage() != lineage) {
                return detail::FinalRevalidationOutcome::stale;
            }

            return detail::FinalRevalidationOutcome::revalidated;
        }

        void changeRelevantState() noexcept {
            ++stateVersion;
            ++lineage;
        }

        void changeIrrelevantState() noexcept {
            ++irrelevantGeneration_;
        }

        void recreateRelationship() noexcept {
            ++relationshipGeneration;
            ++stateVersion;
            ++lineage;
            relationshipExists = true;
        }

        [[nodiscard]] std::uint64_t irrelevantGeneration() const noexcept {
            return irrelevantGeneration_;
        }

    private:
        std::uint64_t irrelevantGeneration_ = 0;
    };

    [[nodiscard]]
    static ProductionTransitionEvaluator evaluator(
        SyntheticBackend& backend) noexcept {
        return ProductionTransitionEvaluator{backend};
    }
};

} // namespace AdaptiveMesh
