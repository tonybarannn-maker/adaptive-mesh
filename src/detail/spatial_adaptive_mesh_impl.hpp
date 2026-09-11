#pragma once

#include "system_architecture.hpp"
#include "detail/production_authority_internal.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <numeric>
#include <shared_mutex>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
namespace AdaptiveMesh {

struct SpatialAdaptiveMesh::Impl {
    private:
        friend class detail::ProductionTransitionEvaluatorScenarioAccess;

        using EdgeKey = std::pair<size_t, size_t>;

        struct EdgeKeyHash {
            size_t operator()(const EdgeKey& key) const noexcept {
                const size_t h1 = std::hash<size_t>{}(key.first);
                const size_t h2 = std::hash<size_t>{}(key.second);
                return h1 ^ (h2 + static_cast<size_t>(0x9e3779b97f4a7c15ULL) + (h1 << 6) + (h1 >> 2));
            }
        };

        struct WorkerSlot final {
            size_t assignedGeneration = 0;
            size_t completedGeneration = 0;
        };

        static constexpr std::chrono::milliseconds
            coordinationRecheckInterval{1};

        struct FirstBridgeState {
            double capacity;
            BridgeStatus status;
        };

        struct ProductionRelationshipLifecycle final {
            std::uint64_t sourceNodeIncarnation;
            std::uint64_t targetNodeIncarnation;
            std::uint64_t relationshipGeneration;
            std::uint64_t authorityRelevantContextLineage;
            detail::ProductionPersistenceRecord persistence;
        };

        class LiveProductionTransitionEvaluationBackend final
            : public detail::ProductionTransitionEvaluationBackend {
        public:
            explicit LiveProductionTransitionEvaluationBackend(
                Impl& owner) noexcept
                : owner_(owner)
            {
            }

            [[nodiscard]] detail::RelationshipResolution
            resolveCurrentRelationship(
                const ProductionTransitionEvaluationLocator& locator)
                override {
                std::shared_lock lock(owner_.topologyMutex);
                const auto found = owner_.productionRelationships_.find(
                    EdgeKey{locator.sourceNodeId, locator.targetNodeId});
                if (found == owner_.productionRelationships_.end()) {
                    return relationshipAbsent();
                }
                const auto& current = found->second;
                return resolvedRelationship(
                    locator.sourceNodeId,
                    locator.targetNodeId,
                    current.relationshipGeneration,
                    current.sourceNodeIncarnation,
                    current.targetNodeIncarnation);
            }

            [[nodiscard]] detail::SnapshotCaptureResult captureSnapshot(
                const detail::CapturedRelationshipIdentity& relationship)
                override {
                std::shared_lock lock(owner_.topologyMutex);
                const auto* current = owner_.findLifecycleUnlocked(relationship);
                if (current == nullptr) return snapshotCaptureFailed();
                return completeSnapshot(
                    relationship,
                    current->authorityRelevantContextLineage,
                    transitionContractIdentity_,
                    current->authorityRelevantContextLineage,
                    detail::ProductionPersistenceAccess::state(
                        current->persistence),
                    detail::ProductionPersistenceAccess::lineage(
                        current->persistence));
            }

            [[nodiscard]] detail::RequestDerivationResult deriveDirection(
                const detail::CoherentProductionTransitionSnapshot& snapshot)
                override {
                std::shared_lock lock(owner_.topologyMutex);
                const auto* current =
                    owner_.findLifecycleUnlocked(snapshot.relationship());
                if (current == nullptr ||
                    current->authorityRelevantContextLineage !=
                        snapshot.lineage()) {
                    return requestDerivationFailed();
                }
                // D7 production observation/confidence provenance is not
                // available in this authorized slice, so no production-native
                // request direction may be derived yet.
                return requestDerivationFailed();
            }

            [[nodiscard]] detail::DomainValidationOutcome validatePermission(
                const detail::CoherentProductionTransitionSnapshot&,
                const detail::ProductionDerivedDirection&) override {
                return detail::DomainValidationOutcome::unavailable_or_failed;
            }

            [[nodiscard]] detail::DomainValidationOutcome validateInvariant(
                const detail::CoherentProductionTransitionSnapshot&,
                const detail::ProductionDerivedDirection&) override {
                return detail::DomainValidationOutcome::unavailable_or_failed;
            }

            [[nodiscard]] detail::DomainValidationOutcome validateResilience(
                const detail::CoherentProductionTransitionSnapshot&,
                const detail::ProductionDerivedDirection&) override {
                return detail::DomainValidationOutcome::unavailable_or_failed;
            }

            [[nodiscard]] detail::DomainValidationOutcome validateFreshness(
                const detail::CoherentProductionTransitionSnapshot&,
                const detail::ProductionDerivedDirection&) override {
                return detail::DomainValidationOutcome::unavailable_or_failed;
            }

            [[nodiscard]] detail::FinalRevalidationOutcome revalidate(
                const detail::CoherentProductionTransitionSnapshot& snapshot,
                const detail::ProductionDerivedDirection&) override {
                std::shared_lock lock(owner_.topologyMutex);
                const auto* current =
                    owner_.findLifecycleUnlocked(snapshot.relationship());
                if (current == nullptr ||
                    current->authorityRelevantContextLineage !=
                        snapshot.lineage() ||
                    detail::ProductionPersistenceAccess::state(
                        current->persistence) !=
                        snapshot.persistenceState() ||
                    detail::ProductionPersistenceAccess::lineage(
                        current->persistence) !=
                        snapshot.persistenceLineage()) {
                    return detail::FinalRevalidationOutcome::stale;
                }
                return detail::FinalRevalidationOutcome::revalidated;
            }

        private:
            static constexpr std::uint64_t transitionContractIdentity_ = 1;
            Impl& owner_;
        };

