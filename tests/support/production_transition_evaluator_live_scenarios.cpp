#include "internal/production_transition_evaluator_live_scenarios.hpp"
#include "internal/production_transition_eligibility_test_access.hpp"

#include "detail/spatial_adaptive_mesh_internal_access.hpp"

#include <array>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

namespace AdaptiveMesh::detail {

class ProductionTransitionEvaluatorScenarioAccess final {
    enum class PersistenceMutation {
        evolve,
        reset
    };

    using EligibilityTestAccess = ProductionTransitionEligibilityTestAccess;

    static void populate(SpatialAdaptiveMesh& mesh) {
        mesh.addNode(0, {0.0, 0.0, 0.0}, 1.0);
        mesh.addNode(1, {1.0, 0.0, 0.0}, 1.0);
        mesh.addNode(2, {2.0, 0.0, 0.0}, 1.0);
        mesh.addNode(3, {3.0, 0.0, 0.0}, 1.0);
        mesh.connectNodes(0, 1);
        mesh.connectNodes(2, 3);
    }

    static SnapshotCaptureResult capture(
        SpatialAdaptiveMesh& mesh, std::size_t source, std::size_t target) {
        return SpatialAdaptiveMeshInternalAccess::captureProductionSnapshot(
            mesh, source, target);
    }

    static FinalRevalidationOutcome revalidate(
        SpatialAdaptiveMesh& mesh,
        const CoherentProductionTransitionSnapshot& snapshot) {
        return SpatialAdaptiveMeshInternalAccess::revalidateProductionSnapshot(
            mesh, snapshot);
    }

    static BridgePolicyEvidence supportEvidence() {
        return AdaptiveBridgePolicy{}.evaluate(
            InteractionObservation(1.0), BridgeConfidence(1.0));
    }

    static void mutatePersistence(
        SpatialAdaptiveMesh& mesh,
        std::size_t source,
        std::size_t target,
        PersistenceMutation mutation) {
        std::unique_lock lock(mesh.impl_->topologyMutex);
        const auto found = mesh.impl_->productionRelationships_.find({source, target});
        if (found == mesh.impl_->productionRelationships_.end()) return;

        if (mutation == PersistenceMutation::evolve) {
            static_cast<void>(ProductionPersistenceAccess::evolve(
                found->second.persistence, supportEvidence()));
            return;
        }

        const bool changed = ProductionPersistenceAccess::reset(
            found->second.persistence.persistence_);
        if (changed) found->second.persistence.lineage_.advance();
    }

    static ProductionExecutionCapability issueCapability(
        SpatialAdaptiveMesh& mesh,
        RequestedTransitionDirection direction = RequestedTransitionDirection::constrain,
        std::uint64_t transitionClass = 1) {
        std::unique_lock lock(mesh.impl_->topologyMutex);
        const auto found = mesh.impl_->productionRelationships_.find({0, 1});
        if (found == mesh.impl_->productionRelationships_.end()) {
            throw std::logic_error("commit scenario relationship missing");
        }
        const auto& lifecycle = found->second;
        const auto binding = EligibilityTestAccess::binding(
            0,
            1,
            lifecycle.relationshipGeneration,
            direction,
            transitionClass,
            lifecycle.authorityRelevantContextLineage);
        auto capability = ProductionAuthorityDerivationAccess::capability(
            ProductionAuthorityDerivationAccess::nextCapabilityId(),
            mesh.impl_->productionAuthorityLedger_.domain(),
            binding,
            lifecycle.authorityRelevantContextLineage);
        mesh.impl_->productionAuthorityLedger_.registerIssued(capability);
        return capability;
    }

