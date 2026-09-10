#if SOAM_DOMAIN_ONLY
#include "soam_domain.hpp"

int main() {
    const AdaptiveMesh::InteractionObservation observation{0.5};
    return observation.compatibility() == 0.5 ? 0 : 1;
}
#else
#include "system_architecture.hpp"

int main() {
    AdaptiveMesh::SpatialAdaptiveMesh mesh;
    mesh.addNode(0, {0.0, 0.0, 0.0}, 1.0);
    return mesh.getNodeState(0) == 1.0 ? 0 : 1;
}
#endif