        std::vector<AutopoieticNode> nodes;
        std::vector<std::uint64_t> nodeIncarnations_;
        std::unordered_map<
            EdgeKey,
            ProductionRelationshipLifecycle,
            EdgeKeyHash> productionRelationships_;
        std::uint64_t nextNodeIncarnation_ = 1;
        std::uint64_t nextRelationshipGeneration_ = 1;
        std::uint64_t nextAuthorityRelevantContextLineage_ = 1;
        double alpha = 0.15;
        mutable std::shared_mutex topologyMutex;
        const size_t workerLimit;
        std::vector<std::unique_ptr<WorkerSlot>> workerSlots;
        std::vector<std::jthread> workers;
        std::mutex workMutex;
        std::condition_variable workAvailable;
        std::condition_variable workCompleted;
        size_t workGeneration = 0;
        size_t activeNodeCount = 0;
        size_t activeWorkerCount = 0;
        size_t remainingWorkerCount = 0;
        std::vector<double>* dispatchedStates = nullptr;
        std::vector<std::vector<double>>* dispatchedBridgeCapacities = nullptr;
        std::vector<std::vector<BridgeStatus>>* dispatchedBridgeStatuses = nullptr;
        std::vector<std::vector<std::uint8_t>>* dispatchedBridgeChanged = nullptr;
        std::vector<double> computedStates;
        std::vector<std::vector<double>> pendingBridgeCapacities;
        std::vector<std::vector<BridgeStatus>> pendingBridgeStatuses;
        std::vector<std::vector<std::uint8_t>> pendingBridgeChanged;
        bool stoppingWorkers = false;
        std::exception_ptr workerException;
        bool simulationBufferShapeDirty = true;
        bool topologyValidationRequired = true;
        detail::ProductionAuthorityLedger productionAuthorityLedger_;
        std::unique_ptr<detail::ProductionTransitionEvaluationBinding>
            productionTransitionEvaluationBinding_;
#if SOAM_PHASE_PROFILE_ENABLED
        SimulationPhaseProfile lastSimulationPhaseProfile{};
#endif

        void validateNodeIndex(int nodeId) const {
            if (nodeId < 0 || nodeId >= static_cast<int>(nodes.size())) {
                throw std::out_of_range("node index is out of range");
            }
        }

        void enforceStabilityConditionUnlocked() noexcept {
            size_t maxDegree = 0;
            for (const auto& node : nodes) {
                maxDegree = std::max(maxDegree, node.bridges.size());
            }
            if (maxDegree > 0) {
                double safeAlpha = 1.0 / static_cast<double>(maxDegree);
                alpha = std::min(0.2, safeAlpha * 0.8);
            } else {
                alpha = 0.15;
            }
        }

