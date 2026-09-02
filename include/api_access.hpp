/**
 * @file api_access.hpp
 * @brief Validated, non-breaking API helpers for SOAM node and bridge state.
 */

#ifndef ADAPTIVE_MESH_API_ACCESS_HPP
#define ADAPTIVE_MESH_API_ACCESS_HPP

#include "system_architecture.hpp"

#include <cstddef>
#include <mutex>
#include <stdexcept>

namespace AdaptiveMesh::api {

struct NodeSnapshot {
    size_t id;
    Vector3D position;
    double state;
    double healthIndex;
    size_t bridgeCount;
};

inline void setNodeState(AutopoieticNode& node, double value) {
    requireFinite(value, "state");
    std::lock_guard lock(node.nodeMutex);
    node.state.store(value);
}

inline void setNodeHealthIndex(AutopoieticNode& node, double value) {
    requireFinite(value, "healthIndex");
    if (value < 0.0 || value > 1.0) {
        throw std::invalid_argument("healthIndex must be in [0, 1]");
    }
    std::lock_guard lock(node.nodeMutex);
    node.healthIndex.store(value);
}

inline void setNodePosition(AutopoieticNode& node, Vector3D position) {
    position.validate();
    std::lock_guard lock(node.nodeMutex);
    node.position = position;
}

[[nodiscard]] inline NodeSnapshot snapshotNode(const AutopoieticNode& node) {
    std::lock_guard lock(node.nodeMutex);
    node.position.validate();
    node.invariant.validate();
    const double state = node.state.load();
    const double healthIndex = node.healthIndex.load();
    requireFinite(state, "state");
    requireFinite(healthIndex, "healthIndex");
    if (healthIndex < 0.0 || healthIndex > 1.0) {
        throw std::runtime_error("node healthIndex violates [0, 1]");
    }
    return {node.id, node.position, state, healthIndex, node.bridges.size()};
}

inline void setBridgeCapacity(SpatialBridge& bridge, double value) {
    requireFinite(value, "bridge capacity");
    if (value < 0.0 || value > 1.0) {
        throw std::invalid_argument("bridge capacity must be in [0, 1]");
    }
    bridge.capacity = value;
}

inline void setBridgeGeometry(SpatialBridge& bridge, int targetNodeId,
                              double distance, double orientationWeight) {
    if (targetNodeId < 0) {
        throw std::invalid_argument("bridge targetNodeId must be non-negative");
    }
    requireFinite(distance, "bridge distance");
    requireFinite(orientationWeight, "bridge orientationWeight");
    if (distance < 0.0) {
        throw std::invalid_argument("bridge distance must be non-negative");
    }
    if (orientationWeight < 0.0 || orientationWeight > 1.0) {
        throw std::invalid_argument("bridge orientationWeight must be in [0, 1]");
    }
    bridge.targetNodeId = targetNodeId;
    bridge.distance = distance;
    bridge.orientationWeight = orientationWeight;
}

inline void validateBridge(const SpatialBridge& bridge) {
    if (bridge.targetNodeId < 0) {
        throw std::invalid_argument("bridge targetNodeId must be non-negative");
    }
    static_cast<void>(bridge.getEffectiveTransmission());
}

} // namespace AdaptiveMesh::api

#endif
