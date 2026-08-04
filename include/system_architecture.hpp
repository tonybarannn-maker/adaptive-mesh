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
#include <atomic>
#include <thread>

namespace AdaptiveMesh {

    struct Vector3D {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;

        [[nodiscard]] double distanceTo(const Vector3D& other) const noexcept {
            double dx = x - other.x;
            double dy = y - other.y;
            double dz = z - other.z;
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        }

        [[nodiscard]] double orientationFactorTo(const Vector3D& target) const noexcept {
            double dist = distanceTo(target);
            if (dist < 1e-6) return 1.0;
            double cosTheta = (target.z - z) / dist;
            return 0.5 * (1.0 + cosTheta);
        }
    };

    struct IdentityInvariant {
        double baseline = 1.6180339887;
        double maxEpsilon = 10.0;

        [[nodiscard]] bool isWithinSafetyBound(double state) const noexcept {
            return std::abs(state - baseline) <= maxEpsilon;
        }
    };

    enum class SignalCategory { NOISE, CREATIVE_SIGNAL, DESTRUCTIVE_DRIFT };
    enum class BridgeStatus { NORMAL, DAMPING, RECOVERY, ISOLATED };

    class MetaEvaluator {
    public:
        [[nodiscard]] SignalCategory evaluate(double candidateState,
                                              double healthIndex,
                                              const IdentityInvariant& omega) const noexcept 
        {
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

        void updateBridgeState(double category) {
            switch (static_cast<SignalCategory>(category)) {
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

        [[nodiscard]] double getEffectiveTransmission() const noexcept {
            double spatialAttenuation = 1.0 / (1.0 + 0.1 * distance);
            return capacity * spatialAttenuation * orientationWeight;
        }
    };

    class AutopoieticNode {
    public:
        int id;
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
            invariant.baseline = initialBaseline;
        }

        AutopoieticNode(const AutopoieticNode& other) 
            : id(other.id), position(other.position), state(other.state.load()),
              healthIndex(other.healthIndex.load()), invariant(other.invariant),
              metaEvaluator(other.metaEvaluator), bridges(other.bridges) {}

        void updateHealth() noexcept {
            double drift = std::abs(state.load() - invariant.baseline);
            healthIndex.store(std::max(0.0, 1.0 - (drift / invariant.maxEpsilon)));
        }

        [[nodiscard]] double applyLocalReflexFilter(double rawInput) const noexcept {
            double maxAllowedReflexStep = 3.5;
            double currentState = state.load();
            double delta = rawInput - currentState;
            if (std::abs(delta) > maxAllowedReflexStep) {
                return currentState + (delta > 0 ? maxAllowedReflexStep : -maxAllowedReflexStep);
            }
            return rawInput;
        }
    };

    class SpatialAdaptiveMesh {
    private:
        std::vector<AutopoieticNode> nodes;
        double alpha = 0.15;

    public:
        void addNode(int id, Vector3D pos, double baseline) {
            nodes.emplace_back(id, pos, baseline);
        }

        void connectNodes(int nodeA, int nodeB) {
            double dist = nodes[nodeA].position.distanceTo(nodes[nodeB].position);
            double orientA = nodes[nodeA].position.orientationFactorTo(nodes[nodeB].position);
            double orientB = nodes[nodeB].position.orientationFactorTo(nodes[nodeA].position);

            nodes[nodeA].bridges.push_back({nodeB, dist, orientA, 1.0, BridgeStatus::NORMAL});
            nodes[nodeB].bridges.push_back({nodeA, dist, orientB, 1.0, BridgeStatus::NORMAL});
            
            enforceStabilityCondition();
        }

        void enforceStabilityCondition() noexcept {
            size_t maxDegree = 0;
            for (const auto& node : nodes) {
                maxDegree = std::max(maxDegree, node.bridges.size());
            }
            if (maxDegree > 0) {
                double safeAlpha = 1.0 / static_cast<double>(maxDegree);
                alpha = std::min(0.2, safeAlpha * 0.8);
            }
        }

        void pruneIsolatedBridges(double minCapacityThreshold = 0.05) {
            for (auto& node : nodes) {
                std::lock_guard<std::mutex> lock(node.nodeMutex);
                std::erase_if(node.bridges, [minCapacityThreshold](const SpatialBridge& b) {
                    return b.capacity < minCapacityThreshold || b.status == BridgeStatus::ISOLATED;
                });
            }
            enforceStabilityCondition();
        }

        void autoConnectNearbyNodes(double radius) {
            for (size_t i = 0; i < nodes.size(); ++i) {
                for (size_t j = i + 1; j < nodes.size(); ++j) {
                    if (nodes[i].position.distanceTo(nodes[j].position) <= radius) {
                        bool exists = std::any_of(nodes[i].bridges.begin(), nodes[i].bridges.end(),
                            [j](const SpatialBridge& b) { return b.targetNodeId == static_cast<int>(j); });
                        if (!exists) {
                            connectNodes(static_cast<int>(i), static_cast<int>(j));
                        }
                    }
                }
            }
        }

        void injectExternalShock(int targetNodeId, double shockMagnitude) {
            if (targetNodeId < 0 || targetNodeId >= static_cast<int>(nodes.size())) return;
            auto& node = nodes[targetNodeId];
            double filteredSignal = node.applyLocalReflexFilter(node.state.load() + shockMagnitude);
            node.state.store(filteredSignal);
            node.updateHealth();
        }

        void simulationStepAsync() {
            std::vector<double> nextStates(nodes.size());
            std::vector<std::jthread> workers;
            workers.reserve(nodes.size());

            for (size_t i = 0; i < nodes.size(); ++i) {
                workers.emplace_back([this, i, &nextStates]() {
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

                        bridge.updateBridgeState(static_cast<double>(category));
                        diffusionSum += bridge.getEffectiveTransmission() * deltaS;
                    }

                    nextStates[i] = currentState + alpha * diffusionSum;
                });
            }

            workers.clear(); // Waits for all threads to join

            for (size_t i = 0; i < nodes.size(); ++i) {
                nodes[i].state.store(nextStates[i]);
                nodes[i].updateHealth();
            }
        }

        [[nodiscard]] double getNodeState(int id) const {
            return nodes.at(id).state.load();
        }

        [[nodiscard]] double getNodeHealth(int id) const {
            return nodes.at(id).healthIndex.load();
        }

        [[nodiscard]] size_t getNodeBridgesCount(int id) const {
            return nodes.at(id).bridges.size();
        }
    };

} // namespace AdaptiveMesh

#endif // SYSTEM_ARCHITECTURE_HPP
