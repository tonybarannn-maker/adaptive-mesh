#include "detail/spatial_adaptive_mesh_impl.hpp"
#include "detail/production_authority_internal.hpp"
#include "production_authority_policy.hpp"
#if SOAM_PHASE_PROFILE_ENABLED
#include "detail/simulation_phase_profile.hpp"
#endif

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace AdaptiveMesh {

ProductionAuthorityDerivationResult
ProductionAuthorityDerivationPolicy::evaluate(
    const ProductionTransitionEligibilityDecision& eligibility,
    const ProductionAuthorityContext& authorityContext) const
{
    ProductionAuthorityDerivationResult result;

    if (eligibility.eligibility() !=
        ProductionTransitionEligibility::eligible_for_authority_consideration) {
        result.rejectionReason_ =
            ProductionAuthorityRejectionReason::eligibility_not_satisfied;
        return result;
    }

    if (eligibility.binding() !=
            detail::ProductionAuthorityDerivationAccess::binding(authorityContext) ||
        !detail::ProductionAuthorityDerivationAccess::liveBindingCurrent(
            authorityContext)) {
        result.rejectionReason_ =
            ProductionAuthorityRejectionReason::binding_mismatch;
        return result;
    }

    if (!detail::ProductionAuthorityDerivationAccess::stateCurrent(
            authorityContext)) {
        result.rejectionReason_ =
            ProductionAuthorityRejectionReason::stale_state;
        return result;
    }

    if (!detail::ProductionAuthorityDerivationAccess::transitionClassCurrent(
            authorityContext)) {
        result.rejectionReason_ =
            ProductionAuthorityRejectionReason::transition_class_mismatch;
        return result;
    }

    if (!detail::ProductionAuthorityDerivationAccess::freshnessSatisfied(
            authorityContext)) {
        result.rejectionReason_ =
            ProductionAuthorityRejectionReason::freshness_invalid;
        return result;
    }

    if (!detail::ProductionAuthorityDerivationAccess::revalidationSatisfied(
            authorityContext)) {
        result.rejectionReason_ =
            ProductionAuthorityRejectionReason::revalidation_failed;
        return result;
    }

    if (!detail::ProductionAuthorityDerivationAccess::authorityPolicySatisfied(
            authorityContext)) {
        result.rejectionReason_ =
            ProductionAuthorityRejectionReason::authority_policy_denied;
        return result;
    }

    result.capability_.emplace(
        detail::ProductionAuthorityDerivationAccess::capability(
            detail::ProductionAuthorityDerivationAccess::nextCapabilityId(),
            detail::ProductionAuthorityDerivationAccess::domain(
                authorityContext),
            eligibility.binding(),
            detail::ProductionAuthorityDerivationAccess::authoritativeEpoch(
                authorityContext)));
    result.decision_ = ProductionAuthorityDecision::capability_issued;
    result.rejectionReason_ = ProductionAuthorityRejectionReason::none;
    return result;
}

void requireFinite(double value, const char* name) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument(std::string{name} + " must be finite");
    }
}

void Vector3D::validate() const {
    requireFinite(x, "x");
    requireFinite(y, "y");
    requireFinite(z, "z");
}

double Vector3D::distanceTo(const Vector3D& other) const {
    validate();
    other.validate();
    const double dx = x - other.x;
    const double dy = y - other.y;
    const double dz = z - other.z;
    requireFinite(dx, "distance dx");
    requireFinite(dy, "distance dy");
    requireFinite(dz, "distance dz");
    const double result = std::hypot(std::hypot(dx, dy), dz);
    requireFinite(result, "distance");
    return result;
}

double Vector3D::orientationFactorTo(const Vector3D& target) const {
    const double distance = distanceTo(target);
    if (distance < 1e-6) return 1.0;
    return 0.5 * (1.0 + (target.z - z) / distance);
}

void IdentityInvariant::validate() const {
    requireFinite(baseline, "baseline");
    requireFinite(maxEpsilon, "maxEpsilon");
    if (maxEpsilon <= 0.0) {
        throw std::invalid_argument("maxEpsilon must be positive");
    }
}

bool IdentityInvariant::isWithinSafetyBound(double value) const {
    validate();
    requireFinite(value, "state");
    return std::abs(value - baseline) <= maxEpsilon;
}

SignalCategory MetaEvaluator::evaluate(
    double candidateState,
    double health,
    const IdentityInvariant& omega) const {
    requireFinite(candidateState, "candidateState");
    requireFinite(health, "healthIndex");
    if (!omega.isWithinSafetyBound(candidateState)) {
        return SignalCategory::DESTRUCTIVE_DRIFT;
    }
    if (health > 0.7 && std::abs(candidateState - omega.baseline) > 1.2) {
        return SignalCategory::CREATIVE_SIGNAL;
    }
    return SignalCategory::NOISE;
}

