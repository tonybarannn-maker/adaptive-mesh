#pragma once

#include "bridge_confidence.hpp"
#include "interaction_observation.hpp"

namespace AdaptiveMesh {

class AdaptiveBridgePolicy;

class BridgePolicyEvidence {
public:
    [[nodiscard]] double value() const noexcept {
        return value_;
    }

private:
    explicit BridgePolicyEvidence(double value) noexcept
        : value_(value)
    {
    }

    friend class AdaptiveBridgePolicy;

    double value_;
};

class AdaptiveBridgePolicy {
public:
    [[nodiscard]] BridgePolicyEvidence evaluate(
        const InteractionObservation& observation,
        const BridgeConfidence& confidence) const noexcept
    {
        const double signedObservation =
            (2.0 * observation.compatibility()) - 1.0;
        return BridgePolicyEvidence(
            signedObservation * confidence.value());
    }
};

} // namespace AdaptiveMesh
