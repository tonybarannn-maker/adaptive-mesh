#include "shadow_adaptive_controller.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

using AdaptiveMesh::AdaptiveBridgePolicy;
using AdaptiveMesh::BridgeConfidence;
using AdaptiveMesh::BridgePersistence;
using AdaptiveMesh::BridgePolicyEvidence;
using AdaptiveMesh::BridgeTransitionPermissions;
using AdaptiveMesh::InteractionObservation;
using AdaptiveMesh::PersistentBridgeRecommendation;
using AdaptiveMesh::experiment::ShadowAdaptiveController;
using AdaptiveMesh::experiment::ShadowAdaptiveInput;
using AdaptiveMesh::experiment::ShadowTransitionDisposition;

[[noreturn]]
void fail(const std::string& message)
{
    std::cerr << "FAIL: " << message << '\n';
    std::exit(EXIT_FAILURE);
}

void require(bool condition, const std::string& message)
{
    if (!condition) {
        fail(message);
    }
}

enum class RawDisposition {
    preserve,
    constrain,
    support
};

[[nodiscard]]
RawDisposition classifyRaw(double evidence) noexcept
{
    if (evidence < 0.0) {
        return RawDisposition::constrain;
    }
    if (evidence > 0.0) {
        return RawDisposition::support;
    }
    return RawDisposition::preserve;
}

struct ScenarioMetrics {
    std::size_t samples = 0;
    std::size_t rawDispositionChanges = 0;
    std::size_t persistentRecommendationChanges = 0;
    std::size_t authorizedShadowChanges = 0;
    std::size_t blockedByConstraintCount = 0;
};

class ScenarioHarness {
public:
    ScenarioHarness(
        double activationThreshold,
        double releaseThreshold,
        std::size_t activationSamples,
        std::size_t releaseSamples)
        : metricPersistence_(
              activationThreshold,
              releaseThreshold,
              activationSamples,
              releaseSamples),
          controller_(
              activationThreshold,
              releaseThreshold,
              activationSamples,
              releaseSamples)
    {
    }

    [[nodiscard]]
    ShadowTransitionDisposition observe(
        double observation,
        double confidence,
        bool constrainAllowed = true,
        bool supportAllowed = true)
    {
        const InteractionObservation typedObservation(observation);
        const BridgeConfidence typedConfidence(confidence);
        const BridgeTransitionPermissions permissions(
            constrainAllowed,
            supportAllowed);

        const BridgePolicyEvidence evidence =
            policy_.evaluate(typedObservation, typedConfidence);

        const RawDisposition raw =
            classifyRaw(evidence.value());

        const PersistentBridgeRecommendation persistent =
            metricPersistence_.observe(evidence);

        const ShadowAdaptiveInput input{
            100,
            200,
            typedObservation,
            typedConfidence,
            permissions
        };

        const auto decision = controller_.evaluate(input);

        ++metrics_.samples;

        if (previousRaw_.has_value() &&
            *previousRaw_ != raw) {
            ++metrics_.rawDispositionChanges;
        }

        if (previousPersistent_.has_value() &&
            *previousPersistent_ != persistent) {
            ++metrics_.persistentRecommendationChanges;
        }

        if (previousShadow_.has_value() &&
            *previousShadow_ != decision.disposition) {
            ++metrics_.authorizedShadowChanges;
        }

        if ((persistent ==
                 PersistentBridgeRecommendation::CONSTRAIN &&
             !constrainAllowed) ||
            (persistent ==
                 PersistentBridgeRecommendation::SUPPORT &&
             !supportAllowed)) {
            ++metrics_.blockedByConstraintCount;
        }

        previousRaw_ = raw;
        previousPersistent_ = persistent;
        previousShadow_ = decision.disposition;

        return decision.disposition;
    }

    [[nodiscard]]
    const ScenarioMetrics& metrics() const noexcept
    {
        return metrics_;
    }

private:
    AdaptiveBridgePolicy policy_;
    BridgePersistence metricPersistence_;
    ShadowAdaptiveController controller_;

    ScenarioMetrics metrics_;

    std::optional<RawDisposition> previousRaw_;
    std::optional<PersistentBridgeRecommendation> previousPersistent_;
    std::optional<ShadowTransitionDisposition> previousShadow_;
};

void scenarioS01StableEvidence()
{
    ScenarioHarness harness(0.5, 0.2, 2, 2);

    for (int i = 0; i < 8; ++i) {
        require(
            harness.observe(0.5, 1.0) ==
                ShadowTransitionDisposition::preserve,
            "S01 stable neutral evidence must preserve");
    }

    require(
        harness.metrics().persistentRecommendationChanges == 0,
        "S01 unnecessary persistent transitions must be zero");

    std::cout
        << "S01 stable_evidence PASS"
        << " raw_changes="
        << harness.metrics().rawDispositionChanges
        << " persistent_changes="
        << harness.metrics().persistentRecommendationChanges
        << '\n';
}

