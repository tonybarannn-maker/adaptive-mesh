#include "system_architecture.hpp"

#include <cstdlib>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<AdaptiveMesh::SpatialAdaptiveMesh>);
static_assert(!std::is_copy_assignable_v<AdaptiveMesh::SpatialAdaptiveMesh>);
static_assert(!std::is_move_constructible_v<AdaptiveMesh::SpatialAdaptiveMesh>);
static_assert(!std::is_move_assignable_v<AdaptiveMesh::SpatialAdaptiveMesh>);

int main() {
    AdaptiveMesh::SpatialAdaptiveMesh mesh;
    mesh.addNode(0, {0.0, 0.0, 0.0}, 1.0);
    mesh.addNode(1, {1.0, 0.0, 0.0}, 1.0);
    mesh.connectNodes(0, 1);
    if (mesh.getNodeBridgesCount(0) != 1) return EXIT_FAILURE;
    auto evaluator = mesh.productionTransitionEvaluator();
    return evaluator.evaluate({0, 1}) ==
            AdaptiveMesh::ProductionTransitionEvaluation::not_eligible
        ? EXIT_SUCCESS : EXIT_FAILURE;
}
