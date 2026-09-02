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
            double dx = x - other.x;
            double dy = y - other.y;
            double dz = z - other.z;
            return std::sqrt(dx * dx + dy * dy + dz * dz);
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
            double spatialAttenuation = 1.0 / (1.0 + 0.1 * distance);
            return capacity * spatialAttenuation * orientationWeight;
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

    /**
     * All topology mutations and simulationStepAsync() are serialized against one
     * topology snapshot. Read-only getters may run concurrently with each other,
     * but wait for an active mutation or simulation step to finish.
     */
    class SpatialAdaptiveMesh {
    private:
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
        bool stoppingWorkers = false;

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
            }
        }

        void connectNodesUnlocked(int nodeA, int nodeB) {
            double dist = nodes[nodeA].position.distanceTo(nodes[nodeB].position);
            double orientA = nodes[nodeA].position.orientationFactorTo(nodes[nodeB].position);
            double orientB = nodes[nodeB].position.orientationFactorTo(nodes[nodeA].position);

            // Reserve both sides before mutating either side. If the second reserve
            // throws, bridge counts and values remain unchanged.
            nodes[nodeA].bridges.reserve(nodes[nodeA].bridges.size() + 1);
            nodes[nodeB].bridges.reserve(nodes[nodeB].bridges.size() + 1);

            nodes[nodeA].bridges.push_back({nodeB, dist, orientA, 1.0, BridgeStatus::NORMAL});
            nodes[nodeB].bridges.push_back({nodeA, dist, orientB, 1.0, BridgeStatus::NORMAL});
            enforceStabilityConditionUnlocked();
        }

        [[nodiscard]] size_t resolveWorkerCountUnlocked() const noexcept {
            if (nodes.empty()) {
                return 0;
            }
            const size_t hardwareWorkers = std::max(
                size_t{1}, static_cast<size_t>(std::thread::hardware_concurrency()));
            const size_t requestedWorkers = workerLimit == 0 ? hardwareWorkers : workerLimit;
            return std::min(requestedWorkers, nodes.size());
        }

        void runNodeRange(size_t firstNode, size_t lastNode, std::vector<double>& outputStates) {
            for (size_t i = firstNode; i < lastNode; ++i) {
                auto& node = nodes[i];
                double diffusionSum = 0.0;
                double currentState = node.state.load();

                for (auto& bridge : node.bridges) {
                    auto& neighbor = nodes[bridge.targetNodeId];
                    double neighborState = neighbor.state.load();
                    double deltaS = neighborState - currentState;

                    SignalCategory category = node.metaEvaluator.evaluate(
                        currentState + deltaS, node.healthIndex.load(), node.invariant
                    );

                    bridge.updateBridgeState(category);
                    diffusionSum += bridge.getEffectiveTransmission() * deltaS;
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
                if (stoppingWorkers) {
                    return;
                }

                observedGeneration = workGeneration;
                const size_t workerCount = activeWorkerCount;
                const size_t nodeCount = activeNodeCount;
                auto* outputStates = dispatchedStates;
                const size_t firstNode = workerId * nodeCount / workerCount;
                const size_t lastNode = (workerId + 1) * nodeCount / workerCount;
                lock.unlock();

                runNodeRange(firstNode, lastNode, *outputStates);

                lock.lock();
                ++completedWorkerCount;
                if (completedWorkerCount == activeWorkerCount) {
                    workCompleted.notify_one();
                }
            }
        }

        void ensureWorkerPoolUnlocked() {
            const size_t requiredWorkerCount = resolveWorkerCountUnlocked();
            for (size_t workerId = workers.size(); workerId < requiredWorkerCount; ++workerId) {
                workers.emplace_back([this, workerId] { workerLoop(workerId); });
            }
            std::unique_lock workLock(workMutex);
            workerReady.wait(workLock, [this] {
                return readyWorkerCount == workers.size();
            });
        }

        void stopWorkerPool() noexcept {
            {
                std::lock_guard lock(workMutex);
                stoppingWorkers = true;
            }
            workAvailable.notify_all();
            workers.clear();
        }

        void validateTopologyAndStateUnlocked() const {
            if (!std::isfinite(alpha)) {
                throw std::runtime_error("simulation alpha must be finite");
            }
            for (size_t sourceNodeId = 0; sourceNodeId < nodes.size(); ++sourceNodeId) {
                const auto& node = nodes[sourceNodeId];
                try {
                    node.position.validate();
                    node.invariant.validate();
                    requireFinite(node.state.load(), "node state");
                    requireFinite(node.healthIndex.load(), "node health");
                } catch (const std::invalid_argument& error) {
                    throw std::runtime_error(std::string{"invalid node invariant: "} + error.what());
                }

                for (const auto& bridge : node.bridges) {
                    if (bridge.targetNodeId < 0 ||
                        bridge.targetNodeId >= static_cast<int>(nodes.size()) ||
                        bridge.targetNodeId == static_cast<int>(sourceNodeId)) {
                        throw std::runtime_error("bridge target violates topology invariant");
                    }
                    if (!std::isfinite(bridge.distance) ||
                        !std::isfinite(bridge.orientationWeight) ||
                        !std::isfinite(bridge.capacity)) {
                        throw std::runtime_error("bridge values must be finite");
                    }
                    const auto& counterpartBridges =
                        nodes[static_cast<size_t>(bridge.targetNodeId)].bridges;
                    const bool hasCounterpart = std::any_of(
                        counterpartBridges.begin(), counterpartBridges.end(),
                        [sourceNodeId](const SpatialBridge& candidate) {
                            return candidate.targetNodeId == static_cast<int>(sourceNodeId);
                        });
                    if (!hasCounterpart) {
                        throw std::runtime_error("bridge pair invariant is violated");
                    }
                }
            }
        }

    public:
        /**
         * maxWorkers limits reusable simulation workers. A value of zero selects
         * min(max(1, hardware_concurrency()), node_count) for each pool expansion.
         */
        explicit SpatialAdaptiveMesh(size_t maxWorkers = 0)
            : workerLimit(maxWorkers) {}

        ~SpatialAdaptiveMesh() {
            stopWorkerPool();
        }

        SpatialAdaptiveMesh(const SpatialAdaptiveMesh&) = delete;
        SpatialAdaptiveMesh& operator=(const SpatialAdaptiveMesh&) = delete;
        SpatialAdaptiveMesh(SpatialAdaptiveMesh&&) = delete;
        SpatialAdaptiveMesh& operator=(SpatialAdaptiveMesh&&) = delete;

        void addNode(size_t id, Vector3D pos, double baseline) {
            pos.validate();
            requireFinite(baseline, "baseline");
            std::unique_lock lock(topologyMutex);
            nodes.emplace_back(id, pos, baseline);
        }

        /**
         * Creates one pair of directed bridges: nodeA -> nodeB and nodeB -> nodeA.
         * @throws std::out_of_range when either index does not name an existing node.
         * @throws std::invalid_argument when nodeA and nodeB are the same node.
         */
        void connectNodes(int nodeA, int nodeB) {
            std::unique_lock lock(topologyMutex);
            validateNodeIndex(nodeA);
            validateNodeIndex(nodeB);
            if (nodeA == nodeB) {
                throw std::invalid_argument("self-connections are not allowed");
            }
            connectNodesUnlocked(nodeA, nodeB);
        }

        void enforceStabilityCondition() noexcept {
            std::unique_lock lock(topologyMutex);
            enforceStabilityConditionUnlocked();
        }

        /** Removes both directions of a bridge pair when either direction is isolated. */
        void pruneIsolatedBridges(double minCapacityThreshold = 0.05) {
            requireFinite(minCapacityThreshold, "minCapacityThreshold");
            if (minCapacityThreshold < 0.0) {
                throw std::invalid_argument("minCapacityThreshold must not be negative");
            }
            std::unique_lock lock(topologyMutex);
            const auto shouldPrune = [this, minCapacityThreshold](size_t sourceNodeId,
                                                                   const SpatialBridge& bridge) {
                const bool localIsInvalid = bridge.capacity < minCapacityThreshold ||
                                            bridge.status == BridgeStatus::ISOLATED;
                const auto& targetBridges = nodes[static_cast<size_t>(bridge.targetNodeId)].bridges;
                const auto counterpart = std::find_if(
                    targetBridges.begin(), targetBridges.end(),
                    [sourceNodeId](const SpatialBridge& candidate) {
                        return candidate.targetNodeId == static_cast<int>(sourceNodeId);
                    });
                const bool counterpartIsInvalid = counterpart == targetBridges.end() ||
                                                   counterpart->capacity < minCapacityThreshold ||
                                                   counterpart->status == BridgeStatus::ISOLATED;
                return localIsInvalid || counterpartIsInvalid;
            };

            for (auto& node : nodes) {
                const size_t sourceNodeId = static_cast<size_t>(&node - nodes.data());
                std::erase_if(node.bridges, [&shouldPrune, sourceNodeId](const SpatialBridge& bridge) {
                    return shouldPrune(sourceNodeId, bridge);
                });
            }
            enforceStabilityConditionUnlocked();
        }

        void autoConnectNearbyNodes(double radius) {
            requireFinite(radius, "radius");
            if (radius < 0.0) {
                throw std::invalid_argument("radius must not be negative");
            }
            std::unique_lock lock(topologyMutex);
            for (size_t i = 0; i < nodes.size(); ++i) {
                for (size_t j = i + 1; j < nodes.size(); ++j) {
                    if (nodes[i].position.distanceTo(nodes[j].position) <= radius) {
                        bool exists = std::any_of(nodes[i].bridges.begin(), nodes[i].bridges.end(),
                            [j](const SpatialBridge& b) { return b.targetNodeId == static_cast<int>(j); });
                        if (!exists) {
                            connectNodesUnlocked(static_cast<int>(i), static_cast<int>(j));
                        }
                    }
                }
            }
        }

        void injectExternalShock(int targetNodeId, double shockMagnitude) {
            requireFinite(shockMagnitude, "shockMagnitude");
            std::unique_lock lock(topologyMutex);
            if (targetNodeId < 0 || targetNodeId >= static_cast<int>(nodes.size())) return;
            auto& node = nodes[targetNodeId];
            double filteredSignal = node.applyLocalReflexFilter(node.state.load() + shockMagnitude);
            node.state.store(filteredSignal);
            node.updateHealth();
        }

        void simulationStepAsync() {
            std::unique_lock lock(topologyMutex);
            validateTopologyAndStateUnlocked();
            ensureWorkerPoolUnlocked();
            std::vector<double> computedStates(nodes.size());
            if (computedStates.empty()) {
                return;
            }

            {
                std::lock_guard workLock(workMutex);
                activeNodeCount = nodes.size();
                activeWorkerCount = workers.size();
                completedWorkerCount = 0;
                dispatchedStates = &computedStates;
                ++workGeneration;
            }
            workAvailable.notify_all();

            {
                std::unique_lock workLock(workMutex);
                workCompleted.wait(workLock, [this] {
                    return completedWorkerCount == activeWorkerCount;
                });
                dispatchedStates = nullptr;
            }

            for (size_t i = 0; i < nodes.size(); ++i) {
                if (!std::isfinite(computedStates[i])) {
                    throw std::runtime_error("simulation produced a non-finite state");
                }
                nodes[i].state.store(computedStates[i]);
                nodes[i].updateHealth();
            }
            validateTopologyAndStateUnlocked();
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

#endif // SYSTEM_ARCHITECTURE_HPP
