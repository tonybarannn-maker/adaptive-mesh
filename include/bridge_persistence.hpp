#pragma once

#include "adaptive_bridge_policy.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace AdaptiveMesh {

enum class PersistentBridgeRecommendation {
    PRESERVE,
    CONSTRAIN,
    SUPPORT
};

class BridgePersistence {
public:
    BridgePersistence(
        double activationThreshold,
        double releaseThreshold,
        std::size_t activationSamples,
        std::size_t releaseSamples)
        : activationThreshold_(activationThreshold),
          releaseThreshold_(releaseThreshold),
          activationSamples_(activationSamples),
          releaseSamples_(releaseSamples)
    {
        if (!std::isfinite(activationThreshold_) ||
            !std::isfinite(releaseThreshold_) ||
            releaseThreshold_ < 0.0 ||
            activationThreshold_ > 1.0 ||
            releaseThreshold_ >= activationThreshold_) {
            throw std::invalid_argument(
                "bridge persistence thresholds must satisfy 0 <= R < A <= 1");
        }
        if (activationSamples_ < 2 || releaseSamples_ < 2) {
            throw std::invalid_argument(
                "bridge persistence sample requirements must be at least 2");
        }
    }

    [[nodiscard]] PersistentBridgeRecommendation observe(
        const BridgePolicyEvidence& evidence) noexcept
    {
        const double value = evidence.value();

        if (recommendation_ == PersistentBridgeRecommendation::PRESERVE) {
            if (std::abs(value) < activationThreshold_) {
                clearPending();
                return recommendation_;
            }

            const PendingDirection direction =
                value < 0.0 ? PendingDirection::CONSTRAIN
                            : PendingDirection::SUPPORT;
            advancePending(direction, activationSamples_);
            if (consecutiveSamples_ == activationSamples_) {
                recommendation_ = direction == PendingDirection::CONSTRAIN
                    ? PersistentBridgeRecommendation::CONSTRAIN
                    : PersistentBridgeRecommendation::SUPPORT;
                clearPending();
            }
            return recommendation_;
        }

        const bool releasePending = recommendation_ ==
            PersistentBridgeRecommendation::SUPPORT
            ? value <= releaseThreshold_
            : value >= -releaseThreshold_;
        if (!releasePending) {
            clearPending();
            return recommendation_;
        }

        advancePending(PendingDirection::PRESERVE, releaseSamples_);
        if (consecutiveSamples_ == releaseSamples_) {
            recommendation_ = PersistentBridgeRecommendation::PRESERVE;
            clearPending();
        }
        return recommendation_;
    }

    void reset() noexcept
    {
        recommendation_ = PersistentBridgeRecommendation::PRESERVE;
        clearPending();
    }

private:
    enum class PendingDirection {
        NONE,
        CONSTRAIN,
        SUPPORT,
        PRESERVE
    };

    void clearPending() noexcept
    {
        pendingDirection_ = PendingDirection::NONE;
        consecutiveSamples_ = 0;
    }

    void advancePending(
        PendingDirection direction,
        std::size_t limit) noexcept
    {
        if (pendingDirection_ != direction) {
            pendingDirection_ = direction;
            consecutiveSamples_ = 1;
        } else if (consecutiveSamples_ < limit) {
            ++consecutiveSamples_;
        }
    }

    double activationThreshold_;
    double releaseThreshold_;
    std::size_t activationSamples_;
    std::size_t releaseSamples_;
    std::size_t consecutiveSamples_ = 0;
    PersistentBridgeRecommendation recommendation_ =
        PersistentBridgeRecommendation::PRESERVE;
    PendingDirection pendingDirection_ = PendingDirection::NONE;
};

} // namespace AdaptiveMesh
