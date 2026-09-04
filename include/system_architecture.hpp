/**
 * @file system_architecture.hpp
 * @brief Self-Organizing Adaptive Mesh Architecture (C++20 Header-Only)
 * @version 1.1.0
 * @copyright Copyright (c) 2026 Безручко Микола Миколайович. All rights reserved.
 * @license Licensed under the GNU AGPLv3 (or Commercial License upon request).
 * Repository: https://github.com/tonybarannn-maker/adaptive-mesh
 */

#ifndef SYSTEM_ARCHITECTURE_HPP
#define SYSTEM_ARCHITECTURE_HPP

#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include <algorithm>
#include <numeric>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#ifdef ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
#include <chrono>
#endif

namespace AdaptiveMesh {

    inline void requireFinite(double value, const char* name) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument(std::string{name} + " must be finite");
        }
    }

    struct Vector3D {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;

        void validate() const {
            requireFinite(x, "x");
            requireFinite(y, "y");
            requireFinite(z, "z");
        }

        [[nodiscard]] double distanceTo(const Vector3D& other) const {
            validate();
            other.validate();
            const double dx = x - other.x;
            const double dy = y - other.y;
            const double dz = z - other.z;
            requireFinite(dx, "distance dx");
            requireFinite(dy, "distance dy");
            requireFinite(dz, "distance dz");
            const double distance = std::hypot(std::hypot(dx, dy), dz);
            requireFinite(distance, "distance");
            return distance;
        }

        [[nodiscard]] double orientationFactorTo(const Vector3D& target) const {
            double dist = distanceTo(target);
            if (dist < 1e-6) return 1.0;
            double cosTheta = (target.z - z) / dist;
            return 0.5 * (1.0 + cosTheta);
        }
    };

    struct IdentityInvariant {
        double baseline = 1.6180339887;
        double maxEpsilon = 10.0;

        void validate() const {
            requireFinite(baseline, "baseline");
            requireFinite(maxEpsilon, "maxEpsilon");
            if (maxEpsilon <= 0.0) {
                throw std::invalid_argument("maxEpsilon must be positive");
            }
        }

        [[nodiscard]] bool isWithinSafetyBound(double state) const {
            validate();
            requireFinite(state, "state");
            return std::abs(state - baseline) <= maxEpsilon;
        }
    };

    enum class SignalCategory { NOISE, CREATIVE_SIGNAL, DESTRUCTIVE_DRIFT };
    enum class BridgeStatus { NORMAL, DAMPING, RECOVERY, ISOLATED };

    class MetaEvaluator {
    public:
        [[nodiscard]] SignalCategory evaluate(double candidateState,
                                              double healthIndex,
                                              const IdentityInvariant& omega) const
        {
            requireFinite(candidateState, "candidateState");
            requireFinite(healthIndex, "healthIndex");
            if (!omega.isWithinSafetyBound(candidateState)) {
                return SignalCategory::DESTRUCTIVE_DRIFT;
            }
            double deltaFromBase = std::abs(candidateState - omega.baseline);
            if (healthIndex > 0.7 && deltaFromBase > 1.2) {
                return SignalCategory::CREATIVE_SIGNAL;
            }
            return SignalCategory::NOISE;
        }
    };

    struct SpatialBridge {
        int targetNodeId;
        double distance;
        double orientationWeight;
        double capacity = 1.0;
        BridgeStatus status = BridgeStatus::NORMAL;

        void updateBridgeState(SignalCategory category) {
            switch (category) {
                case SignalCategory::NOISE:
                    capacity = std::max(0.01, capacity * 0.85);
                    status = BridgeStatus::DAMPING;
                    break;
                case SignalCategory::CREATIVE_SIGNAL:
                    capacity = std::min(1.0, capacity + 0.15);
                    status = (capacity >= 0.9) ? BridgeStatus::NORMAL : BridgeStatus::RECOVERY;
                    break;
                case SignalCategory::DESTRUCTIVE_DRIFT:
                    capacity = 0.0;
                    status = BridgeStatus::ISOLATED;
                    break;
            }
        }

        [[nodiscard]] double getEffectiveTransmission() const {
            requireFinite(distance, "bridge distance");
            requireFinite(orientationWeight, "bridge orientationWeight");
            requireFinite(capacity, "bridge capacity");
            if (distance < 0.0) {
                throw std::invalid_argument("bridge distance must be non-negative");
            }
            if (orientationWeight < 0.0 || orientationWeight > 1.0) {
                throw std::invalid_argument("bridge orientationWeight must be in [0, 1]");
            }
            if (capacity < 0.0 || capacity > 1.0) {
                throw std::invalid_argument("bridge capacity must be in [0, 1]");
            }
            const double spatialAttenuation = 1.0 / (1.0 + 0.1 * distance);
            const double transmission = capacity * spatialAttenuation * orientationWeight;
            requireFinite(transmission, "effective transmission");
            return transmission;
        }
    };

    class AutopoieticNode {
    public:
        size_t id;
        Vector3D position;
        std::atomic<double> state;
        std::atomic<double> healthIndex{1.0};
        IdentityInvariant invariant;
        MetaEvaluator metaEvaluator;
        std::vector<SpatialBridge> bridges;
        mutable std::mutex nodeMutex;

        AutopoieticNode(size_t nodeId, Vector3D pos, double initialBaseline)
            : id(nodeId), position(pos), state(initialBaseline)
        {
            position.validate();
            requireFinite(initialBaseline, "baseline");
            invariant.baseline = initialBaseline;
        }

        AutopoieticNode(const AutopoieticNode& other)
            : id(other.id), position(other.position), state(other.state.load()),
              healthIndex(other.healthIndex.load()), invariant(other.invariant),
              metaEvaluator(other.metaEvaluator), bridges(other.bridges) {}

        void updateHealth() {
            invariant.validate();
            const double currentState = state.load();
            requireFinite(currentState, "state");
            double drift = std::abs(currentState - invariant.baseline);
            healthIndex.store(std::max(0.0, 1.0 - (drift / invariant.maxEpsilon)));
        }

        [[nodiscard]] double applyLocalReflexFilter(double rawInput) const {
            requireFinite(rawInput, "rawInput");
            double maxAllowedReflexStep = 3.5;
            double currentState = state.load();
            requireFinite(currentState, "state");
            double delta = rawInput - currentState;
            if (std::abs(delta) > maxAllowedReflexStep) {
                return currentState + (delta > 0 ? maxAllowedReflexStep : -maxAllowedReflexStep);
            }
            return rawInput;
        }
    };

