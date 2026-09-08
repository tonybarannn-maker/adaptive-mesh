#pragma once

#include "adaptive_bridge_policy.hpp"
#include "bridge_persistence.hpp"
#include "bridge_transition_authorization.hpp"

#include <cstddef>

namespace AdaptiveMesh::experiment {

enum class ShadowTransitionDisposition {
    preserve,
    constrain,
    support
};

struct ShadowAdaptiveInput {
    std::size_t sourceNodeId;
    std::size_t targetNodeId;

    InteractionObservation observation;
    BridgeConfidence confidence;
    BridgeTransitionPermissions permissions;
};

struct ShadowAdaptiveDecision {
    std::size_t sourceNodeId;
    std::size_t targetNodeId;

    ShadowTransitionDisposition disposition;

    double observationValue;
    double confidenceValue;
    double evidenceValue;
};

class ShadowAdaptiveController {
public:
    ShadowAdaptiveController(
        double activationThreshold,
        double releaseThreshold,
        std::size_t activationSamples,
        std::size_t releaseSamples);

    [[nodiscard]]
    ShadowAdaptiveDecision evaluate(
        const ShadowAdaptiveInput& input) noexcept;

    void reset() noexcept;

private:
    AdaptiveBridgePolicy bridgePolicy_;
    BridgePersistence persistence_;
    BridgeTransitionAuthorizationPolicy authorizationPolicy_;
};

} // namespace AdaptiveMesh::experiment
