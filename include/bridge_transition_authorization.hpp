#pragma once

#include "bridge_persistence.hpp"

namespace AdaptiveMesh {

class BridgeTransitionPermissions {
public:
    explicit BridgeTransitionPermissions(
        bool constrainAllowed,
        bool supportAllowed) noexcept
        : constrainAllowed_(constrainAllowed),
          supportAllowed_(supportAllowed)
    {
    }

    [[nodiscard]] bool allowsConstrain() const noexcept {
        return constrainAllowed_;
    }

    [[nodiscard]] bool allowsSupport() const noexcept {
        return supportAllowed_;
    }

private:
    const bool constrainAllowed_;
    const bool supportAllowed_;
};

enum class BridgeTransitionIntent {
    PRESERVE,
    CONSTRAIN,
    SUPPORT
};

class BridgeTransitionAuthorizationPolicy {
public:
    [[nodiscard]] BridgeTransitionIntent evaluate(
        PersistentBridgeRecommendation recommendation,
        const BridgeTransitionPermissions& permissions) const noexcept
    {
        switch (recommendation) {
        case PersistentBridgeRecommendation::CONSTRAIN:
            return permissions.allowsConstrain()
                ? BridgeTransitionIntent::CONSTRAIN
                : BridgeTransitionIntent::PRESERVE;
        case PersistentBridgeRecommendation::SUPPORT:
            return permissions.allowsSupport()
                ? BridgeTransitionIntent::SUPPORT
                : BridgeTransitionIntent::PRESERVE;
        case PersistentBridgeRecommendation::PRESERVE:
        default:
            return BridgeTransitionIntent::PRESERVE;
        }
    }
};

} // namespace AdaptiveMesh