#ifdef ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
    struct SimulationPhaseProfile {
        double preValidationMicroseconds = 0.0;
        double workerPoolReadyMicroseconds = 0.0;
        double bufferPreparationMicroseconds = 0.0;
        double workerDispatchWaitMicroseconds = 0.0;
        double resultValidationMicroseconds = 0.0;
        double commitMicroseconds = 0.0;
        double postValidationMicroseconds = 0.0;
    };
#endif

    class SpatialAdaptiveMesh {
    private:
        using EdgeKey = std::pair<size_t, size_t>;

        struct EdgeKeyHash {
            size_t operator()(const EdgeKey& key) const noexcept {
                const size_t h1 = std::hash<size_t>{}(key.first);
                const size_t h2 = std::hash<size_t>{}(key.second);
                return h1 ^ (h2 + static_cast<size_t>(0x9e3779b97f4a7c15ULL) + (h1 << 6) + (h1 >> 2));
            }
        };

        struct FirstBridgeState {
            double capacity;
            BridgeStatus status;
        };

        std::vector<AutopoieticNode> nodes;
        double alpha = 0.15;
        mutable std::shared_mutex topologyMutex;
        const size_t workerLimit;
        std::vector<std::jthread> workers;
        std::mutex workMutex;
        std::condition_variable workAvailable;
        std::condition_variable workCompleted;
        std::condition_variable workerReady;
        size_t workGeneration = 0;
        size_t activeNodeCount = 0;
        size_t activeWorkerCount = 0;
        size_t completedWorkerCount = 0;
        size_t readyWorkerCount = 0;
        std::vector<double>* dispatchedStates = nullptr;
        std::vector<std::vector<double>>* dispatchedBridgeCapacities = nullptr;
        std::vector<std::vector<BridgeStatus>>* dispatchedBridgeStatuses = nullptr;
        std::vector<double> computedStates;
        std::vector<std::vector<double>> pendingBridgeCapacities;
        std::vector<std::vector<BridgeStatus>> pendingBridgeStatuses;
        bool stoppingWorkers = false;
        std::exception_ptr workerException;
        bool topologyValidationRequired = true;
#ifdef ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
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

        void connectNodesUnlocked(int nodeA, int nodeB, bool enforceStability = true) {
            double dist = nodes[nodeA].position.distanceTo(nodes[nodeB].position);
            double orientA = nodes[nodeA].position.orientationFactorTo(nodes[nodeB].position);
            double orientB = nodes[nodeB].position.orientationFactorTo(nodes[nodeA].position);
            nodes[nodeA].bridges.reserve(nodes[nodeA].bridges.size() + 1);
            nodes[nodeB].bridges.reserve(nodes[nodeB].bridges.size() + 1);
            nodes[nodeA].bridges.push_back({nodeB, dist, orientA, 1.0, BridgeStatus::NORMAL});
            nodes[nodeB].bridges.push_back({nodeA, dist, orientB, 1.0, BridgeStatus::NORMAL});
            if (enforceStability) {
                enforceStabilityConditionUnlocked();
            }
        }

        [[nodiscard]] size_t resolveWorkerCountUnlocked() const noexcept {
            if (nodes.empty()) return 0;
            const size_t hardwareWorkers = std::max(size_t{1}, static_cast<size_t>(std::thread::hardware_concurrency()));
            const size_t requestedWorkers = workerLimit == 0 ? hardwareWorkers : workerLimit;
            return std::min(requestedWorkers, nodes.size());
        }

        void runNodeRange(size_t firstNode, size_t lastNode,
                          std::vector<double>& outputStates,
                          std::vector<std::vector<double>>& bridgeCapacityOutput,
                          std::vector<std::vector<BridgeStatus>>& bridgeStatusOutput)
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
                    SignalCategory category = node.metaEvaluator.evaluate(
                        currentState + deltaS, node.healthIndex.load(), node.invariant);
                    SpatialBridge nextBridge = bridge;
                    nextBridge.updateBridgeState(category);
                    bridgeCapacityOutput[i][bridgeIndex] = nextBridge.capacity;
                    bridgeStatusOutput[i][bridgeIndex] = nextBridge.status;
                    diffusionSum += nextBridge.getEffectiveTransmission() * deltaS;
                }
                outputStates[i] = currentState + alpha * diffusionSum;
            }
        }

        void workerLoop(size_t workerId) {
            std::unique_lock lock(workMutex);
            size_t observedGeneration = workGeneration;
            ++readyWorkerCount;
            workerReady.notify_one();
            while (true) {
                workAvailable.wait(lock, [this, observedGeneration] {
                    return stoppingWorkers || workGeneration != observedGeneration;
                });
                if (stoppingWorkers) return;
                observedGeneration = workGeneration;
                const size_t workerCount = activeWorkerCount;
                const size_t nodeCount = activeNodeCount;
                auto* outputStates = dispatchedStates;
                auto* bridgeCapacities = dispatchedBridgeCapacities;
                auto* bridgeStatuses = dispatchedBridgeStatuses;
                const size_t firstNode = workerId * nodeCount / workerCount;
                const size_t lastNode = (workerId + 1) * nodeCount / workerCount;
                lock.unlock();
                try {
                    runNodeRange(firstNode, lastNode, *outputStates, *bridgeCapacities, *bridgeStatuses);
                } catch (...) {
                    lock.lock();
                    if (!workerException) workerException = std::current_exception();
                    ++completedWorkerCount;
                    if (completedWorkerCount == activeWorkerCount) workCompleted.notify_one();
                    continue;
                }
                lock.lock();
                ++completedWorkerCount;
                if (completedWorkerCount == activeWorkerCount) workCompleted.notify_one();
            }
        }

        void ensureWorkerPoolUnlocked() {
            const size_t requiredWorkerCount = resolveWorkerCountUnlocked();
            for (size_t workerId = workers.size(); workerId < requiredWorkerCount; ++workerId) {
                workers.emplace_back([this, workerId] { workerLoop(workerId); });
            }
            std::unique_lock workLock(workMutex);
            workerReady.wait(workLock, [this] { return readyWorkerCount == workers.size(); });
        }

        void stopWorkerPool() noexcept {
            {
                std::lock_guard lock(workMutex);
                stoppingWorkers = true;
            }
            workAvailable.notify_all();
            workers.clear();
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

                for (const auto& bridge : node.bridges) {
                    if (!std::isfinite(bridge.capacity)) {
                        throw std::runtime_error(
                            "bridge capacity must be finite");
                    }
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
        explicit SpatialAdaptiveMesh(size_t maxWorkers = 0)
            : workerLimit(maxWorkers) {}

        ~SpatialAdaptiveMesh() { stopWorkerPool(); }

        SpatialAdaptiveMesh(const SpatialAdaptiveMesh&) = delete;
        SpatialAdaptiveMesh& operator=(const SpatialAdaptiveMesh&) = delete;
        SpatialAdaptiveMesh(SpatialAdaptiveMesh&&) = delete;
        SpatialAdaptiveMesh& operator=(SpatialAdaptiveMesh&&) = delete;

        void addNode(size_t id, Vector3D pos, double baseline) {
            pos.validate();
            requireFinite(baseline, "baseline");
            std::unique_lock lock(topologyMutex);
            if (id != nodes.size()) {
                throw std::invalid_argument("node ID must match insertion index");
            }
            nodes.emplace_back(id, pos, baseline);
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
            connectNodesUnlocked(nodeA, nodeB);
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

            for (const auto& [nodeA, nodeB] : connections) {
                connectNodesUnlocked(nodeA, nodeB, false);
            }
            if (!connections.empty()) {
                enforceStabilityConditionUnlocked();
                topologyValidationRequired = true;
            }
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
            for (size_t sourceNodeId = 0; sourceNodeId < nodes.size(); ++sourceNodeId) {
                auto& bridges = nodes[sourceNodeId].bridges;
                std::erase_if(bridges, [&invalidPairs, sourceNodeId, nodeCount = nodes.size()](const SpatialBridge& bridge) {
                    if (bridge.targetNodeId < 0 || bridge.targetNodeId >= static_cast<int>(nodeCount)) return true;
                    const size_t targetNodeId = static_cast<size_t>(bridge.targetNodeId);
                    const EdgeKey pairKey = sourceNodeId < targetNodeId
                        ? EdgeKey{sourceNodeId, targetNodeId}
                        : EdgeKey{targetNodeId, sourceNodeId};
                    return invalidPairs.contains(pairKey);
                });
            }
            enforceStabilityConditionUnlocked();
            topologyValidationRequired = true;
        }

        void autoConnectNearbyNodes(double radius) {
            requireFinite(radius, "radius");
            if (radius < 0.0) throw std::invalid_argument("radius must not be negative");
            std::unique_lock lock(topologyMutex);
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
                            connectNodesUnlocked(static_cast<int>(i), static_cast<int>(j), false);
                            directedEdges.insert(forwardKey);
                            directedEdges.insert(EdgeKey{j, i});
                        }
                    }
                }
            }
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
#ifdef ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
            SimulationPhaseProfile profile{};
            const auto preValidationStart = std::chrono::steady_clock::now();
