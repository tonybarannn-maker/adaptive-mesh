#include "shadow_adaptive_controller.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace {

using AdaptiveMesh::AdaptiveBridgePolicy;
using AdaptiveMesh::BridgeConfidence;
using AdaptiveMesh::BridgeTransitionPermissions;
using AdaptiveMesh::InteractionObservation;
using AdaptiveMesh::experiment::ShadowAdaptiveController;
using AdaptiveMesh::experiment::ShadowAdaptiveDecision;
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

void requireEqual(
    ShadowTransitionDisposition actual,
    ShadowTransitionDisposition expected,
    const std::string& message)
{
    if (actual != expected) {
        fail(message);
    }
}

void requireExactDouble(
    double actual,
    double expected,
    const std::string& message)
{
    if (actual != expected) {
        fail(message);
    }
}

template <typename Fn>
void requireThrowsInvalidArgument(
    Fn&& fn,
    const std::string& message)
{
    try {
        std::forward<Fn>(fn)();
    } catch (const std::invalid_argument&) {
        return;
    } catch (...) {
        fail(message + " (wrong exception type)");
    }

    fail(message + " (no exception)");
}

ShadowAdaptiveInput makeInput(
    double observation,
    double confidence,
    bool constrainAllowed = true,
    bool supportAllowed = true,
    std::size_t sourceNodeId = 10,
    std::size_t targetNodeId = 20)
{
    return ShadowAdaptiveInput{
        sourceNodeId,
        targetNodeId,
        InteractionObservation(observation),
        BridgeConfidence(confidence),
        BridgeTransitionPermissions(
            constrainAllowed,
            supportAllowed)
    };
}

void testC01IdentifiersPreserved()
{
    ShadowAdaptiveController controller(0.5, 0.2, 2, 2);

    const auto decision =
        controller.evaluate(makeInput(
            0.5,
            1.0,
            true,
            true,
            123,
            456));

    require(
        decision.sourceNodeId == 123,
        "C01 sourceNodeId must be preserved");
    require(
        decision.targetNodeId == 456,
        "C01 targetNodeId must be preserved");
}

void testC02ObservationDiagnosticPreserved()
{
    ShadowAdaptiveController controller(0.5, 0.2, 2, 2);

    const auto decision =
        controller.evaluate(makeInput(0.75, 1.0));

    requireExactDouble(
        decision.observationValue,
        0.75,
        "C02 observation diagnostic must match typed input");
}

void testC03ConfidenceDiagnosticPreserved()
{
    ShadowAdaptiveController controller(0.5, 0.2, 2, 2);

    const auto decision =
        controller.evaluate(makeInput(0.75, 0.625));

    requireExactDouble(
        decision.confidenceValue,
        0.625,
        "C03 confidence diagnostic must match typed input");
}

void testC04EvidenceDiagnosticUsesActualPolicy()
{
    const InteractionObservation observation(0.25);
    const BridgeConfidence confidence(0.8);

    const double expected =
        AdaptiveBridgePolicy{}
            .evaluate(observation, confidence)
            .value();

    ShadowAdaptiveController controller(0.5, 0.2, 2, 2);

    const ShadowAdaptiveInput input{
        1,
        2,
        observation,
        confidence,
        BridgeTransitionPermissions(true, true)
    };

    const auto decision = controller.evaluate(input);

    requireExactDouble(
        decision.evidenceValue,
        expected,
        "C04 evidence diagnostic must come from AdaptiveBridgePolicy");
}

void testC05InitialPersistencePreserves()
{
    ShadowAdaptiveController controller(0.5, 0.2, 2, 2);

    const auto decision =
        controller.evaluate(makeInput(0.0, 1.0));

    requireEqual(
        decision.disposition,
        ShadowTransitionDisposition::preserve,
        "C05 first activating sample must remain preserve");
}

