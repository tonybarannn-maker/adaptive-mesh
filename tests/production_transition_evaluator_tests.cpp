#include "internal/production_transition_evaluator_test_access.hpp"

#include <cstdlib>

namespace {

using AdaptiveMesh::ProductionTransitionEvaluation;
using AdaptiveMesh::ProductionTransitionEvaluatorTestAccess;
using AdaptiveMesh::RequestedTransitionDirection;
using AdaptiveMesh::detail::DomainValidationOutcome;
using AdaptiveMesh::detail::FinalRevalidationOutcome;

[[noreturn]] void failTest() noexcept {
    std::abort();
}

void require(bool condition) noexcept {
    if (!condition) {
        failTest();
    }
}

ProductionTransitionEvaluation evaluateDefault(
    ProductionTransitionEvaluatorTestAccess::SyntheticBackend& backend) {
    auto evaluator =
        ProductionTransitionEvaluatorTestAccess::evaluator(backend);
    return evaluator.evaluate({1, 2});
}

} // namespace

int main() {
    using Backend =
        ProductionTransitionEvaluatorTestAccess::SyntheticBackend;

    {
        Backend backend;
        auto evaluator =
            ProductionTransitionEvaluatorTestAccess::evaluator(backend);
        require(
            evaluator.evaluate({1, 1}) ==
            ProductionTransitionEvaluation::not_eligible);
        require(backend.resolutionCalls == 0);
    }

    {
        Backend backend;
        auto evaluator =
            ProductionTransitionEvaluatorTestAccess::evaluator(backend);
        require(
            evaluator.evaluate({9, 2}) ==
            ProductionTransitionEvaluation::not_eligible);
        require(
            evaluator.evaluate({1, 9}) ==
            ProductionTransitionEvaluation::not_eligible);
        backend.resolutionFailure = true;
        require(
            evaluator.evaluate({1, 2}) ==
            ProductionTransitionEvaluation::not_eligible);
    }

    {
        Backend backend;
        backend.relationshipExists = false;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::no_request);
        require(backend.captureCalls == 0);
    }

    {
        Backend backend;
        backend.preserve = true;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::no_request);
        require(backend.permissionCalls == 0);
    }

    {
        Backend backend;
        backend.snapshotFailure = true;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::not_eligible);
    }
    {
        Backend backend;
        backend.requestFailure = true;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::not_eligible);
        require(backend.permissionCalls == 0);
    }

    {
        Backend backend;
        backend.permission =
            DomainValidationOutcome::unavailable_or_failed;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::not_eligible);
        require(backend.invariantCalls == 0);
        require(backend.revalidationCalls == 0);
    }
    {
        Backend backend;
        backend.invariant =
            DomainValidationOutcome::unavailable_or_failed;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::not_eligible);
        require(backend.resilienceCalls == 0);
        require(backend.revalidationCalls == 0);
    }
    {
        Backend backend;
        backend.resilience =
            DomainValidationOutcome::unavailable_or_failed;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::not_eligible);
        require(backend.freshnessCalls == 0);
        require(backend.revalidationCalls == 0);
    }
    {
        Backend backend;
        backend.freshness =
            DomainValidationOutcome::unavailable_or_failed;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::not_eligible);
        require(backend.revalidationCalls == 0);
    }

    {
        Backend backend;
        backend.permission = DomainValidationOutcome::not_satisfied;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::not_eligible);
        require(backend.invariantCalls == 1);
        require(backend.resilienceCalls == 1);
        require(backend.freshnessCalls == 1);
        require(backend.revalidationCalls == 1);
    }
    {
        Backend backend;
        backend.invariant = DomainValidationOutcome::not_satisfied;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::not_eligible);
        require(backend.revalidationCalls == 1);
    }
    {
        Backend backend;
        backend.resilience = DomainValidationOutcome::not_satisfied;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::not_eligible);
        require(backend.revalidationCalls == 1);
    }
    {
        Backend backend;
        backend.freshness = DomainValidationOutcome::not_satisfied;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::not_eligible);
        require(backend.revalidationCalls == 1);
    }

    {
        Backend backend;
        backend.finalRevalidation = FinalRevalidationOutcome::stale;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::not_eligible);
        require(backend.revalidationCalls == 1);
    }
    {
        Backend backend;
        backend.finalRevalidation =
            FinalRevalidationOutcome::unavailable_or_failed;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::not_eligible);
    }

    {
        Backend backend;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::
                eligible_for_authority_consideration);
        require(backend.revalidationCalls == 1);
    }

    {
        Backend backend;
        backend.direction = RequestedTransitionDirection::support;
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::
                eligible_for_authority_consideration);
    }

    {
        Backend backend;
        auto evaluator =
            ProductionTransitionEvaluatorTestAccess::evaluator(backend);
        require(
            evaluator.evaluate({1, 2}) ==
            ProductionTransitionEvaluation::
                eligible_for_authority_consideration);
        backend.preserve = true;
        require(
            evaluator.evaluate({1, 2}) ==
            ProductionTransitionEvaluation::no_request);
        require(backend.resolutionCalls == 2);
        require(backend.captureCalls == 2);
        require(backend.directionCalls == 2);
    }

    {
        Backend backend;
        backend.beforeFinalRevalidation = [&backend] {
            backend.recreateRelationship();
            backend.beforeFinalRevalidation = {};
        };
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::not_eligible);

        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::
                eligible_for_authority_consideration);
        require(backend.captureCalls == 2);
        require(backend.permissionCalls == 2);
        require(backend.invariantCalls == 2);
        require(backend.resilienceCalls == 2);
        require(backend.freshnessCalls == 2);
    }

    {
        Backend backend;
        backend.beforeFinalRevalidation = [&backend] {
            backend.changeRelevantState();
            backend.beforeFinalRevalidation = {};
        };
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::not_eligible);
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::
                eligible_for_authority_consideration);
    }

    {
        Backend backend;
        backend.beforeFinalRevalidation = [&backend] {
            backend.changeIrrelevantState();
            backend.beforeFinalRevalidation = {};
        };
        require(
            evaluateDefault(backend) ==
            ProductionTransitionEvaluation::
                eligible_for_authority_consideration);
        require(backend.irrelevantGeneration() == 1);
    }

    return 0;
}