#endif
            std::unique_lock lock(topologyMutex);
            validateTopologyAndStateUnlocked();
#ifdef ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
            const auto workerPoolStart = std::chrono::steady_clock::now();
            profile.preValidationMicroseconds = std::chrono::duration<double, std::micro>(workerPoolStart - preValidationStart).count();
#endif
            ensureWorkerPoolUnlocked();
#ifdef ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
            const auto bufferPreparationStart = std::chrono::steady_clock::now();
            profile.workerPoolReadyMicroseconds = std::chrono::duration<double, std::micro>(bufferPreparationStart - workerPoolStart).count();
#endif
            computedStates.resize(nodes.size());
            pendingBridgeCapacities.resize(nodes.size());
            pendingBridgeStatuses.resize(nodes.size());
            for (size_t nodeId = 0; nodeId < nodes.size(); ++nodeId) {
                const size_t bridgeCount = nodes[nodeId].bridges.size();
                pendingBridgeCapacities[nodeId].resize(bridgeCount);
                pendingBridgeStatuses[nodeId].resize(bridgeCount);
            }
#ifdef ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
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
                    pendingBridgeStatuses);
#ifdef ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
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
                    completedWorkerCount = 0;
                    workerException = nullptr;
                    dispatchedStates = &computedStates;
                    dispatchedBridgeCapacities = &pendingBridgeCapacities;
                    dispatchedBridgeStatuses = &pendingBridgeStatuses;
                    ++workGeneration;
                }
                workAvailable.notify_all();
                {
#ifdef ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
                    const auto waitStart = std::chrono::steady_clock::now();
#endif
                    std::unique_lock workLock(workMutex);
                    workCompleted.wait(workLock, [this] {
                        return completedWorkerCount == activeWorkerCount;
                    });
#ifdef ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
                    const auto resultValidationStart = std::chrono::steady_clock::now();
                    profile.workerDispatchWaitMicroseconds =
                        std::chrono::duration<double, std::micro>(
                            resultValidationStart - waitStart).count();
#endif
                    dispatchedStates = nullptr;
                    dispatchedBridgeCapacities = nullptr;
                    dispatchedBridgeStatuses = nullptr;
                    std::exception_ptr error = workerException;
                    workerException = nullptr;
                    workLock.unlock();
                    if (error) std::rethrow_exception(error);
                }
            }
