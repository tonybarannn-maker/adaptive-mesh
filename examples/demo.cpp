#include <iostream>
#include "system_architecture.hpp"

int main() {
    using namespace AdaptiveMesh;
    std::cout << "--- SOAM v1.1.0 Demonstration ---" << std::endl;
    
    SpatialAdaptiveMesh mesh;
    mesh.addNode(0, {0.0, 0.0, 0.0}, 1.618);
    mesh.addNode(1, {1.5, 0.0, 0.0}, 1.618);
    mesh.connectNodes(0, 1);

    std::cout << "Initial Node 0 state: " << mesh.getNodeState(0) << std::endl;
    std::cout << "Injecting creative signal +2.0..." << std::endl;
    mesh.injectExternalShock(0, 2.0);
    
    for (int step = 0; step < 5; ++step) {
        mesh.simulationStepAsync();
        std::cout << "Step " << step << " | Node 0: " << mesh.getNodeState(0) 
                  << " | Node 1: " << mesh.getNodeState(1) << std::endl;
    }

    return 0;
}
