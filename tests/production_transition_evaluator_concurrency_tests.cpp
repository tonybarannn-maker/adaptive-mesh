#include "internal/production_transition_evaluator_test_access.hpp"

#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <thread>

namespace {

using AdaptiveMesh::ProductionTransitionEvaluation;
using AdaptiveMesh::ProductionTransitionEvaluatorTestAccess;

[[noreturn]] void failTest() noexcept { std::abort(); }
void require(bool condition) noexcept { if (!condition) failTest(); }

class RevalidationBarrier final {
public:
    void workerArriveAndWait() {
        std::unique_lock<std::mutex> lock(mutex_);
        workerArrived_ = true;
        cv_.notify_all();
        cv_.wait(lock, [this] { return resume_; });
    }

    template <typename Mutation>
    void mutateAndResume(Mutation mutation) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return workerArrived_; });
        mutation();
        resume_ = true;
        lock.unlock();
        cv_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool workerArrived_ = false;
    bool resume_ = false;
};

} // namespace

int main() {
    using Backend = ProductionTransitionEvaluatorTestAccess::SyntheticBackend;

    {
        Backend backend;
        RevalidationBarrier barrier;
        backend.beforeFinalRevalidation = [&barrier] { barrier.workerArriveAndWait(); };
        auto evaluator = ProductionTransitionEvaluatorTestAccess::evaluator(backend);
        ProductionTransitionEvaluation result = ProductionTransitionEvaluation::no_request;
        std::thread worker([&] { result = evaluator.evaluate({1, 2}); });
        barrier.mutateAndResume([&] { backend.changeRelevantState(); });
        worker.join();
        require(result == ProductionTransitionEvaluation::not_eligible);
        require(backend.revalidationCalls == 1);
    }

    {
        Backend backend;
        RevalidationBarrier barrier;
        backend.beforeFinalRevalidation = [&barrier] { barrier.workerArriveAndWait(); };
        auto evaluator = ProductionTransitionEvaluatorTestAccess::evaluator(backend);
        ProductionTransitionEvaluation result = ProductionTransitionEvaluation::no_request;
        std::thread worker([&] { result = evaluator.evaluate({1, 2}); });
        barrier.mutateAndResume([&] { backend.changeIrrelevantState(); });
        worker.join();
        require(result == ProductionTransitionEvaluation::eligible_for_authority_consideration);
        require(backend.irrelevantGeneration() == 1);
    }

    {
        Backend backend;
        RevalidationBarrier barrier;
        backend.beforeFinalRevalidation = [&barrier] { barrier.workerArriveAndWait(); };
        auto evaluator = ProductionTransitionEvaluatorTestAccess::evaluator(backend);
        ProductionTransitionEvaluation result = ProductionTransitionEvaluation::no_request;
        std::thread worker([&] { result = evaluator.evaluate({1, 2}); });
        barrier.mutateAndResume([&] { backend.recreateRelationship(); });
        worker.join();
        require(result == ProductionTransitionEvaluation::not_eligible);
    }

    {
        Backend first;
        Backend second;
        auto firstEvaluator = ProductionTransitionEvaluatorTestAccess::evaluator(first);
        auto secondEvaluator = ProductionTransitionEvaluatorTestAccess::evaluator(second);
        ProductionTransitionEvaluation firstResult = ProductionTransitionEvaluation::no_request;
        ProductionTransitionEvaluation secondResult = ProductionTransitionEvaluation::no_request;
        std::thread a([&] { firstResult = firstEvaluator.evaluate({1, 2}); });
        std::thread b([&] { secondResult = secondEvaluator.evaluate({1, 2}); });
        a.join();
        b.join();
        require(firstResult == ProductionTransitionEvaluation::eligible_for_authority_consideration);
        require(secondResult == ProductionTransitionEvaluation::eligible_for_authority_consideration);
        require(first.resolutionCalls == 1 && first.captureCalls == 1);
        require(second.resolutionCalls == 1 && second.captureCalls == 1);
    }

    return 0;
}
