#include "system_architecture.hpp"

#include "interaction_observation.hpp"
#include "bridge_confidence.hpp"
#include "adaptive_bridge_policy.hpp"
#include "bridge_persistence.hpp"
#include "bridge_transition_authorization.hpp"

#include <cassert>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
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

void test_topology_contract() {
    using namespace AdaptiveMesh;

    SpatialAdaptiveMesh mesh;
    mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);

    requireThrows<std::invalid_argument>(
        [&mesh] { mesh.addNode(42, {1.0, 0.0, 0.0}, 0.0); },
        "node ID must match insertion index"
    );

    mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);

    requireThrows<std::out_of_range>([&mesh] { mesh.connectNodes(-1, 0); },
                                     "negative node ID must throw out_of_range");
    requireThrows<std::out_of_range>([&mesh] { mesh.connectNodes(0, 2); },
                                     "out-of-range node ID must throw out_of_range");
    requireThrows<std::invalid_argument>([&mesh] { mesh.connectNodes(0, 0); },
                                          "self-connection must throw invalid_argument");

    requireThrows<std::out_of_range>([&mesh] { mesh.injectExternalShock(-1, 1.0); },
                                     "negative shock target must throw out_of_range");
    requireThrows<std::out_of_range>([&mesh] { mesh.injectExternalShock(2, 1.0); },
                                     "out-of-range shock target must throw out_of_range");

    mesh.connectNodes(0, 1);
    require(mesh.getNodeBridgesCount(0) == 1, "connection must create a forward bridge");
    require(mesh.getNodeBridgesCount(1) == 1, "connection must create a reverse bridge");

    requireThrows<std::invalid_argument>([&mesh] { mesh.connectNodes(0, 1); },
                                          "duplicate bridge pair must throw invalid_argument");
    require(mesh.getNodeBridgesCount(0) == 1,
            "duplicate connection must not add a second forward bridge");
    require(mesh.getNodeBridgesCount(1) == 1,
            "duplicate connection must not add a second reverse bridge");

    mesh.injectExternalShock(0, 25.0);
    mesh.injectExternalShock(0, 25.0);
    mesh.injectExternalShock(0, 25.0);
    mesh.simulationStep();
    mesh.pruneIsolatedBridges();
    require(mesh.getNodeBridgesCount(0) == 0, "pruning must remove forward bridge with its pair");
    require(mesh.getNodeBridgesCount(1) == 0, "pruning must remove reverse bridge with its pair");
}

void test_bulk_connection_contract() {
    using namespace AdaptiveMesh;
    const std::vector<std::pair<int, int>> pairs{{0, 1}, {1, 2}};
    SpatialAdaptiveMesh bulkMesh;
    SpatialAdaptiveMesh individualMesh;
    for (size_t nodeId = 0; nodeId < 3; ++nodeId) {
        const Vector3D position{static_cast<double>(nodeId), 0.0, 0.0};
        bulkMesh.addNode(nodeId, position, 0.0);
        individualMesh.addNode(nodeId, position, 0.0);
    }

    bulkMesh.connectNodePairs(pairs);
    individualMesh.connectNodes(0, 1);
    individualMesh.connectNodes(1, 2);
    bulkMesh.injectExternalShock(0, 2.0);
    individualMesh.injectExternalShock(0, 2.0);
    bulkMesh.simulationStep();
    individualMesh.simulationStep();
    for (size_t nodeId = 0; nodeId < 3; ++nodeId) {
        require(bulkMesh.getNodeBridgesCount(nodeId) == individualMesh.getNodeBridgesCount(nodeId),
                "bulk and individual topology bridge counts must match");
        require(std::abs(bulkMesh.getNodeState(nodeId) - individualMesh.getNodeState(nodeId)) < 1e-12,
                "bulk and individual topology state must match");
    }

    SpatialAdaptiveMesh invalidMesh;
    invalidMesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
    invalidMesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
    invalidMesh.addNode(2, {2.0, 0.0, 0.0}, 0.0);
    requireThrows<std::out_of_range>([&invalidMesh] {
        invalidMesh.connectNodePairs({{0, 1}, {1, 3}});
    }, "invalid pair must reject the entire batch");
    require(invalidMesh.getNodeBridgesCount(0) == 0 &&
                invalidMesh.getNodeBridgesCount(1) == 0 &&
                invalidMesh.getNodeBridgesCount(2) == 0,
            "invalid batch must not partially mutate topology");
    requireThrows<std::invalid_argument>([&invalidMesh] {
        invalidMesh.connectNodePairs({{0, 1}, {1, 0}});
    }, "duplicate pair in batch must be rejected");
    invalidMesh.connectNodes(0, 1);
    requireThrows<std::invalid_argument>([&invalidMesh] {
        invalidMesh.connectNodePairs({{1, 0}});
    }, "existing pair in batch must be rejected");
}