        // Caller holds topologyMutex exclusively throughout preparation/publication.
        // Provisional map entries are rolled back on failure and cannot be observed.
        void connectPairsUnlocked(const std::vector<std::pair<int, int>>& pairs) {
            struct PendingEdge {
                EdgeKey key;
                SpatialBridge bridge;
                ProductionRelationshipLifecycle lifecycle;
            };
            std::vector<PendingEdge> pending;
            std::unordered_map<size_t, size_t> additions;
            auto generation = nextRelationshipGeneration_;
            auto lineage = nextAuthorityRelevantContextLineage_;
            for (const auto& [a, b] : pairs) {
                const auto source = static_cast<size_t>(a);
                const auto target = static_cast<size_t>(b);
                const double distance = nodes[source].position.distanceTo(nodes[target].position);
                const double forward = nodes[source].position.orientationFactorTo(nodes[target].position);
                const double reverse = nodes[target].position.orientationFactorTo(nodes[source].position);
                pending.push_back({{source, target}, {b, distance, forward, 1.0, BridgeStatus::NORMAL},
                    {nodeIncarnations_.at(source), nodeIncarnations_.at(target),
                     generation++, lineage++, detail::ProductionPersistenceRecord{}}});
                pending.push_back({{target, source}, {a, distance, reverse, 1.0, BridgeStatus::NORMAL},
                    {nodeIncarnations_.at(target), nodeIncarnations_.at(source),
                     generation++, lineage++, detail::ProductionPersistenceRecord{}}});
                ++additions[source];
                ++additions[target];
            }
            // Reserve geometrically: repeated single-edge calls retain amortized growth.
            for (const auto& [id, count] : additions) {
                auto& bridges = nodes[id].bridges;
                const auto required = bridges.size() + count;
                if (required > bridges.capacity()) {
                    bridges.reserve(std::max(required, bridges.capacity() * 2));
                }
            }
            std::vector<EdgeKey> inserted;
            inserted.reserve(pending.size());
            try {
                for (auto& edge : pending) {
                    const auto result = productionRelationships_.try_emplace(
                        edge.key, std::move(edge.lifecycle));
                    if (!result.second) {
                        throw std::logic_error("relationship already exists during connection preparation");
                    }
                    inserted.push_back(edge.key); // reserved, trivial value
                }
            } catch (...) {
                for (const auto& key : inserted) productionRelationships_.erase(key);
                throw;
            }
            // Publication: capacity is sufficient; SpatialBridge is a trivial value.
            static_assert(std::is_nothrow_copy_constructible_v<SpatialBridge>);
            for (const auto& edge : pending) nodes[edge.key.first].bridges.push_back(edge.bridge);
            nextRelationshipGeneration_ = generation;
            nextAuthorityRelevantContextLineage_ = lineage;
            if (!pairs.empty()) {
                enforceStabilityConditionUnlocked();
                simulationBufferShapeDirty = true;
                topologyValidationRequired = true;
            }
        }

        [[nodiscard]] const ProductionRelationshipLifecycle*
        findLifecycleUnlocked(
            const detail::CapturedRelationshipIdentity& relationship) const {
            const auto found = productionRelationships_.find(
                EdgeKey{
                    relationship.sourceNodeId(),
                    relationship.targetNodeId()});
            if (found == productionRelationships_.end()) return nullptr;
            const auto& current = found->second;
            if (current.relationshipGeneration != relationship.generation() ||
                current.sourceNodeIncarnation !=
                    relationship.sourceNodeIncarnation() ||
                current.targetNodeIncarnation !=
                    relationship.targetNodeIncarnation()) {
                return nullptr;
            }
            return &current;
        }

        void discardAbsentProductionRelationshipsUnlocked() {
            std::erase_if(
                productionRelationships_,
                [this](const auto& item) {
                    const auto sourceNodeId = item.first.first;
                    const auto targetNodeId = item.first.second;
                    if (sourceNodeId >= nodes.size()) return true;
                    const auto& bridges = nodes[sourceNodeId].bridges;
                    return std::none_of(
                        bridges.begin(),
                        bridges.end(),
                        [targetNodeId](const SpatialBridge& bridge) {
                            return bridge.targetNodeId ==
                                static_cast<int>(targetNodeId);
                        });
                });
        }

        [[nodiscard]] size_t resolveWorkerCountUnlocked() const noexcept {
            if (nodes.empty()) return 0;
            const size_t hardwareWorkers = std::max(size_t{1}, static_cast<size_t>(std::thread::hardware_concurrency()));
            const size_t requestedWorkers = workerLimit == 0 ? hardwareWorkers : workerLimit;
            return std::min(requestedWorkers, nodes.size());
        }

        [[nodiscard]] static double computeEffectiveCouplingUnchecked(
            const SpatialBridge& bridge) noexcept
        {
            const double spatialAttenuation =
                1.0 / (1.0 + 0.1 * bridge.distance);
            return bridge.capacity * spatialAttenuation *
                   bridge.orientationWeight;
        }

        static void updateHealthUnchecked(
            AutopoieticNode& node,
            double currentState) noexcept
        {
            const double drift =
                std::abs(currentState - node.invariant.baseline);

            node.healthIndex.store(
                std::max(
                    0.0,
                    1.0 - (drift / node.invariant.maxEpsilon)));
        }

        [[nodiscard]] static SignalCategory evaluateSignalUncheckedInvariant(
            double candidateState,
            double healthIndex,
            const IdentityInvariant& invariant)
        {
            requireFinite(candidateState, "candidateState");

            const double deltaFromBase =
                std::abs(candidateState - invariant.baseline);

            if (deltaFromBase > invariant.maxEpsilon) {
                return SignalCategory::DESTRUCTIVE_DRIFT;
            }

            if (healthIndex > 0.7 && deltaFromBase > 1.2) {
                return SignalCategory::CREATIVE_SIGNAL;
            }

            return SignalCategory::NOISE;
        }