    static std::pair<ProductionExecutionCapability, ProductionExecutionCapability>
    issueSameIdentityPair(SpatialAdaptiveMesh& mesh) {
        std::unique_lock lock(mesh.impl_->topologyMutex);
        const auto found = mesh.impl_->productionRelationships_.find({0, 1});
        if (found == mesh.impl_->productionRelationships_.end()) {
            throw std::logic_error("commit scenario relationship missing");
        }
        const auto& lifecycle = found->second;
        const auto binding = EligibilityTestAccess::binding(
            0,
            1,
            lifecycle.relationshipGeneration,
            RequestedTransitionDirection::constrain,
            1,
            lifecycle.authorityRelevantContextLineage);
        const auto id = ProductionAuthorityDerivationAccess::nextCapabilityId();
        auto first = ProductionAuthorityDerivationAccess::capability(
            id,
            mesh.impl_->productionAuthorityLedger_.domain(),
            binding,
            lifecycle.authorityRelevantContextLineage);
        auto second = ProductionAuthorityDerivationAccess::capability(
            id,
            mesh.impl_->productionAuthorityLedger_.domain(),
            binding,
            lifecycle.authorityRelevantContextLineage);
        mesh.impl_->productionAuthorityLedger_.registerIssued(first);
        return {std::move(first), std::move(second)};
    }

    static void advanceAuthorityVersion(SpatialAdaptiveMesh& mesh) {
        std::unique_lock lock(mesh.impl_->topologyMutex);
        const auto found = mesh.impl_->productionRelationships_.find({0, 1});
        if (found == mesh.impl_->productionRelationships_.end()) {
            throw std::logic_error("commit scenario relationship missing");
        }
        found->second.authorityRelevantContextLineage =
            mesh.impl_->nextAuthorityRelevantContextLineage_;
        ++mesh.impl_->nextAuthorityRelevantContextLineage_;
    }

public:
    static bool run(test_support::LiveScenario scenario) {
        using test_support::LiveScenario;
        using Outcome = FinalRevalidationOutcome;
        if (scenario == LiveScenario::fail_closed_binding) {
            SpatialAdaptiveMesh mesh;
            mesh.addNode(0, {0.0, 0.0, 0.0}, 1.0);
            mesh.addNode(1, {1.0, 0.0, 0.0}, 1.0);
            mesh.connectNodes(0, 1);
            auto evaluator = mesh.productionTransitionEvaluator();
            return evaluator.evaluate({0, 1}) == ProductionTransitionEvaluation::not_eligible;
        }

        SpatialAdaptiveMesh mesh;
        populate(mesh);
        const auto captured = capture(mesh, 0, 1);
        if (!captured.snapshot()) return false;
        if (scenario == LiveScenario::unchanged_persistence_current) {
            return revalidate(mesh, *captured.snapshot()) == Outcome::revalidated;
        }
        if (scenario == LiveScenario::changed_persistence_stale) {
            mutatePersistence(mesh, 0, 1, PersistenceMutation::evolve);
            return revalidate(mesh, *captured.snapshot()) == Outcome::stale;
        }
        if (scenario == LiveScenario::persistence_anti_resurrection) {
            mutatePersistence(mesh, 0, 1, PersistenceMutation::evolve);
            mutatePersistence(mesh, 0, 1, PersistenceMutation::evolve);
            mutatePersistence(mesh, 0, 1, PersistenceMutation::reset);
            return revalidate(mesh, *captured.snapshot()) == Outcome::stale;
        }
        if (scenario == LiveScenario::unrelated_relationship_isolation) {
            mutatePersistence(mesh, 2, 3, PersistenceMutation::evolve);
            return revalidate(mesh, *captured.snapshot()) == Outcome::revalidated;
        }
        if (scenario == LiveScenario::reverse_direction_isolation) {
            mutatePersistence(mesh, 1, 0, PersistenceMutation::evolve);
            return revalidate(mesh, *captured.snapshot()) == Outcome::revalidated;
        }
        return false;
    }