void test_numeric_input_contract() {
    using namespace AdaptiveMesh;

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    const double negativeInfinity = -std::numeric_limits<double>::infinity();
    SpatialAdaptiveMesh mesh;

    requireThrows<std::invalid_argument>([&mesh, nan] {
        mesh.addNode(0, {nan, 0.0, 0.0}, 0.0);
    }, "NaN coordinate must be rejected");
    requireThrows<std::invalid_argument>([&mesh, infinity] {
        mesh.addNode(0, {0.0, 0.0, 0.0}, infinity);
    }, "infinite baseline must be rejected");

    mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
    mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
    requireThrows<std::invalid_argument>([&mesh, nan] {
        mesh.autoConnectNearbyNodes(nan);
    }, "NaN radius must be rejected");
    requireThrows<std::invalid_argument>([&mesh, infinity] {
        mesh.injectExternalShock(0, infinity);
    }, "infinite shock must be rejected");
    requireThrows<std::invalid_argument>([&mesh, negativeInfinity] {
        mesh.injectExternalShock(0, negativeInfinity);
    }, "negative infinite shock must be rejected");
    requireThrows<std::invalid_argument>([&mesh, nan] {
        mesh.pruneIsolatedBridges(nan);
    }, "NaN pruning threshold must be rejected");

    SpatialBridge bridge{1, 1.0, 0.5};
    bridge.updateBridgeState(SignalCategory::NOISE);
    require(bridge.status == BridgeStatus::DAMPING,
            "bridge state update must accept SignalCategory");
    SpatialBridge invalidBridge{1, infinity, 0.5};
    requireThrows<std::invalid_argument>([&invalidBridge] {
        static_cast<void>(invalidBridge.getEffectiveCoupling());
    }, "infinite bridge distance must be rejected");

    SpatialBridge negativeDistanceBridge{1, -1.0, 0.5};
    requireThrows<std::invalid_argument>([&negativeDistanceBridge] {
        static_cast<void>(negativeDistanceBridge.getEffectiveCoupling());
    }, "negative bridge distance must be rejected");

    SpatialBridge invalidOrientationLow{1, 1.0, -0.01};
    requireThrows<std::invalid_argument>([&invalidOrientationLow] {
        static_cast<void>(invalidOrientationLow.getEffectiveCoupling());
    }, "negative bridge orientationWeight must be rejected");

    SpatialBridge invalidOrientationHigh{1, 1.0, 1.01};
    requireThrows<std::invalid_argument>([&invalidOrientationHigh] {
        static_cast<void>(invalidOrientationHigh.getEffectiveCoupling());
    }, "bridge orientationWeight above one must be rejected");

    SpatialBridge invalidCapacityLow{1, 1.0, 0.5, -0.01};
    requireThrows<std::invalid_argument>([&invalidCapacityLow] {
        static_cast<void>(invalidCapacityLow.getEffectiveCoupling());
    }, "negative bridge capacity must be rejected");

    SpatialBridge invalidCapacityHigh{1, 1.0, 0.5, 1.01};
    requireThrows<std::invalid_argument>([&invalidCapacityHigh] {
        static_cast<void>(invalidCapacityHigh.getEffectiveCoupling());
    }, "bridge capacity above one must be rejected");

    SpatialBridge validBridge{1, 2.0, 0.5, 0.8};
    const double coupling = validBridge.getEffectiveCoupling();
    require(std::isfinite(coupling), "valid bridge coupling must be finite");

    const double legacyTransmission = validBridge.getEffectiveTransmission();
    require(coupling == legacyTransmission,
            "legacy transmission API must delegate to canonical coupling API");
}

void test_interaction_observation_contract() {
    using AdaptiveMesh::InteractionObservation;

    const InteractionObservation zero(0.0);
    require(zero.compatibility() == 0.0,
            "zero interaction compatibility must be preserved");

    const InteractionObservation one(1.0);
    require(one.compatibility() == 1.0,
            "unit interaction compatibility must be preserved");

    const InteractionObservation midpoint(0.375);
    require(midpoint.compatibility() == 0.375,
            "intermediate interaction compatibility must be preserved");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    const double negativeInfinity = -infinity;

    requireThrows<std::invalid_argument>([nan] {
        static_cast<void>(InteractionObservation(nan));
    }, "NaN interaction compatibility must be rejected");
    requireThrows<std::invalid_argument>([infinity] {
        static_cast<void>(InteractionObservation(infinity));
    }, "positive infinite interaction compatibility must be rejected");
    requireThrows<std::invalid_argument>([negativeInfinity] {
        static_cast<void>(InteractionObservation(negativeInfinity));
    }, "negative infinite interaction compatibility must be rejected");
    requireThrows<std::invalid_argument>([] {
        static_cast<void>(InteractionObservation(-0.001));
    }, "negative interaction compatibility must be rejected");
    requireThrows<std::invalid_argument>([] {
        static_cast<void>(InteractionObservation(1.001));
    }, "out-of-range interaction compatibility must be rejected");
}

