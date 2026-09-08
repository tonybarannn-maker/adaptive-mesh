#include "shadow_adaptive_controller.hpp"

namespace AdaptiveMesh::experiment {

namespace {

[[nodiscard]]
ShadowTransitionDisposition toShadowDisposition(
    BridgeTransitionIntent intent) noexcept
{
    switch (intent) {
    case BridgeTransitionIntent::CONSTRAIN:
        return ShadowTransitionDisposition::constrain;
    case BridgeTransitionIntent::SUPPORT:
        return ShadowTransitionDisposition::support;
    case BridgeTransitionIntent::PRESERVE:
    default:
        return ShadowTransitionDisposition::preserve;
    }
}

} // namespace

ShadowAdaptiveController::ShadowAdaptiveController(
    double activationThreshold,
    double releaseThreshold,
    std::size_t activationSamples,
    std::size_t releaseSamples)
    : persistence_(
          activationThreshold,
          releaseThreshold,
          activationSamples,
          releaseSamples)
{
}

ShadowAdaptiveDecision ShadowAdaptiveController::evaluate(
    const ShadowAdaptiveInput& input) noexcept
{
    const BridgePolicyEvidence evidence =
        bridgePolicy_.evaluate(input.observation, input.confidence);

    const PersistentBridgeRecommendation recommendation =
        persistence_.observe(evidence);

    const BridgeTransitionIntent intent =
        authorizationPolicy_.evaluate(
            recommendation,
            input.permissions);

    return ShadowAdaptiveDecision{
        input.sourceNodeId,
        input.targetNodeId,
        toShadowDisposition(intent),
        input.observation.compatibility(),
        input.confidence.value(),
        evidence.value()
    };
}

void ShadowAdaptiveController::reset() noexcept
{
    persistence_.reset();
}

} // namespace AdaptiveMesh::experiment