        void runNodeRange(size_t firstNode, size_t lastNode,
                          std::vector<double>& outputStates,
                          std::vector<std::vector<double>>& bridgeCapacityOutput,
                          std::vector<std::vector<BridgeStatus>>& bridgeStatusOutput,
                          std::vector<std::vector<std::uint8_t>>& bridgeChangedOutput)
        {
            for (size_t i = firstNode; i < lastNode; ++i) {
                auto& node = nodes[i];
                double diffusionSum = 0.0;
                double currentState = node.state.load();
                for (size_t bridgeIndex = 0; bridgeIndex < node.bridges.size(); ++bridgeIndex) {
                    const auto& bridge = node.bridges[bridgeIndex];
                    auto& neighbor = nodes[static_cast<size_t>(bridge.targetNodeId)];
                    double neighborState = neighbor.state.load();
                    double deltaS = neighborState - currentState;
                    SignalCategory category = evaluateSignalUncheckedInvariant(
                        currentState + deltaS,
                        node.healthIndex.load(),
                        node.invariant);
                    SpatialBridge nextBridge = bridge;
                    nextBridge.updateBridgeState(category);
                    if (!std::isfinite(nextBridge.capacity)) {
                        throw std::runtime_error(
                            "simulation produced a non-finite bridge capacity");
                    }
                    const bool bridgeChanged =
                        bridge.capacity != nextBridge.capacity ||
                        bridge.status != nextBridge.status;
                    bridgeChangedOutput[i][bridgeIndex] =
                        static_cast<std::uint8_t>(bridgeChanged);
                    if (bridgeChanged) {
                        bridgeCapacityOutput[i][bridgeIndex] = nextBridge.capacity;
                        bridgeStatusOutput[i][bridgeIndex] = nextBridge.status;
                    }
                    diffusionSum +=
                        computeEffectiveCouplingUnchecked(nextBridge) * deltaS;
                }
                outputStates[i] = currentState + alpha * diffusionSum;
            }
        }

        void acknowledgeWorkerCompletion(
            size_t workerId,
            WorkerSlot& slot,
            size_t generation) noexcept
        {
            if (generation != workGeneration ||
                workerId >= activeWorkerCount ||
                generation != slot.assignedGeneration ||
                slot.completedGeneration >= generation ||
                remainingWorkerCount == 0) {
                return;
            }
            slot.completedGeneration = generation;
            --remainingWorkerCount;
            if (remainingWorkerCount == 0) workCompleted.notify_one();
        }

        void workerLoop(size_t workerId, WorkerSlot& slot) {
            std::unique_lock lock(workMutex);
            while (true) {
                workAvailable.wait_for(
                    lock,
                    coordinationRecheckInterval,
                    [this, &slot] {
                        return stoppingWorkers ||
                            slot.assignedGeneration >
                                slot.completedGeneration;
                    });
                if (slot.assignedGeneration <= slot.completedGeneration) {
                    if (stoppingWorkers) return;
                    continue;
                }

                const size_t observedGeneration = slot.assignedGeneration;
                const size_t workerCount = activeWorkerCount;
                const size_t nodeCount = activeNodeCount;
                auto* outputStates = dispatchedStates;
                auto* bridgeCapacities = dispatchedBridgeCapacities;
                auto* bridgeStatuses = dispatchedBridgeStatuses;
                auto* bridgeChanged = dispatchedBridgeChanged;
                const size_t firstNode = workerId * nodeCount / workerCount;
                const size_t lastNode = (workerId + 1) * nodeCount / workerCount;
                lock.unlock();
                std::exception_ptr error;
                try {
                    runNodeRange(firstNode, lastNode, *outputStates, *bridgeCapacities, *bridgeStatuses, *bridgeChanged);
                } catch (...) {
                    error = std::current_exception();
                }
                lock.lock();
                if (error && !workerException) workerException = error;
                acknowledgeWorkerCompletion(
                    workerId,
                    slot,
                    observedGeneration);
            }
        }

        void ensureWorkerPoolUnlocked() {
            const size_t requiredWorkerCount = resolveWorkerCountUnlocked();
            workers.reserve(requiredWorkerCount);
            workerSlots.reserve(requiredWorkerCount);
            for (size_t workerId = workers.size(); workerId < requiredWorkerCount; ++workerId) {
                auto slot = std::make_unique<WorkerSlot>();
                slot->assignedGeneration = workGeneration;
                slot->completedGeneration = workGeneration;
                WorkerSlot* const slotAddress = slot.get();
                workerSlots.push_back(std::move(slot));
                try {
                    workers.emplace_back([this, workerId, slotAddress] {
                        workerLoop(workerId, *slotAddress);
                    });
                } catch (...) {
                    workerSlots.pop_back();
                    throw;
                }
            }
        }

        void stopWorkerPool() noexcept {
            {
                std::lock_guard lock(workMutex);
                stoppingWorkers = true;
            }
            workAvailable.notify_all();
            workers.clear();
            workerSlots.clear();
        }

