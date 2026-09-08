#include "k9_interchange.hpp"

#include "adaptive_bridge_policy.hpp"
#include "bridge_confidence.hpp"
#include "bridge_persistence.hpp"
#include "bridge_transition_authorization.hpp"
#include "interaction_observation.hpp"

#include <type_traits>

namespace {

using ExternalObservation =
    soam::experiment::kiko::
        ValidatedExternalExperimentObservation;

static_assert(
    !std::is_default_constructible_v<ExternalObservation>);

static_assert(
    !std::is_convertible_v<ExternalObservation, bool>);

static_assert(
    !std::is_convertible_v<ExternalObservation, double>);

static_assert(
    !std::is_constructible_v<
        AdaptiveMesh::InteractionObservation,
        ExternalObservation>);

static_assert(
    !std::is_constructible_v<
        AdaptiveMesh::BridgeConfidence,
        ExternalObservation>);

static_assert(
    !std::is_constructible_v<
        AdaptiveMesh::BridgePolicyEvidence,
        ExternalObservation>);

static_assert(
    !std::is_constructible_v<
        AdaptiveMesh::BridgeTransitionPermissions,
        ExternalObservation>);

static_assert(
    !std::is_convertible_v<
        ExternalObservation,
        AdaptiveMesh::PersistentBridgeRecommendation>);

static_assert(
    !std::is_convertible_v<
        ExternalObservation,
        AdaptiveMesh::BridgeTransitionIntent>);

static_assert(
    !std::is_constructible_v<
        ExternalObservation,
        AdaptiveMesh::InteractionObservation>);

static_assert(
    !std::is_constructible_v<
        ExternalObservation,
        AdaptiveMesh::BridgeConfidence>);

static_assert(
    !std::is_constructible_v<
        ExternalObservation,
        AdaptiveMesh::BridgePolicyEvidence>);

static_assert(
    !std::is_constructible_v<
        ExternalObservation,
        AdaptiveMesh::BridgeTransitionPermissions>);

} // namespace

int main()
{
    return 0;
}