    static bool runCommit(test_support::CommitScenario scenario) {
        using test_support::CommitScenario;
        using CommitResult = ProductionTransitionCommitResult;

        if (scenario == CommitScenario::wrong_target) {
            SpatialAdaptiveMesh source;
            SpatialAdaptiveMesh other;
            populate(source);
            populate(other);
            auto capability = issueCapability(source);
            return other.commitProductionTransition(std::move(capability)) ==
                CommitResult::target_mismatch;
        }

        SpatialAdaptiveMesh mesh;
        populate(mesh);

        if (scenario == CommitScenario::successful_atomic_commit) {
            auto capability = issueCapability(mesh);
            return mesh.commitProductionTransition(std::move(capability)) ==
                CommitResult::committed;
        }

        if (scenario == CommitScenario::stale_state) {
            auto capability = issueCapability(mesh);
            advanceAuthorityVersion(mesh);
            return mesh.commitProductionTransition(std::move(capability)) ==
                CommitResult::stale_state;
        }

        if (scenario == CommitScenario::replay) {
            auto capability = issueCapability(mesh);
            if (mesh.commitProductionTransition(std::move(capability)) !=
                CommitResult::committed) {
                return false;
            }
            return mesh.commitProductionTransition(std::move(capability)) ==
                CommitResult::capability_consumed;
        }

        if (scenario == CommitScenario::wrong_transition_class) {
            auto capability = issueCapability(
                mesh,
                RequestedTransitionDirection::constrain,
                99);
            return mesh.commitProductionTransition(std::move(capability)) ==
                CommitResult::transition_rejected;
        }

        if (scenario == CommitScenario::expired_capability) {
            auto capability = issueCapability(mesh);
            const auto id = ProductionTransitionCommitAccess::id(capability);
            if (!mesh.impl_->productionAuthorityLedger_.markExpired(id)) {
                return false;
            }
            return mesh.commitProductionTransition(std::move(capability)) ==
                CommitResult::capability_invalid;
        }

        if (scenario == CommitScenario::concurrent_distinct_same_version) {
            auto first = issueCapability(mesh);
            auto second = issueCapability(mesh);
            std::array<CommitResult, 2> outcomes{
                CommitResult::capability_invalid,
                CommitResult::capability_invalid};
            std::thread firstThread([&] {
                outcomes[0] = mesh.commitProductionTransition(std::move(first));
            });
            std::thread secondThread([&] {
                outcomes[1] = mesh.commitProductionTransition(std::move(second));
            });
            firstThread.join();
            secondThread.join();
            return
                (outcomes[0] == CommitResult::committed &&
                 outcomes[1] == CommitResult::stale_state) ||
                (outcomes[1] == CommitResult::committed &&
                 outcomes[0] == CommitResult::stale_state);
        }

        if (scenario == CommitScenario::concurrent_same_capability) {
            auto pair = issueSameIdentityPair(mesh);
            std::array<CommitResult, 2> outcomes{
                CommitResult::capability_invalid,
                CommitResult::capability_invalid};
            std::thread firstThread([&] {
                outcomes[0] = mesh.commitProductionTransition(
                    std::move(pair.first));
            });
            std::thread secondThread([&] {
                outcomes[1] = mesh.commitProductionTransition(
                    std::move(pair.second));
            });
            firstThread.join();
            secondThread.join();
            return
                (outcomes[0] == CommitResult::committed &&
                 outcomes[1] == CommitResult::capability_consumed) ||
                (outcomes[1] == CommitResult::committed &&
                 outcomes[0] == CommitResult::capability_consumed);
        }

        return false;
    }
};

} // namespace AdaptiveMesh::detail

namespace AdaptiveMesh::test_support {

bool runLiveScenario(LiveScenario scenario) {
    return detail::ProductionTransitionEvaluatorScenarioAccess::run(scenario);
}

bool runCommitScenario(CommitScenario scenario) {
    return detail::ProductionTransitionEvaluatorScenarioAccess::runCommit(scenario);
}

} // namespace AdaptiveMesh::test_support
