#include <cassert>
#include <iostream>
#include "system_architecture.hpp"

void test_stability_and_shock() {
    using namespace AdaptiveMesh;
    SpatialAdaptiveMesh mesh;

    // Create 3 nodes in 3D
    mesh.addNode(0, {0.0, 0.0, 0.0}, 1.618);
    mesh.addNode(1, {1.0, 1.0, 1.0}, 1.618);
    mesh.addNode(2, {2.0, 2.0, 2.0}, 1.618);

    mesh.connectNodes(0, 1);
    mesh.connectNodes(1, 2);

    // Initial state check
    assert(std::abs(mesh.getNodeState(0) - 1.618) < 1e-6);

    // Dynamic edge discovery test
    mesh.autoConnectNearbyNodes(3.0); // Should connect 0 and 2
    assert(mesh.getNodeBridgesCount(0) == 2);

    // Local Reflex Filter limit test (shock magnitude +5.0)
    mesh.injectExternalShock(0, 5.0);
    // Node 0 applyLocalReflexFilter should clamp delta to maxAllowedReflexStep (3.5)
    // 1.618 + 3.5 = 5.118
    assert(std::abs(mesh.getNodeState(0) - 5.118) < 1e-4);

    // Async simulation step test
    mesh.simulationStepAsync();
    
    // Check that health has degraded due to drift from baseline
    assert(mesh.getNodeHealth(0) < 1.0);

    // Prune isolated or failed bridges
    // Inject massive shock to Node 2 to isolate it
    mesh.injectExternalShock(2, 25.0); // Destructive drift -> Isolated
    mesh.simulationStepAsync();
    mesh.pruneIsolatedBridges(0.05);

    std::cout << "All SOAM RC Unit Tests Passed successfully!" << std::endl;
}

int main() {
    test_stability_and_shock();
    return 0;
}
