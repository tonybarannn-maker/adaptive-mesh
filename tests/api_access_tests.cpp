#include "api_access.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Exception, typename Function>
void requireThrows(Function&& function, const char* message) {
    try {
        function();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error(message);
}

void test_node_helpers() {
    using namespace AdaptiveMesh;
    using namespace AdaptiveMesh::api;

    AutopoieticNode node(0, {0.0, 0.0, 0.0}, 1.0);
    setNodeState(node, 2.0);
    setNodeHealthIndex(node, 0.75);
    setNodePosition(node, {1.0, 2.0, 3.0});

    const NodeSnapshot snapshot = snapshotNode(node);
    require(snapshot.id == 0, "snapshot must preserve node ID");
    require(snapshot.position.x == 1.0 && snapshot.position.y == 2.0 && snapshot.position.z == 3.0,
            "snapshot must preserve validated position");
    require(snapshot.state == 2.0, "snapshot must expose current state");
    require(snapshot.healthIndex == 0.75, "snapshot must expose current health");
    require(snapshot.bridgeCount == 0, "snapshot must expose bridge count");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    requireThrows<std::invalid_argument>([&] { setNodeState(node, nan); },
                                          "NaN node state must be rejected");
    requireThrows<std::invalid_argument>([&] { setNodeHealthIndex(node, -0.1); },
                                          "negative health must be rejected");
    requireThrows<std::invalid_argument>([&] { setNodeHealthIndex(node, 1.1); },
                                          "health above one must be rejected");
    requireThrows<std::invalid_argument>([&] { setNodePosition(node, {nan, 0.0, 0.0}); },
                                          "non-finite position must be rejected");
}

void test_bridge_helpers() {
    using namespace AdaptiveMesh;
    using namespace AdaptiveMesh::api;

    SpatialBridge bridge{1, 2.0, 0.5};
    setBridgeCapacity(bridge, 0.65);
    setBridgeGeometry(bridge, 2, 3.0, 0.25);
    validateBridge(bridge);
    require(bridge.targetNodeId == 2, "bridge helper must update target ID");
    require(std::abs(bridge.distance - 3.0) < 1e-12, "bridge helper must update distance");
    require(std::abs(bridge.orientationWeight - 0.25) < 1e-12,
            "bridge helper must update orientation weight");
    require(std::abs(bridge.capacity - 0.65) < 1e-12,
            "bridge helper must update capacity");

    requireThrows<std::invalid_argument>([&] { setBridgeCapacity(bridge, -0.1); },
                                          "negative capacity must be rejected");
    requireThrows<std::invalid_argument>([&] { setBridgeCapacity(bridge, 1.1); },
                                          "capacity above one must be rejected");
    requireThrows<std::invalid_argument>([&] { setBridgeGeometry(bridge, -1, 1.0, 0.5); },
                                          "negative target ID must be rejected");
    requireThrows<std::invalid_argument>([&] { setBridgeGeometry(bridge, 1, -1.0, 0.5); },
                                          "negative distance must be rejected");
    requireThrows<std::invalid_argument>([&] { setBridgeGeometry(bridge, 1, 1.0, 1.1); },
                                          "orientation above one must be rejected");
}

} // namespace

int main() {
    try {
        test_node_helpers();
        test_bridge_helpers();
        std::cout << "Adaptive Mesh API access tests passed." << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Adaptive Mesh API access tests failed: " << error.what() << std::endl;
        return 1;
    }
}