        void validateTopologyUnlocked() const {
            if (!std::isfinite(alpha)) {
                throw std::runtime_error("simulation alpha must be finite");
            }

            std::unordered_set<EdgeKey, EdgeKeyHash> directedEdges;
            size_t edgeCount = 0;
            for (const auto& node : nodes) {
                edgeCount += node.bridges.size();
            }
            directedEdges.reserve(edgeCount);

            for (size_t sourceNodeId = 0; sourceNodeId < nodes.size(); ++sourceNodeId) {
                const auto& node = nodes[sourceNodeId];
                try {
                    node.position.validate();
                    node.invariant.validate();
                } catch (const std::invalid_argument& error) {
                    throw std::runtime_error(
                        std::string{"invalid node invariant: "} + error.what());
                }

                for (const auto& bridge : node.bridges) {
                    if (bridge.targetNodeId < 0 ||
                        bridge.targetNodeId >= static_cast<int>(nodes.size()) ||
                        bridge.targetNodeId == static_cast<int>(sourceNodeId)) {
                        throw std::runtime_error(
                            "bridge target violates topology invariant");
                    }

                    if (!std::isfinite(bridge.distance) ||
                        !std::isfinite(bridge.orientationWeight)) {
                        throw std::runtime_error(
                            "bridge geometry values must be finite");
                    }

                    directedEdges.emplace(
                        sourceNodeId,
                        static_cast<size_t>(bridge.targetNodeId));
                }
            }

            for (size_t sourceNodeId = 0; sourceNodeId < nodes.size(); ++sourceNodeId) {
                for (const auto& bridge : nodes[sourceNodeId].bridges) {
                    const EdgeKey reverseKey{
                        static_cast<size_t>(bridge.targetNodeId),
                        sourceNodeId
                    };

                    if (!directedEdges.contains(reverseKey)) {
                        throw std::runtime_error(
                            "bridge pair invariant is violated");
                    }
                }
            }
        }

        void validateDynamicStateUnlocked() const {
            if (!std::isfinite(alpha)) {
                throw std::runtime_error("simulation alpha must be finite");
            }

            for (const auto& node : nodes) {
                try {
                    requireFinite(node.state.load(), "node state");
                    requireFinite(node.healthIndex.load(), "node health");
                } catch (const std::invalid_argument& error) {
                    throw std::runtime_error(
                        std::string{"invalid node state: "} + error.what());
                }

            }
        }

        void validateTopologyAndStateUnlocked() {
            if (topologyValidationRequired) {
                validateTopologyUnlocked();
                topologyValidationRequired = false;
            }

            validateDynamicStateUnlocked();
        }

    public:
        explicit Impl(size_t maxWorkers = 0)
            : workerLimit(maxWorkers),
              productionAuthorityLedger_(
                  detail::MeshInitializationAccess::nextDomainIdentity()),
              productionTransitionEvaluationBinding_(
                  std::make_unique<
                      detail::ProductionTransitionEvaluationBinding>(
                          std::make_shared<
                              LiveProductionTransitionEvaluationBackend>(
                                  *this)))
        {
        }