void scenarioS02PersistentDegradation()
{
    ScenarioHarness allowed(0.5, 0.2, 2, 2);

    require(
        allowed.observe(0.0, 1.0) ==
            ShadowTransitionDisposition::preserve,
        "S02 activation delay must preserve first sample");

    require(
        allowed.observe(0.0, 1.0) ==
            ShadowTransitionDisposition::constrain,
        "S02 persistent degradation must activate constrain");

    ScenarioHarness denied(0.5, 0.2, 2, 2);

    static_cast<void>(
        denied.observe(0.0, 1.0, false, true));

    require(
        denied.observe(0.0, 1.0, false, true) ==
            ShadowTransitionDisposition::preserve,
        "S02 denied constrain must produce shadow preserve");

    require(
        denied.metrics().blockedByConstraintCount >= 1,
        "S02 blocked constraint must be measured");

    std::cout
        << "S02 persistent_degradation PASS"
        << " blocked="
        << denied.metrics().blockedByConstraintCount
        << '\n';
}

void scenarioS03RecoveryThroughPreserve()
{
    ScenarioHarness harness(0.5, 0.2, 2, 2);

    static_cast<void>(harness.observe(0.0, 1.0));

    require(
        harness.observe(0.0, 1.0) ==
            ShadowTransitionDisposition::constrain,
        "S03 constrain must first activate");

    require(
        harness.observe(0.5, 1.0) ==
            ShadowTransitionDisposition::constrain,
        "S03 first release sample must retain constrain");

    require(
        harness.observe(0.5, 1.0) ==
            ShadowTransitionDisposition::preserve,
        "S03 recovery must pass through preserve");

    std::cout
        << "S03 recovery_through_preserve PASS\n";
}

void scenarioS04ThresholdNoise()
{
    ScenarioHarness harness(0.5, 0.2, 2, 2);

    const std::vector<double> observations{
        0.20,
        0.80,
        0.20,
        0.80,
        0.20,
        0.80,
        0.20,
        0.80
    };

    for (const double observation : observations) {
        static_cast<void>(
            harness.observe(observation, 1.0));
    }

    require(
        harness.metrics().rawDispositionChanges > 0,
        "S04 raw classification must exhibit changes");

    require(
        harness.metrics().persistentRecommendationChanges == 0,
        "S04 alternating threshold noise must not activate persistence");

    std::cout
        << "S04 threshold_noise PASS"
        << " raw_changes="
        << harness.metrics().rawDispositionChanges
        << " persistent_changes="
        << harness.metrics().persistentRecommendationChanges
        << '\n';
}

void scenarioS05AlternatingEvidence()
{
    ScenarioHarness harness(0.5, 0.2, 3, 2);

    for (int i = 0; i < 12; ++i) {
        const double observation =
            (i % 2 == 0) ? 0.0 : 1.0;

        require(
            harness.observe(observation, 1.0) ==
                ShadowTransitionDisposition::preserve,
            "S05 alternating evidence must reset activation progress");
    }

    require(
        harness.metrics().persistentRecommendationChanges == 0,
        "S05 alternating evidence must remain persistent preserve");

    std::cout
        << "S05 alternating_evidence PASS"
        << " raw_changes="
        << harness.metrics().rawDispositionChanges
        << '\n';
}

void scenarioS06LocalizedFixtureChange()
{
    ScenarioHarness fixtureA(0.5, 0.2, 2, 2);
    ScenarioHarness fixtureB(0.5, 0.2, 2, 2);

    static_cast<void>(
        fixtureA.observe(0.0, 1.0));
    static_cast<void>(
        fixtureB.observe(0.5, 1.0));

    const auto decisionA =
        fixtureA.observe(0.0, 1.0);

    const auto decisionB =
        fixtureB.observe(0.5, 1.0);

    require(
        decisionA ==
            ShadowTransitionDisposition::constrain,
        "S06 changed fixture input must affect only its controller");

    require(
        decisionB ==
            ShadowTransitionDisposition::preserve,
        "S06 unchanged fixture input must remain preserve");

    std::cout
        << "S06 localized_fixture_change PASS\n";
}

} // namespace

int main()
{
    scenarioS01StableEvidence();
    scenarioS02PersistentDegradation();
    scenarioS03RecoveryThroughPreserve();
    scenarioS04ThresholdNoise();
    scenarioS05AlternatingEvidence();
    scenarioS06LocalizedFixtureChange();

    std::cout << "K10 runtime isolation: structural PASS\n";
    std::cout << "K10 shadow scenarios: PASS\n";

    return EXIT_SUCCESS;
}