#ifdef ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
            const auto resultValidationStart = std::chrono::steady_clock::now();
#endif
            for (size_t i = 0; i < nodes.size(); ++i) {
                if (!std::isfinite(computedStates[i])) throw std::runtime_error("simulation produced a non-finite state");
                if (pendingBridgeCapacities[i].size() != nodes[i].bridges.size() ||
                    pendingBridgeStatuses[i].size() != nodes[i].bridges.size()) {
                    throw std::runtime_error("pending bridge state size mismatch");
                }
                for (size_t bridgeIndex = 0; bridgeIndex < nodes[i].bridges.size(); ++bridgeIndex) {
                    if (!std::isfinite(pendingBridgeCapacities[i][bridgeIndex])) {
                        throw std::runtime_error("simulation produced a non-finite bridge capacity");
                    }
                }
            }
#ifdef ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
            const auto commitStart = std::chrono::steady_clock::now();
            profile.resultValidationMicroseconds = std::chrono::duration<double, std::micro>(commitStart - resultValidationStart).count();
#endif
            for (size_t i = 0; i < nodes.size(); ++i) {
                nodes[i].state.store(computedStates[i]);
                nodes[i].updateHealth();
                for (size_t bridgeIndex = 0; bridgeIndex < nodes[i].bridges.size(); ++bridgeIndex) {
                    nodes[i].bridges[bridgeIndex].capacity = pendingBridgeCapacities[i][bridgeIndex];
                    nodes[i].bridges[bridgeIndex].status = pendingBridgeStatuses[i][bridgeIndex];
                }
            }
#ifdef ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
            const auto postValidationStart = std::chrono::steady_clock::now();
            profile.commitMicroseconds = std::chrono::duration<double, std::micro>(postValidationStart - commitStart).count();
#endif
            validateTopologyAndStateUnlocked();
#ifdef ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
            const auto profileEnd = std::chrono::steady_clock::now();
            profile.postValidationMicroseconds = std::chrono::duration<double, std::micro>(profileEnd - postValidationStart).count();
            lastSimulationPhaseProfile = profile;
#endif
        }

#ifdef ADAPTIVE_MESH_ENABLE_PHASE_PROFILE
        [[nodiscard]] SimulationPhaseProfile getLastSimulationPhaseProfile() const {
            std::shared_lock lock(topologyMutex);
            return lastSimulationPhaseProfile;
        }
#endif

        void simulationStepAsync() { simulationStep(); }

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

#endif // SYSTEM_ARCHITECTURE_HPP