void test_bridge_confidence_contract() {
    using AdaptiveMesh::BridgeConfidence;

    static_assert(!std::is_default_constructible_v<BridgeConfidence>);
    static_assert(std::is_constructible_v<BridgeConfidence, double>);
    static_assert(!std::is_convertible_v<double, BridgeConfidence>);
    static_assert(noexcept(std::declval<const BridgeConfidence&>().value()));

    const BridgeConfidence zero(0.0);
    require(zero.value() == 0.0,
            "zero bridge confidence must be preserved");

    const BridgeConfidence one(1.0);
    require(one.value() == 1.0,
            "unit bridge confidence must be preserved");

    const BridgeConfidence midpoint(0.625);
    require(midpoint.value() == 0.625,
            "intermediate bridge confidence must be preserved");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    const double negativeInfinity = -infinity;

    requireThrows<std::invalid_argument>([nan] {
        static_cast<void>(BridgeConfidence(nan));
    }, "NaN bridge confidence must be rejected");
    requireThrows<std::invalid_argument>([infinity] {
        static_cast<void>(BridgeConfidence(infinity));
    }, "positive infinite bridge confidence must be rejected");
    requireThrows<std::invalid_argument>([negativeInfinity] {
        static_cast<void>(BridgeConfidence(negativeInfinity));
    }, "negative infinite bridge confidence must be rejected");
    requireThrows<std::invalid_argument>([] {
        static_cast<void>(BridgeConfidence(-0.001));
    }, "negative bridge confidence must be rejected");
    requireThrows<std::invalid_argument>([] {
        static_cast<void>(BridgeConfidence(1.001));
    }, "out-of-range bridge confidence must be rejected");
}

void test_adaptive_bridge_policy_contract() {
    using AdaptiveMesh::AdaptiveBridgePolicy;
    using AdaptiveMesh::BridgeConfidence;
    using AdaptiveMesh::BridgePolicyEvidence;
    using AdaptiveMesh::InteractionObservation;

    static_assert(!std::is_default_constructible_v<BridgePolicyEvidence>);
    static_assert(!std::is_constructible_v<BridgePolicyEvidence, double>);
    static_assert(noexcept(
        std::declval<const BridgePolicyEvidence&>().value()));
    static_assert(noexcept(
        std::declval<const AdaptiveBridgePolicy&>().evaluate(
            std::declval<const InteractionObservation&>(),
            std::declval<const BridgeConfidence&>())));

    const AdaptiveBridgePolicy policy;
    const BridgeConfidence fullConfidence(1.0);

    require(policy.evaluate(InteractionObservation(0.0), fullConfidence).value() == -1.0,
            "zero compatibility must produce full negative evidence");
    require(policy.evaluate(InteractionObservation(0.25), fullConfidence).value() == -0.5,
            "quarter compatibility must produce negative evidence");
    require(policy.evaluate(InteractionObservation(0.5), fullConfidence).value() == 0.0,
            "neutral compatibility must produce zero evidence");
    require(policy.evaluate(InteractionObservation(0.75), fullConfidence).value() == 0.5,
            "three-quarter compatibility must produce positive evidence");
    require(policy.evaluate(InteractionObservation(1.0), fullConfidence).value() == 1.0,
            "unit compatibility must produce full positive evidence");

    require(policy.evaluate(InteractionObservation(0.0), BridgeConfidence(0.0)).value() == 0.0,
            "zero confidence must produce zero evidence");
    require(policy.evaluate(InteractionObservation(0.5), BridgeConfidence(0.0)).value() == 0.0,
            "zero confidence at neutral compatibility must produce zero evidence");
    require(policy.evaluate(InteractionObservation(1.0), BridgeConfidence(0.0)).value() == 0.0,
            "zero confidence at full compatibility must produce zero evidence");

    const double lowerMagnitude =
        policy.evaluate(InteractionObservation(0.75), BridgeConfidence(0.4)).value();
    const double higherMagnitude =
        policy.evaluate(InteractionObservation(0.75), BridgeConfidence(0.8)).value();
    require(higherMagnitude > lowerMagnitude && higherMagnitude >= 0.0,
            "positive evidence magnitude must increase with confidence");

    const double lowerNegativeMagnitude =
        policy.evaluate(InteractionObservation(0.25), BridgeConfidence(0.4)).value();
    const double higherNegativeMagnitude =
        policy.evaluate(InteractionObservation(0.25), BridgeConfidence(0.8)).value();
    require(std::abs(lowerNegativeMagnitude + 0.2) < 1e-12,
            "lower negative confidence must produce -0.2 evidence");
    require(std::abs(higherNegativeMagnitude + 0.4) < 1e-12,
            "higher negative confidence must produce -0.4 evidence");
    require(higherNegativeMagnitude < lowerNegativeMagnitude,
            "negative evidence must strengthen with confidence");

    const double negativeEvidence =
        policy.evaluate(InteractionObservation(0.25), BridgeConfidence(0.8)).value();
    require(std::abs(negativeEvidence + higherMagnitude) < 1e-12,
            "complementary compatibility must produce symmetric evidence");

    const double boundedEvidence =
        policy.evaluate(InteractionObservation(0.9), BridgeConfidence(0.9)).value();
    require(boundedEvidence >= -1.0 && boundedEvidence <= 1.0,
            "policy evidence must remain bounded in [-1, 1]");

    const double lowCompatibility =
        policy.evaluate(InteractionObservation(0.2), fullConfidence).value();
    const double highCompatibility =
        policy.evaluate(InteractionObservation(0.8), fullConfidence).value();
    require(highCompatibility > lowCompatibility,
            "evidence must be monotonic in compatibility");
}