void testC06ConstrainActivation()
{
    ShadowAdaptiveController controller(0.5, 0.2, 2, 2);

    requireEqual(
        controller.evaluate(makeInput(0.0, 1.0)).disposition,
        ShadowTransitionDisposition::preserve,
        "C06 constrain activation sample 1 must preserve");

    requireEqual(
        controller.evaluate(makeInput(0.0, 1.0)).disposition,
        ShadowTransitionDisposition::constrain,
        "C06 constrain activation sample 2 must constrain");
}

void testC07SupportActivation()
{
    ShadowAdaptiveController controller(0.5, 0.2, 2, 2);

    requireEqual(
        controller.evaluate(makeInput(1.0, 1.0)).disposition,
        ShadowTransitionDisposition::preserve,
        "C07 support activation sample 1 must preserve");

    requireEqual(
        controller.evaluate(makeInput(1.0, 1.0)).disposition,
        ShadowTransitionDisposition::support,
        "C07 support activation sample 2 must support");
}

void testC08ConstrainPermissionDenied()
{
    ShadowAdaptiveController controller(0.5, 0.2, 2, 2);

    static_cast<void>(
        controller.evaluate(
            makeInput(0.0, 1.0, false, true)));

    const auto decision =
        controller.evaluate(
            makeInput(0.0, 1.0, false, true));

    requireEqual(
        decision.disposition,
        ShadowTransitionDisposition::preserve,
        "C08 denied constrain must fail closed to preserve");
}

void testC09SupportPermissionDenied()
{
    ShadowAdaptiveController controller(0.5, 0.2, 2, 2);

    static_cast<void>(
        controller.evaluate(
            makeInput(1.0, 1.0, true, false)));

    const auto decision =
        controller.evaluate(
            makeInput(1.0, 1.0, true, false));

    requireEqual(
        decision.disposition,
        ShadowTransitionDisposition::preserve,
        "C09 denied support must fail closed to preserve");
}

void testC10AllowedConstrain()
{
    ShadowAdaptiveController controller(0.5, 0.2, 2, 2);

    static_cast<void>(
        controller.evaluate(
            makeInput(0.0, 1.0, true, true)));

    const auto decision =
        controller.evaluate(
            makeInput(0.0, 1.0, true, true));

    requireEqual(
        decision.disposition,
        ShadowTransitionDisposition::constrain,
        "C10 allowed constrain must remain constrain");
}

void testC11AllowedSupport()
{
    ShadowAdaptiveController controller(0.5, 0.2, 2, 2);

    static_cast<void>(
        controller.evaluate(
            makeInput(1.0, 1.0, true, true)));

    const auto decision =
        controller.evaluate(
            makeInput(1.0, 1.0, true, true));

    requireEqual(
        decision.disposition,
        ShadowTransitionDisposition::support,
        "C11 allowed support must remain support");
}

void testC12Release()
{
    ShadowAdaptiveController controller(0.5, 0.2, 2, 2);

    static_cast<void>(controller.evaluate(makeInput(0.0, 1.0)));
    static_cast<void>(controller.evaluate(makeInput(0.0, 1.0)));

    requireEqual(
        controller.evaluate(makeInput(0.5, 1.0)).disposition,
        ShadowTransitionDisposition::constrain,
        "C12 release sample 1 must retain constrain");

    requireEqual(
        controller.evaluate(makeInput(0.5, 1.0)).disposition,
        ShadowTransitionDisposition::preserve,
        "C12 release sample 2 must return preserve");
}

void testC13Reset()
{
    ShadowAdaptiveController controller(0.5, 0.2, 2, 2);

    static_cast<void>(controller.evaluate(makeInput(1.0, 1.0)));
    static_cast<void>(controller.evaluate(makeInput(1.0, 1.0)));

    controller.reset();

    const auto decision =
        controller.evaluate(makeInput(1.0, 1.0));

    requireEqual(
        decision.disposition,
        ShadowTransitionDisposition::preserve,
        "C13 reset must restore initial persistence semantics");
}