        ~Impl() {
            productionTransitionEvaluationBinding_->invalidateAndDrain();
            stopWorkerPool();
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;

        void addNode(size_t id, Vector3D pos, double baseline) {
            pos.validate();
            requireFinite(baseline, "baseline");
            std::unique_lock lock(topologyMutex);
            if (id != nodes.size()) {
                throw std::invalid_argument("node ID must match insertion index");
            }
            if (nodeIncarnations_.size() == nodeIncarnations_.capacity()) {
                nodeIncarnations_.reserve(std::max(size_t{1}, nodeIncarnations_.capacity() * 2));
            }
            // vector insertion has the strong guarantee for this copyable node type.
            nodes.emplace_back(id, pos, baseline);
            nodeIncarnations_.push_back(nextNodeIncarnation_);
            ++nextNodeIncarnation_;
            simulationBufferShapeDirty = true;
            topologyValidationRequired = true;
        }

        void connectNodes(int nodeA, int nodeB) {
            std::unique_lock lock(topologyMutex);
            validateNodeIndex(nodeA);
            validateNodeIndex(nodeB);
            if (nodeA == nodeB) throw std::invalid_argument("self-connections are not allowed");
            const bool alreadyConnected = std::any_of(
                nodes[static_cast<size_t>(nodeA)].bridges.begin(),
                nodes[static_cast<size_t>(nodeA)].bridges.end(),
                [nodeB](const SpatialBridge& bridge) {
                    return bridge.targetNodeId == nodeB;
                });
            if (alreadyConnected) {
                throw std::invalid_argument("bridge pair already exists");
            }
            connectPairsUnlocked({{nodeA, nodeB}});
            simulationBufferShapeDirty = true;
            topologyValidationRequired = true;
        }

        /**
         * Connects a batch of node pairs and recomputes stability once at the end.
         * All pairs are validated before the first topology mutation.
         */
        void connectNodePairs(const std::vector<std::pair<int, int>>& connections) {
            std::unique_lock lock(topologyMutex);
            std::unordered_set<EdgeKey, EdgeKeyHash> batchPairs;
            batchPairs.reserve(connections.size());

            for (const auto& [nodeA, nodeB] : connections) {
                validateNodeIndex(nodeA);
                validateNodeIndex(nodeB);
                if (nodeA == nodeB) {
                    throw std::invalid_argument("self-connections are not allowed");
                }

                const size_t first = static_cast<size_t>(std::min(nodeA, nodeB));
                const size_t second = static_cast<size_t>(std::max(nodeA, nodeB));
                const EdgeKey pairKey{first, second};
                if (!batchPairs.insert(pairKey).second) {
                    throw std::invalid_argument("duplicate bridge pair in batch");
                }

                const bool alreadyConnected = std::any_of(
                    nodes[first].bridges.begin(), nodes[first].bridges.end(),
                    [second](const SpatialBridge& bridge) {
                        return bridge.targetNodeId == static_cast<int>(second);
                    });
                if (alreadyConnected) {
                    throw std::invalid_argument("bridge pair already exists");
                }
            }

            connectPairsUnlocked(connections);
        }

        void enforceStabilityCondition() noexcept {
            std::unique_lock lock(topologyMutex);
            enforceStabilityConditionUnlocked();
            topologyValidationRequired = true;
        }

        /** Removes both directions of a bridge pair when either direction is isolated. */
        void pruneIsolatedBridges(double minCapacityThreshold = 0.05) {
            requireFinite(minCapacityThreshold, "minCapacityThreshold");
            if (minCapacityThreshold < 0.0) {
                throw std::invalid_argument("minCapacityThreshold must not be negative");
            }
            std::unique_lock lock(topologyMutex);
            std::unordered_map<EdgeKey, FirstBridgeState, EdgeKeyHash> firstBridgeByEdge;
            std::unordered_set<EdgeKey, EdgeKeyHash> invalidPairs;
            size_t edgeCount = 0;
            for (const auto& node : nodes) edgeCount += node.bridges.size();
            firstBridgeByEdge.reserve(edgeCount);
            invalidPairs.reserve(edgeCount);
            for (size_t sourceNodeId = 0; sourceNodeId < nodes.size(); ++sourceNodeId) {
                const auto& node = nodes[sourceNodeId];
                for (const auto& bridge : node.bridges) {
                    if (bridge.targetNodeId < 0 ||
                        bridge.targetNodeId >= static_cast<int>(nodes.size()) ||
                        bridge.targetNodeId == static_cast<int>(sourceNodeId)) {
                        continue;
                    }
                    const EdgeKey key{sourceNodeId, static_cast<size_t>(bridge.targetNodeId)};
                    firstBridgeByEdge.emplace(key, FirstBridgeState{bridge.capacity, bridge.status});
                    if (bridge.capacity < minCapacityThreshold || bridge.status == BridgeStatus::ISOLATED) {
                        const size_t targetNodeId = static_cast<size_t>(bridge.targetNodeId);
                        invalidPairs.insert(sourceNodeId < targetNodeId
                            ? EdgeKey{sourceNodeId, targetNodeId}
                            : EdgeKey{targetNodeId, sourceNodeId});
                    }
                }
            }
            for (size_t sourceNodeId = 0; sourceNodeId < nodes.size(); ++sourceNodeId) {
                for (const auto& bridge : nodes[sourceNodeId].bridges) {
                    if (bridge.targetNodeId < 0 ||
                        bridge.targetNodeId >= static_cast<int>(nodes.size()) ||
                        bridge.targetNodeId == static_cast<int>(sourceNodeId)) {
                        continue;
                    }
                    const size_t targetNodeId = static_cast<size_t>(bridge.targetNodeId);
                    const EdgeKey pairKey = sourceNodeId < targetNodeId
                        ? EdgeKey{sourceNodeId, targetNodeId}
                        : EdgeKey{targetNodeId, sourceNodeId};
                    const EdgeKey reverseKey{targetNodeId, sourceNodeId};
                    const auto counterpart = firstBridgeByEdge.find(reverseKey);
                    if (counterpart == firstBridgeByEdge.end() ||
                        counterpart->second.capacity < minCapacityThreshold ||
                        counterpart->second.status == BridgeStatus::ISOLATED) {
                        invalidPairs.insert(pairKey);
                    }
                }
            }
            bool topologyShapeChanged = false;
            for (size_t sourceNodeId = 0; sourceNodeId < nodes.size(); ++sourceNodeId) {
                auto& bridges = nodes[sourceNodeId].bridges;
                const size_t oldSize = bridges.size();
                std::erase_if(bridges, [&invalidPairs, sourceNodeId, nodeCount = nodes.size()](const SpatialBridge& bridge) {
                    if (bridge.targetNodeId < 0 || bridge.targetNodeId >= static_cast<int>(nodeCount)) return true;
                    const size_t targetNodeId = static_cast<size_t>(bridge.targetNodeId);
                    const EdgeKey pairKey = sourceNodeId < targetNodeId
                        ? EdgeKey{sourceNodeId, targetNodeId}
                        : EdgeKey{targetNodeId, sourceNodeId};
                    return invalidPairs.contains(pairKey);
                });
                topologyShapeChanged |= bridges.size() != oldSize;
            }
            enforceStabilityConditionUnlocked();
            if (topologyShapeChanged) {
                discardAbsentProductionRelationshipsUnlocked();
                simulationBufferShapeDirty = true;
            }
            topologyValidationRequired = true;
        }

        void autoConnectNearbyNodes(double radius) {
            requireFinite(radius, "radius");
            if (radius < 0.0) throw std::invalid_argument("radius must not be negative");
            std::unique_lock lock(topologyMutex);
            std::vector<std::pair<int, int>> connections;
            std::unordered_set<EdgeKey, EdgeKeyHash> directedEdges;
            size_t edgeCount = 0;
            for (const auto& node : nodes) edgeCount += node.bridges.size();
            directedEdges.reserve(edgeCount + nodes.size());
            for (size_t sourceNodeId = 0; sourceNodeId < nodes.size(); ++sourceNodeId) {
                for (const auto& bridge : nodes[sourceNodeId].bridges) {
                    if (bridge.targetNodeId >= 0 && bridge.targetNodeId < static_cast<int>(nodes.size())) {
                        directedEdges.emplace(sourceNodeId, static_cast<size_t>(bridge.targetNodeId));
                    }
                }
            }
            for (size_t i = 0; i < nodes.size(); ++i) {
                for (size_t j = i + 1; j < nodes.size(); ++j) {
                    if (nodes[i].position.distanceTo(nodes[j].position) <= radius) {
                        const EdgeKey forwardKey{i, j};
                        if (!directedEdges.contains(forwardKey)) {
                            connections.emplace_back(static_cast<int>(i), static_cast<int>(j));
                            directedEdges.insert(forwardKey);
                            directedEdges.insert(EdgeKey{j, i});
                        }
                    }
                }
            }
            connectPairsUnlocked(connections);
            enforceStabilityConditionUnlocked();
            topologyValidationRequired = true;
        }

        void injectExternalShock(int targetNodeId, double shockMagnitude) {
            requireFinite(shockMagnitude, "shockMagnitude");
            std::unique_lock lock(topologyMutex);
            validateNodeIndex(targetNodeId);
            auto& node = nodes[static_cast<size_t>(targetNodeId)];
            double filteredSignal = node.applyLocalReflexFilter(node.state.load() + shockMagnitude);
            node.state.store(filteredSignal);
            node.updateHealth();
        }

        void simulationStep() {
#if SOAM_PHASE_PROFILE_ENABLED
            SimulationPhaseProfile profile{};
            const auto preValidationStart = std::chrono::steady_clock::now();
#endif
            std::unique_lock lock(topologyMutex);
            validateTopologyAndStateUnlocked();
#if SOAM_PHASE_PROFILE_ENABLED
            const auto workerPoolStart = std::chrono::steady_clock::now();
            profile.preValidationMicroseconds = std::chrono::duration<double, std::micro>(workerPoolStart - preValidationStart).count();
#endif
            ensureWorkerPoolUnlocked();
#if SOAM_PHASE_PROFILE_ENABLED
            const auto bufferPreparationStart = std::chrono::steady_clock::now();
            profile.workerPoolReadyMicroseconds = std::chrono::duration<double, std::micro>(bufferPreparationStart - workerPoolStart).count();
#endif
            if (simulationBufferShapeDirty) {
                computedStates.resize(nodes.size());
                pendingBridgeCapacities.resize(nodes.size());
                pendingBridgeStatuses.resize(nodes.size());
                pendingBridgeChanged.resize(nodes.size());
                for (size_t nodeId = 0; nodeId < nodes.size(); ++nodeId) {
                    const size_t bridgeCount = nodes[nodeId].bridges.size();
                    pendingBridgeCapacities[nodeId].resize(bridgeCount);
                    pendingBridgeStatuses[nodeId].resize(bridgeCount);
                    pendingBridgeChanged[nodeId].resize(bridgeCount);
                }
                simulationBufferShapeDirty = false;
            }
#if SOAM_PHASE_PROFILE_ENABLED
            const auto dispatchStart = std::chrono::steady_clock::now();
            profile.bufferPreparationMicroseconds = std::chrono::duration<double, std::micro>(dispatchStart - bufferPreparationStart).count();
#endif
            if (computedStates.empty()) return;

            if (workers.size() == 1) {
                runNodeRange(
                    0,
                    nodes.size(),
                    computedStates,
                    pendingBridgeCapacities,
                    pendingBridgeStatuses,
                    pendingBridgeChanged);
#if SOAM_PHASE_PROFILE_ENABLED
                const auto directResultValidationStart = std::chrono::steady_clock::now();
                profile.workerDispatchWaitMicroseconds =
                    std::chrono::duration<double, std::micro>(
                        directResultValidationStart - dispatchStart).count();
#endif
            } else {
                {
                    std::lock_guard workLock(workMutex);
                    activeNodeCount = nodes.size();
                    activeWorkerCount = workers.size();
                    remainingWorkerCount = activeWorkerCount;
                    workerException = nullptr;
                    dispatchedStates = &computedStates;
                    dispatchedBridgeCapacities = &pendingBridgeCapacities;
                    dispatchedBridgeStatuses = &pendingBridgeStatuses;
                    dispatchedBridgeChanged = &pendingBridgeChanged;
                    ++workGeneration;
                    for (size_t workerId = 0;
                         workerId < activeWorkerCount;
                         ++workerId) {
                        workerSlots[workerId]->assignedGeneration =
                            workGeneration;
                    }
                }
                workAvailable.notify_all();
                {
#if SOAM_PHASE_PROFILE_ENABLED
                    const auto waitStart = std::chrono::steady_clock::now();
#endif
                    std::unique_lock workLock(workMutex);
                    while (remainingWorkerCount != 0) {
                        workCompleted.wait_for(
                            workLock,
                            coordinationRecheckInterval);
                    }
#if SOAM_PHASE_PROFILE_ENABLED
                    const auto resultValidationStart = std::chrono::steady_clock::now();
                    profile.workerDispatchWaitMicroseconds =
                        std::chrono::duration<double, std::micro>(
                            resultValidationStart - waitStart).count();
#endif
                    dispatchedStates = nullptr;
                    dispatchedBridgeCapacities = nullptr;
                    dispatchedBridgeStatuses = nullptr;
                    dispatchedBridgeChanged = nullptr;
                    std::exception_ptr error = workerException;
                    workerException = nullptr;
                    workLock.unlock();
                    if (error) std::rethrow_exception(error);
                }
            }
#if SOAM_PHASE_PROFILE_ENABLED
            const auto resultValidationStart = std::chrono::steady_clock::now();
#endif
            for (size_t i = 0; i < nodes.size(); ++i) {
                if (!std::isfinite(computedStates[i])) throw std::runtime_error("simulation produced a non-finite state");
            }
#if SOAM_PHASE_PROFILE_ENABLED
            const auto resultValidationEnd = std::chrono::steady_clock::now();
            profile.resultValidationMicroseconds = std::chrono::duration<double, std::micro>(resultValidationEnd - resultValidationStart).count();
#endif
            for (size_t i = 0; i < nodes.size(); ++i) {
                const double committedState = computedStates[i];
                nodes[i].state.store(committedState);
                updateHealthUnchecked(nodes[i], committedState);
                for (size_t bridgeIndex = 0; bridgeIndex < nodes[i].bridges.size(); ++bridgeIndex) {
                    if (pendingBridgeChanged[i][bridgeIndex] == 0) continue;
                    nodes[i].bridges[bridgeIndex].capacity = pendingBridgeCapacities[i][bridgeIndex];
                    nodes[i].bridges[bridgeIndex].status = pendingBridgeStatuses[i][bridgeIndex];
                }
            }
#if SOAM_PHASE_PROFILE_ENABLED
            const auto postValidationStart = std::chrono::steady_clock::now();
#endif
#if SOAM_PHASE_PROFILE_ENABLED
            const auto profileEnd = std::chrono::steady_clock::now();
            profile.postValidationMicroseconds = std::chrono::duration<double, std::micro>(profileEnd - postValidationStart).count();
            lastSimulationPhaseProfile = profile;
#endif
        }

#if SOAM_PHASE_PROFILE_ENABLED
        [[nodiscard]] SimulationPhaseProfile getLastSimulationPhaseProfile() const {
            std::shared_lock lock(topologyMutex);
            return lastSimulationPhaseProfile;
        }
#endif

        void simulationStepAsync() { simulationStep(); }

        [[nodiscard]] detail::ProductionTransitionEvaluationBindingHandle
        evaluationHandle() const noexcept {
            return productionTransitionEvaluationBinding_->handle();
        }

        [[nodiscard]] ProductionAuthorityDerivationResult
        deriveProductionAuthority(
            const ProductionTransitionEligibilityDecision& eligibility,
            bool authorityPolicySatisfied)
        {
            LiveProductionTransitionEvaluationBackend backend(*this);
            return detail::ProductionAuthorityLiveDerivation::evaluate(
                backend,
                productionAuthorityLedger_,
                eligibility,
                authorityPolicySatisfied);
        }

        [[nodiscard]] double getNodeState(size_t id) const {
            std::shared_lock lock(topologyMutex);
            return nodes.at(id).state.load();
        }

        [[nodiscard]] double getNodeHealth(size_t id) const {
            std::shared_lock lock(topologyMutex);
            return nodes.at(id).healthIndex.load();
        }

        [[nodiscard]] size_t getNodeBridgesCount(size_t id) const {
            std::shared_lock lock(topologyMutex);
            return nodes.at(id).bridges.size();
        }
    };

} // namespace AdaptiveMesh