void SpatialBridge::updateBridgeState(SignalCategory category) {
    switch (category) {
    case SignalCategory::NOISE:
        capacity = std::max(0.01, capacity * 0.85);
        status = BridgeStatus::DAMPING;
        break;
    case SignalCategory::CREATIVE_SIGNAL:
        capacity = std::min(1.0, capacity + 0.15);
        status = capacity >= 0.9 ? BridgeStatus::NORMAL : BridgeStatus::RECOVERY;
        break;
    case SignalCategory::DESTRUCTIVE_DRIFT:
        capacity = 0.0;
        status = BridgeStatus::ISOLATED;
        break;
    }
}

double SpatialBridge::getEffectiveCoupling() const {
    requireFinite(distance, "bridge distance");
    requireFinite(orientationWeight, "bridge orientationWeight");
    requireFinite(capacity, "bridge capacity");
    if (distance < 0.0) throw std::invalid_argument("bridge distance must be non-negative");
    if (orientationWeight < 0.0 || orientationWeight > 1.0) {
        throw std::invalid_argument("bridge orientationWeight must be in [0, 1]");
    }
    if (capacity < 0.0 || capacity > 1.0) {
        throw std::invalid_argument("bridge capacity must be in [0, 1]");
    }
    const double result = capacity * (1.0 / (1.0 + 0.1 * distance)) * orientationWeight;
    requireFinite(result, "effective coupling");
    return result;
}

double SpatialBridge::getEffectiveTransmission() const {
    return getEffectiveCoupling();
}

AutopoieticNode::AutopoieticNode(
    std::size_t nodeId, Vector3D pos, double initialBaseline)
    : id(nodeId), position(pos), state(initialBaseline) {
    position.validate();
    requireFinite(initialBaseline, "baseline");
    invariant.baseline = initialBaseline;
}

AutopoieticNode::AutopoieticNode(const AutopoieticNode& other)
    : id(other.id), position(other.position), state(other.state.load()),
      healthIndex(other.healthIndex.load()), invariant(other.invariant),
      metaEvaluator(other.metaEvaluator), bridges(other.bridges) {}

void AutopoieticNode::updateHealth() {
    invariant.validate();
    const double current = state.load();
    requireFinite(current, "state");
    const double drift = std::abs(current - invariant.baseline);
    healthIndex.store(std::max(
        0.0, 1.0 - (drift / invariant.maxEpsilon)));
}

double AutopoieticNode::applyLocalReflexFilter(double rawInput) const {
    requireFinite(rawInput, "rawInput");
    constexpr double maxAllowedReflexStep = 3.5;
    const double current = state.load();
    requireFinite(current, "state");
    const double delta = rawInput - current;
    if (std::abs(delta) > maxAllowedReflexStep) {
        return current + (delta > 0.0
            ? maxAllowedReflexStep : -maxAllowedReflexStep);
    }
    return rawInput;
}

SpatialAdaptiveMesh::SpatialAdaptiveMesh(std::size_t maxWorkers)
    : impl_(std::make_unique<Impl>(maxWorkers)) {}

SpatialAdaptiveMesh::~SpatialAdaptiveMesh() = default;

void SpatialAdaptiveMesh::addNode(std::size_t id, Vector3D pos, double baseline) {
    impl_->addNode(id, pos, baseline);
}
void SpatialAdaptiveMesh::connectNodes(int a, int b) { impl_->connectNodes(a, b); }
void SpatialAdaptiveMesh::connectNodePairs(const std::vector<std::pair<int, int>>& pairs) {
    impl_->connectNodePairs(pairs);
}
void SpatialAdaptiveMesh::enforceStabilityCondition() noexcept { impl_->enforceStabilityCondition(); }
void SpatialAdaptiveMesh::pruneIsolatedBridges(double threshold) { impl_->pruneIsolatedBridges(threshold); }
void SpatialAdaptiveMesh::autoConnectNearbyNodes(double radius) { impl_->autoConnectNearbyNodes(radius); }
void SpatialAdaptiveMesh::injectExternalShock(int id, double magnitude) { impl_->injectExternalShock(id, magnitude); }
void SpatialAdaptiveMesh::simulationStep() { impl_->simulationStep(); }
void SpatialAdaptiveMesh::simulationStepAsync() { impl_->simulationStepAsync(); }
ProductionTransitionCommitResult SpatialAdaptiveMesh::commitProductionTransition(
    ProductionExecutionCapability&& capability) {
    return impl_->commitProductionTransition(std::move(capability));
}
double SpatialAdaptiveMesh::getNodeState(std::size_t id) const { return impl_->getNodeState(id); }
double SpatialAdaptiveMesh::getNodeHealth(std::size_t id) const { return impl_->getNodeHealth(id); }
std::size_t SpatialAdaptiveMesh::getNodeBridgesCount(std::size_t id) const { return impl_->getNodeBridgesCount(id); }

#if SOAM_PHASE_PROFILE_ENABLED
std::optional<detail::SimulationPhaseProfile>
detail::SimulationPhaseProfileAccessor::current(
    const SpatialAdaptiveMesh& mesh)
{
    return mesh.impl_->currentSimulationPhaseProfile();
}
#endif

} // namespace AdaptiveMesh