void testC14IndependentControllerInstances()
{
    ShadowAdaptiveController left(0.5, 0.2, 2, 2);
    ShadowAdaptiveController right(0.5, 0.2, 2, 2);

    static_cast<void>(
        left.evaluate(makeInput(0.0, 1.0, true, true, 1, 2)));

    const auto leftDecision =
        left.evaluate(makeInput(0.0, 1.0, true, true, 1, 2));

    const auto rightDecision =
        right.evaluate(makeInput(0.0, 1.0, true, true, 3, 4));

    requireEqual(
        leftDecision.disposition,
        ShadowTransitionDisposition::constrain,
        "C14 left controller must activate independently");

    requireEqual(
        rightDecision.disposition,
        ShadowTransitionDisposition::preserve,
        "C14 right controller must retain independent state");
}

void testConstructorValidation()
{
    const double nan =
        std::numeric_limits<double>::quiet_NaN();
    const double inf =
        std::numeric_limits<double>::infinity();

    requireThrowsInvalidArgument(
        [nan] {
            ShadowAdaptiveController controller(nan, 0.2, 2, 2);
            static_cast<void>(controller);
        },
        "constructor must reject NaN activation threshold");

    requireThrowsInvalidArgument(
        [inf] {
            ShadowAdaptiveController controller(inf, 0.2, 2, 2);
            static_cast<void>(controller);
        },
        "constructor must reject infinite activation threshold");

    requireThrowsInvalidArgument(
        [nan] {
            ShadowAdaptiveController controller(0.5, nan, 2, 2);
            static_cast<void>(controller);
        },
        "constructor must reject NaN release threshold");

    requireThrowsInvalidArgument(
        [] {
            ShadowAdaptiveController controller(0.5, -0.1, 2, 2);
            static_cast<void>(controller);
        },
        "constructor must reject negative release threshold");

    requireThrowsInvalidArgument(
        [] {
            ShadowAdaptiveController controller(1.1, 0.2, 2, 2);
            static_cast<void>(controller);
        },
        "constructor must reject activation threshold above 1");

    requireThrowsInvalidArgument(
        [] {
            ShadowAdaptiveController controller(0.5, 0.5, 2, 2);
            static_cast<void>(controller);
        },
        "constructor must reject release threshold >= activation threshold");

    requireThrowsInvalidArgument(
        [] {
            ShadowAdaptiveController controller(0.5, 0.2, 1, 2);
            static_cast<void>(controller);
        },
        "constructor must reject activationSamples < 2");

    requireThrowsInvalidArgument(
        [] {
            ShadowAdaptiveController controller(0.5, 0.2, 2, 1);
            static_cast<void>(controller);
        },
        "constructor must reject releaseSamples < 2");
}

} // namespace

static_assert(
    noexcept(
        std::declval<ShadowAdaptiveController&>().evaluate(
            std::declval<const ShadowAdaptiveInput&>())),
    "C15 evaluate() must remain noexcept");

static_assert(
    noexcept(
        std::declval<ShadowAdaptiveController&>().reset()),
    "C16 reset() must remain noexcept");

int main()
{
    testC01IdentifiersPreserved();
    testC02ObservationDiagnosticPreserved();
    testC03ConfidenceDiagnosticPreserved();
    testC04EvidenceDiagnosticUsesActualPolicy();
    testC05InitialPersistencePreserves();
    testC06ConstrainActivation();
    testC07SupportActivation();
    testC08ConstrainPermissionDenied();
    testC09SupportPermissionDenied();
    testC10AllowedConstrain();
    testC11AllowedSupport();
    testC12Release();
    testC13Reset();
    testC14IndependentControllerInstances();
    testConstructorValidation();

    std::cout << "K10 component tests: PASS\n";
    return EXIT_SUCCESS;
}