void test_bridge_persistence_contract() {
    using AdaptiveMesh::BridgePersistence;
    using AdaptiveMesh::PersistentBridgeRecommendation;
    using AdaptiveMesh::AdaptiveBridgePolicy;
    using AdaptiveMesh::BridgeConfidence;
    using AdaptiveMesh::InteractionObservation;

    static_assert(!std::is_default_constructible_v<BridgePersistence>);
    static_assert(!std::is_constructible_v<BridgePersistence, std::size_t>);
    static_assert(noexcept(std::declval<BridgePersistence&>().observe(
        std::declval<const AdaptiveMesh::BridgePolicyEvidence&>())));
    static_assert(noexcept(std::declval<BridgePersistence&>().reset()));

    const AdaptiveBridgePolicy policy;
    const BridgeConfidence fullConfidence(1.0);
    const auto evidence = [&policy, &fullConfidence](double compatibility) {
        return policy.evaluate(
            InteractionObservation(compatibility), fullConfidence);
    };

    requireThrows<std::invalid_argument>([] {
        static_cast<void>(BridgePersistence(0.5, 0.5, 2, 2));
    }, "equal persistence thresholds must be rejected");
    requireThrows<std::invalid_argument>([] {
        static_cast<void>(BridgePersistence(0.5, 0.6, 2, 2));
    }, "reversed persistence thresholds must be rejected");
    requireThrows<std::invalid_argument>([] {
        static_cast<void>(BridgePersistence(0.5, -0.01, 2, 2));
    }, "negative release threshold must be rejected");
    requireThrows<std::invalid_argument>([] {
        static_cast<void>(BridgePersistence(1.01, 0.25, 2, 2));
    }, "activation threshold above one must be rejected");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    requireThrows<std::invalid_argument>([nan] {
        static_cast<void>(BridgePersistence(nan, 0.25, 2, 2));
    }, "NaN activation threshold must be rejected");
    requireThrows<std::invalid_argument>([nan] {
        static_cast<void>(BridgePersistence(0.5, nan, 2, 2));
    }, "NaN release threshold must be rejected");
    requireThrows<std::invalid_argument>([infinity] {
        static_cast<void>(BridgePersistence(infinity, 0.25, 2, 2));
    }, "positive infinite activation threshold must be rejected");
    requireThrows<std::invalid_argument>([infinity] {
        static_cast<void>(BridgePersistence(0.5, -infinity, 2, 2));
    }, "negative infinite release threshold must be rejected");
    requireThrows<std::invalid_argument>([infinity] {
        static_cast<void>(BridgePersistence(0.5, infinity, 2, 2));
    }, "positive infinite release threshold must be rejected");
    requireThrows<std::invalid_argument>([] {
        static_cast<void>(BridgePersistence(0.5, 0.2, 1, 2));
    }, "activation sample count below two must be rejected");
    requireThrows<std::invalid_argument>([] {
        static_cast<void>(BridgePersistence(0.5, 0.2, 2, 1));
    }, "release sample count below two must be rejected");

    BridgePersistence boundaryPersistence(0.5, 0.25, 2, 2);
    require(std::abs(evidence(0.625).value() - 0.25) < 1e-12,
            "positive boundary fixture must produce +0.25 evidence");
    require(boundaryPersistence.observe(evidence(0.75)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "positive evidence at activation boundary must start a streak");
    require(boundaryPersistence.observe(evidence(0.75)) ==
                PersistentBridgeRecommendation::SUPPORT,
            "positive evidence at activation boundary must confirm");
    require(boundaryPersistence.observe(evidence(0.625)) ==
                PersistentBridgeRecommendation::SUPPORT,
            "positive evidence at release boundary must start release");
    require(boundaryPersistence.observe(evidence(0.625)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "positive evidence at release boundary must release support");

    BridgePersistence negativeBoundaryPersistence(0.5, 0.25, 2, 2);
    require(std::abs(evidence(0.375).value() + 0.25) < 1e-12,
            "negative boundary fixture must produce -0.25 evidence");
    require(negativeBoundaryPersistence.observe(evidence(0.25)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "negative evidence at activation boundary must start a streak");
    require(negativeBoundaryPersistence.observe(evidence(0.25)) ==
                PersistentBridgeRecommendation::CONSTRAIN,
            "negative evidence at activation boundary must confirm");
    require(negativeBoundaryPersistence.observe(evidence(0.375)) ==
                PersistentBridgeRecommendation::CONSTRAIN,
            "negative evidence at release boundary must start release");
    require(negativeBoundaryPersistence.observe(evidence(0.375)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "negative evidence at release boundary must release constraint");

    BridgePersistence resetPersistence(0.5, 0.25, 2, 2);
    require(resetPersistence.observe(evidence(0.75)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "partial activation must preserve");
    require(resetPersistence.observe(evidence(0.5)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "neutral evidence must clear partial activation");
    require(resetPersistence.observe(evidence(0.75)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "post-neutral activation must start a new streak");
    require(resetPersistence.observe(evidence(0.75)) ==
                PersistentBridgeRecommendation::SUPPORT,
            "post-neutral activation must require two samples");

    BridgePersistence reversalPersistence(0.5, 0.25, 2, 2);
    require(reversalPersistence.observe(evidence(0.75)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "initial positive candidate must preserve");
    require(reversalPersistence.observe(evidence(0.25)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "direction change must reset the old streak");
    require(reversalPersistence.observe(evidence(0.25)) ==
                PersistentBridgeRecommendation::CONSTRAIN,
            "new negative candidate must require two samples");

    BridgePersistence persistence(0.5, 0.25, 3, 2);
    require(persistence.observe(evidence(0.0)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "first negative evidence must preserve");
    require(persistence.observe(evidence(0.0)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "second negative evidence must preserve");
    require(persistence.observe(evidence(0.0)) ==
                PersistentBridgeRecommendation::CONSTRAIN,
            "third negative evidence must constrain");

    require(persistence.observe(evidence(0.5)) ==
                PersistentBridgeRecommendation::CONSTRAIN,
            "first release sample must preserve constrained state");
    require(persistence.observe(evidence(0.5)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "second release sample must return to preserve");
    require(persistence.observe(evidence(1.0)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "opposite direction must not jump directly to support");

    BridgePersistence strengthenedReversal(0.5, 0.25, 2, 2);
    require(strengthenedReversal.observe(evidence(1.0)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "support activation sample must preserve");
    require(strengthenedReversal.observe(evidence(1.0)) ==
                PersistentBridgeRecommendation::SUPPORT,
            "support must be initially confirmed");
    require(strengthenedReversal.observe(evidence(0.05)) ==
                PersistentBridgeRecommendation::SUPPORT,
            "first opposite sample must only start release");
    require(strengthenedReversal.observe(evidence(0.05)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "second opposite sample must release to preserve");
    require(strengthenedReversal.observe(evidence(0.05)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "first post-release constrain sample must preserve");
    require(strengthenedReversal.observe(evidence(0.05)) ==
                PersistentBridgeRecommendation::CONSTRAIN,
            "second post-release constrain sample must confirm constrain");

    BridgePersistence supportPersistence(0.5, 0.25, 3, 2);
    require(supportPersistence.observe(evidence(1.0)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "first support sample must preserve");
    require(supportPersistence.observe(evidence(1.0)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "second support sample must preserve");
    require(supportPersistence.observe(evidence(1.0)) ==
                PersistentBridgeRecommendation::SUPPORT,
            "third support sample must support");
    require(supportPersistence.observe(evidence(1.0)) ==
                PersistentBridgeRecommendation::SUPPORT,
            "support recommendation must be level output");

    require(supportPersistence.observe(evidence(1.0)) ==
                PersistentBridgeRecommendation::SUPPORT,
            "saturated support streak must remain a level output");

    supportPersistence.reset();
    require(supportPersistence.observe(evidence(1.0)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "reset must clear confirmed state");

    BridgePersistence isolatedA(0.5, 0.25, 2, 2);
    BridgePersistence isolatedB(0.5, 0.25, 2, 2);
    require(isolatedA.observe(evidence(1.0)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "first isolated A sample must preserve");
    require(isolatedB.observe(evidence(1.0)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "first isolated B sample must preserve");
    require(isolatedA.observe(evidence(1.0)) ==
                PersistentBridgeRecommendation::SUPPORT,
            "isolated A must confirm independently");
    require(isolatedB.observe(evidence(0.5)) ==
                PersistentBridgeRecommendation::PRESERVE,
            "isolated B must retain independent pending state");
}

void test_bridge_transition_authorization_contract() {
    using AdaptiveMesh::BridgeTransitionAuthorizationPolicy;
    using AdaptiveMesh::BridgeTransitionIntent;
    using AdaptiveMesh::BridgeTransitionPermissions;
    using AdaptiveMesh::PersistentBridgeRecommendation;

    static_assert(!std::is_default_constructible_v<BridgeTransitionPermissions>);
    static_assert(std::is_constructible_v<BridgeTransitionPermissions, bool, bool>);
    static_assert(!std::is_assignable_v<BridgeTransitionPermissions&, BridgeTransitionPermissions>);
    static_assert(noexcept(BridgeTransitionPermissions(true, true)));
    static_assert(noexcept(std::declval<const BridgeTransitionPermissions&>().allowsConstrain()));
    static_assert(noexcept(std::declval<const BridgeTransitionPermissions&>().allowsSupport()));
    static_assert(noexcept(std::declval<const BridgeTransitionAuthorizationPolicy&>().evaluate(
        std::declval<PersistentBridgeRecommendation>(),
        std::declval<const BridgeTransitionPermissions&>())));

    const BridgeTransitionAuthorizationPolicy policy;
    const BridgeTransitionPermissions maximum(true, true);
    const BridgeTransitionPermissions none(false, false);
    const BridgeTransitionPermissions constrainOnly(true, false);
    const BridgeTransitionPermissions supportOnly(false, true);

    const PersistentBridgeRecommendation recommendations[] = {
        PersistentBridgeRecommendation::PRESERVE,
        PersistentBridgeRecommendation::CONSTRAIN,
        PersistentBridgeRecommendation::SUPPORT};
    const BridgeTransitionPermissions permissions[] = {
        none, constrainOnly, supportOnly, maximum};
    const BridgeTransitionIntent expected[3][4] = {
        {BridgeTransitionIntent::PRESERVE, BridgeTransitionIntent::PRESERVE,
         BridgeTransitionIntent::PRESERVE, BridgeTransitionIntent::PRESERVE},
        {BridgeTransitionIntent::PRESERVE, BridgeTransitionIntent::CONSTRAIN,
         BridgeTransitionIntent::PRESERVE, BridgeTransitionIntent::CONSTRAIN},
        {BridgeTransitionIntent::PRESERVE, BridgeTransitionIntent::PRESERVE,
         BridgeTransitionIntent::SUPPORT, BridgeTransitionIntent::SUPPORT}};

    for (std::size_t recommendationIndex = 0;
         recommendationIndex < 3;
         ++recommendationIndex) {
        for (std::size_t permissionIndex = 0;
             permissionIndex < 4;
             ++permissionIndex) {
            require(policy.evaluate(
                        recommendations[recommendationIndex],
                        permissions[permissionIndex]) ==
                        expected[recommendationIndex][permissionIndex],
                    "authorization truth table mismatch");
        }
    }

    const auto invalidRecommendation =
        static_cast<PersistentBridgeRecommendation>(99);
    require(policy.evaluate(invalidRecommendation, maximum) ==
                BridgeTransitionIntent::PRESERVE,
            "invalid recommendation must fail closed");

    require(policy.evaluate(PersistentBridgeRecommendation::CONSTRAIN, none) ==
                BridgeTransitionIntent::PRESERVE,
            "denied constrain must preserve");
    require(policy.evaluate(PersistentBridgeRecommendation::SUPPORT, none) ==
                BridgeTransitionIntent::PRESERVE,
            "denied support must preserve");
    require(policy.evaluate(PersistentBridgeRecommendation::CONSTRAIN, maximum) ==
                BridgeTransitionIntent::CONSTRAIN,
            "allowed constrain direction must not invert");
    require(policy.evaluate(PersistentBridgeRecommendation::SUPPORT, maximum) ==
                BridgeTransitionIntent::SUPPORT,
            "allowed support direction must not invert");

    require(policy.evaluate(PersistentBridgeRecommendation::CONSTRAIN, maximum) ==
                policy.evaluate(PersistentBridgeRecommendation::CONSTRAIN, constrainOnly),
            "adding permissions must not block constrain");
    require(policy.evaluate(PersistentBridgeRecommendation::SUPPORT, maximum) ==
                policy.evaluate(PersistentBridgeRecommendation::SUPPORT, supportOnly),
            "adding permissions must not block support");
    require(policy.evaluate(PersistentBridgeRecommendation::PRESERVE, maximum) ==
                BridgeTransitionIntent::PRESERVE,
            "preserve recommendation must be idempotent");
}

void populateLinearMesh(AdaptiveMesh::SpatialAdaptiveMesh& mesh, size_t nodeCount) {
    for (size_t nodeId = 0; nodeId < nodeCount; ++nodeId) {
        mesh.addNode(nodeId, {static_cast<double>(nodeId), 0.0, 0.0}, 0.0);
    }
    for (size_t nodeId = 1; nodeId < nodeCount; ++nodeId) {
        mesh.connectNodes(static_cast<int>(nodeId - 1), static_cast<int>(nodeId));
    }
}

void test_worker_configuration_is_deterministic() {
    using namespace AdaptiveMesh;

    SpatialAdaptiveMesh singleWorkerMesh(1);
    SpatialAdaptiveMesh multiWorkerMesh(2);
    populateLinearMesh(singleWorkerMesh, 3);
    populateLinearMesh(multiWorkerMesh, 3);
    singleWorkerMesh.injectExternalShock(0, 2.0);
    multiWorkerMesh.injectExternalShock(0, 2.0);
    singleWorkerMesh.simulationStep();
    multiWorkerMesh.simulationStep();

    for (size_t nodeId = 0; nodeId < 3; ++nodeId) {
        require(std::abs(singleWorkerMesh.getNodeState(nodeId) -
                         multiWorkerMesh.getNodeState(nodeId)) < 1e-12,
                "worker configuration must preserve node state");
        require(std::abs(singleWorkerMesh.getNodeHealth(nodeId) -
                         multiWorkerMesh.getNodeHealth(nodeId)) < 1e-12,
                "worker configuration must preserve node health");
        require(singleWorkerMesh.getNodeBridgesCount(nodeId) ==
                    multiWorkerMesh.getNodeBridgesCount(nodeId),
                "worker configuration must preserve topology");
    }
}

void test_legacy_simulation_step_wrapper() {
    using namespace AdaptiveMesh;

    SpatialAdaptiveMesh mesh(1);
    populateLinearMesh(mesh, 2);
    mesh.injectExternalShock(0, 2.0);
    mesh.simulationStepAsync();

    require(std::isfinite(mesh.getNodeState(0)),
            "legacy simulationStepAsync wrapper must complete synchronously");
}

void test_post_commit_health_remains_finite_for_large_drift() {
    using namespace AdaptiveMesh;

    SpatialAdaptiveMesh mesh(1);
    constexpr double largeBaseline = 1.0e308;
    mesh.addNode(0, {0.0, 0.0, 0.0}, largeBaseline);
    mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
    mesh.connectNodes(0, 1);

    mesh.simulationStep();

    require(std::isfinite(mesh.getNodeState(0)),
            "large finite drift must not produce a non-finite state");
    require(std::isfinite(mesh.getNodeHealth(0)),
            "post-commit health must remain finite for large drift");
    require(std::isfinite(mesh.getNodeHealth(1)),
            "neighbor health must remain finite for large drift");
}

void requireFiniteNodeOutputs(const AdaptiveMesh::SpatialAdaptiveMesh& mesh, size_t nodeCount) {
    for (size_t nodeId = 0; nodeId < nodeCount; ++nodeId) {
        require(std::isfinite(mesh.getNodeState(nodeId)), "state must remain finite after topology mutation");
        require(std::isfinite(mesh.getNodeHealth(nodeId)), "health must remain finite after topology mutation");
    }
}

void test_simulation_buffer_shape_invalidation() {
    using namespace AdaptiveMesh;

    {
        SpatialAdaptiveMesh mesh(1);
        mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
        mesh.simulationStep();
        mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
        mesh.simulationStep();
        requireFiniteNodeOutputs(mesh, 2);
    }

    {
        SpatialAdaptiveMesh mesh(1);
        mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
        mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
        mesh.simulationStep();
        mesh.connectNodes(0, 1);
        mesh.simulationStep();
        requireFiniteNodeOutputs(mesh, 2);
    }

    {
        SpatialAdaptiveMesh mesh(1);
        for (size_t nodeId = 0; nodeId < 3; ++nodeId) {
            mesh.addNode(nodeId, {static_cast<double>(nodeId), 0.0, 0.0}, 0.0);
        }
        mesh.simulationStep();
        mesh.connectNodePairs({{0, 1}, {1, 2}});
        mesh.simulationStep();
        requireFiniteNodeOutputs(mesh, 3);
    }

    {
        SpatialAdaptiveMesh mesh(1);
        mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
        mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
        mesh.connectNodes(0, 1);
        mesh.injectExternalShock(0, 25.0);
        mesh.injectExternalShock(0, 25.0);
        mesh.injectExternalShock(0, 25.0);
        mesh.simulationStep();
        mesh.pruneIsolatedBridges();
        mesh.simulationStep();
        require(mesh.getNodeBridgesCount(0) == 0, "prune must remove the forward bridge");
        requireFiniteNodeOutputs(mesh, 2);
    }

    {
        SpatialAdaptiveMesh mesh(1);
        mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
        mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
        mesh.simulationStep();
        mesh.autoConnectNearbyNodes(2.0);
        mesh.simulationStep();
        require(mesh.getNodeBridgesCount(0) == 1, "auto-connect must add the edge");
        requireFiniteNodeOutputs(mesh, 2);
    }

    {
        SpatialAdaptiveMesh mesh(1);
        mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
        mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
        mesh.simulationStep();
        mesh.connectNodePairs({});
        mesh.pruneIsolatedBridges();
        mesh.autoConnectNearbyNodes(0.0);
        mesh.simulationStep();
        requireFiniteNodeOutputs(mesh, 2);
    }
}

void test_bounded_worker_scale_smoke(size_t nodeCount) {
    using namespace AdaptiveMesh;

    SpatialAdaptiveMesh mesh(4);
    populateLinearMesh(mesh, nodeCount);
    mesh.injectExternalShock(0, 2.0);
    mesh.simulationStep();

    require(std::isfinite(mesh.getNodeState(0)), "scaled simulation state must be finite");
    require(mesh.getNodeBridgesCount(nodeCount / 2) == 2,
            "scaled simulation must preserve interior topology");
}

void test_worker_pool_expands_between_steps() {
    using namespace AdaptiveMesh;

    SpatialAdaptiveMesh mesh(4);
    mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
    mesh.simulationStep();

    mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
    mesh.addNode(2, {2.0, 0.0, 0.0}, 0.0);
    mesh.connectNodes(0, 1);
    mesh.connectNodes(1, 2);
    mesh.injectExternalShock(0, 2.0);
    mesh.simulationStep();

    require(std::isfinite(mesh.getNodeState(0)),
            "expanded worker pool must complete a simulation step");
}

void test_stability_and_shock() {
    using namespace AdaptiveMesh;

    SpatialAdaptiveMesh mesh;
    mesh.addNode(0, {0.0, 0.0, 0.0}, 0.0);
    mesh.addNode(1, {1.0, 0.0, 0.0}, 0.0);
    mesh.addNode(2, {2.0, 0.0, 0.0}, 0.0);
    mesh.connectNodes(0, 1);
    mesh.connectNodes(1, 2);

    require(mesh.getNodeBridgesCount(0) == 1, "node 0 must have one bridge");
    require(mesh.getNodeBridgesCount(1) == 2, "node 1 must have two bridges");
    assert(mesh.getNodeBridgesCount(2) == 1);

    mesh.injectExternalShock(0, 2.0);
    const double stateAfterShock = mesh.getNodeState(0);
    require(std::abs(stateAfterShock - 2.0) < 1e-12, "shock must update node state");
    require(mesh.getNodeHealth(0) < 1.0, "shock must reduce node health");
    assert(mesh.getNodeHealth(0) < 1.0);

    mesh.simulationStep();

    const double stateAfterStep = mesh.getNodeState(0);
    const double healthAfterStep = mesh.getNodeHealth(0);
    require(std::isfinite(stateAfterStep), "simulation state must be finite");
    require(std::isfinite(healthAfterStep), "simulation health must be finite");
    require(stateAfterStep < stateAfterShock, "diffusion must reduce shocked node state");
    assert(stateAfterStep < stateAfterShock);

    const std::filesystem::path resultsDirectory =
        std::filesystem::current_path() / "results";
    std::filesystem::create_directories(resultsDirectory);
    std::ofstream trace(resultsDirectory / "runtime_trace.jsonl", std::ios::out | std::ios::trunc);
    require(trace.is_open(), "unable to open telemetry trace");

    trace << "{\"event\":\"smoke_test\",\"nodes\":3,\"shock\":2.0,"
             "\"state_after_step\":" << stateAfterStep
          << ",\"health_after_step\":" << healthAfterStep << "}\n";
    require(trace.good(), "unable to write telemetry trace");
    trace.close();
    require(!trace.fail(), "unable to close telemetry trace");
}

} // namespace

int main() {
    try {
        test_topology_contract();
        test_bulk_connection_contract();
        test_numeric_input_contract();
        test_interaction_observation_contract();
        test_bridge_confidence_contract();
        test_adaptive_bridge_policy_contract();
        test_bridge_persistence_contract();
        test_bridge_transition_authorization_contract();
        test_worker_configuration_is_deterministic();
        test_legacy_simulation_step_wrapper();
        test_post_commit_health_remains_finite_for_large_drift();
        test_simulation_buffer_shape_invalidation();
        test_bounded_worker_scale_smoke(100);
        test_bounded_worker_scale_smoke(1000);
        test_worker_pool_expands_between_steps();
        test_stability_and_shock();
        std::cout << "Adaptive Mesh smoke test passed." << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Adaptive Mesh smoke test failed: " << error.what() << std::endl;
        return 1;
    }
}
