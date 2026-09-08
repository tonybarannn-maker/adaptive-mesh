#include "k9_interchange.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>

namespace {

using soam::experiment::kiko::AdapterError;
using soam::experiment::kiko::ExternalExperimentalOutcome;
using soam::experiment::kiko::ExternalExperimentKind;
using soam::experiment::kiko::ValidatedExternalExperimentObservation;
using soam::experiment::kiko::ValidationResult;
using soam::experiment::kiko::validate;

[[noreturn]]
void fail(std::string_view message)
{
    std::cerr << "K9 interchange test failure: "
              << message << '\n';
    std::exit(EXIT_FAILURE);
}

void expect(bool condition, std::string_view message)
{
    if (!condition) {
        fail(message);
    }
}

void expectError(
    std::string_view encodedRecord,
    ExternalExperimentKind expectedKind,
    AdapterError expectedError,
    std::string_view message)
{
    const ValidationResult result =
        validate(encodedRecord, expectedKind);

    expect(
        std::holds_alternative<AdapterError>(result),
        message);

    expect(
        std::get<AdapterError>(result) == expectedError,
        message);
}

const ValidatedExternalExperimentObservation&
expectValid(
    const ValidationResult& result,
    std::string_view message)
{
    expect(
        std::holds_alternative<
            ValidatedExternalExperimentObservation>(result),
        message);

    return std::get<
        ValidatedExternalExperimentObservation>(result);
}

void testValidProductionPassed()
{
    const auto result = validate(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\taudit-0001\n",
        ExternalExperimentKind::production_gate);

    const auto& observation =
        expectValid(result, "production passed must validate");

    expect(observation.schemaVersion() == 1U, "schema must be 1");
    expect(
        observation.experimentKind() ==
            ExternalExperimentKind::production_gate,
        "production kind must be preserved");
    expect(
        observation.outcome() ==
            ExternalExperimentalOutcome::passed,
        "passed outcome must be preserved");
    expect(
        observation.correlationId() == "audit-0001",
        "correlation must be preserved");
}

void testValidOtherVocabulary()
{
    {
        const auto result = validate(
            "KIKO-EXPERIMENT\t1\tproduction_gate\tblocked\tA\n",
            ExternalExperimentKind::production_gate);

        const auto& observation =
            expectValid(result, "production blocked must validate");

        expect(
            observation.outcome() ==
                ExternalExperimentalOutcome::blocked,
            "blocked outcome must be preserved");
        expect(
            observation.correlationId() == "A",
            "one-byte correlation must validate");
    }

    {
        const auto result = validate(
            "KIKO-EXPERIMENT\t1\tk3_authorization_disabled\tpassed\tAZaz09._:-\n",
            ExternalExperimentKind::k3_authorization_disabled);

        const auto& observation =
            expectValid(result, "ablation passed must validate");

        expect(
            observation.experimentKind() ==
                ExternalExperimentKind::k3_authorization_disabled,
            "ablation kind must be preserved");
        expect(
            observation.correlationId() == "AZaz09._:-",
            "allowed correlation vocabulary must be preserved");
    }

    {
        const auto result = validate(
            "KIKO-EXPERIMENT\t1\tk3_authorization_disabled\tblocked\tcorr-2\n",
            ExternalExperimentKind::k3_authorization_disabled);

        const auto& observation =
            expectValid(result, "ablation blocked must validate");

        expect(
            observation.outcome() ==
                ExternalExperimentalOutcome::blocked,
            "ablation blocked outcome must be preserved");
    }
}

void testCorrelationBoundaries()
{
    const std::string correlation128(128, 'a');

    const std::string validRecord =
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\t" +
        correlation128 + "\n";

    const auto validResult = validate(
        validRecord,
        ExternalExperimentKind::production_gate);

    const auto& observation =
        expectValid(validResult, "128-byte correlation must validate");

    expect(
        observation.correlationId() == correlation128,
        "128-byte correlation must be preserved exactly");

    const std::string correlation129(129, 'a');

    const std::string invalidRecord =
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\t" +
        correlation129 + "\n";

    expectError(
        invalidRecord,
        ExternalExperimentKind::production_gate,
        AdapterError::invalid_correlation,
        "129-byte correlation must fail");
}

void testDeterminism()
{
    constexpr std::string_view record =
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\tCase.Sensitive-ID:9\n";

    const auto first =
        validate(record, ExternalExperimentKind::production_gate);
    const auto second =
        validate(record, ExternalExperimentKind::production_gate);

    const auto& a =
        expectValid(first, "first deterministic validation must pass");
    const auto& b =
        expectValid(second, "second deterministic validation must pass");

    expect(
        a.schemaVersion() == b.schemaVersion(),
        "schema must be deterministic");
    expect(
        a.experimentKind() == b.experimentKind(),
        "kind must be deterministic");
    expect(
        a.outcome() == b.outcome(),
        "outcome must be deterministic");
    expect(
        a.correlationId() == b.correlationId(),
        "correlation must be deterministic");

    expect(
        a.correlationId() == "Case.Sensitive-ID:9",
        "correlation case must not be normalized");
}

void testMalformedFraming()
{
    const auto kind = ExternalExperimentKind::production_gate;

    expectError(
        "",
        kind,
        AdapterError::malformed_record,
        "empty record must be malformed");

    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed",
        kind,
        AdapterError::malformed_record,
        "missing field must be malformed");

    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\tcorr\textra\n",
        kind,
        AdapterError::malformed_record,
        "extra field must be malformed");

    expectError(
        "WRONG\t1\tproduction_gate\tpassed\tcorr\n",
        kind,
        AdapterError::malformed_record,
        "wrong marker must be malformed");

    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\tcorr",
        kind,
        AdapterError::malformed_record,
        "missing terminal LF must be malformed");

    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\tcorr\r\n",
        kind,
        AdapterError::malformed_record,
        "CRLF must be malformed");

    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\tcorr\n\n",
        kind,
        AdapterError::malformed_record,
        "double LF must be malformed");

    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\n\tpassed\tcorr\n",
        kind,
        AdapterError::malformed_record,
        "embedded LF must be malformed");

    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\tcorr\nX\n",
        kind,
        AdapterError::malformed_record,
        "bytes after first LF must be malformed");
}

void testSchemaErrors()
{
    const auto kind = ExternalExperimentKind::production_gate;

    expectError(
        "KIKO-EXPERIMENT\t0\tproduction_gate\tpassed\tcorr\n",
        kind,
        AdapterError::unsupported_schema,
        "schema 0 must be unsupported");

    expectError(
        "KIKO-EXPERIMENT\t2\tproduction_gate\tpassed\tcorr\n",
        kind,
        AdapterError::unsupported_schema,
        "schema 2 must be unsupported");

    expectError(
        "KIKO-EXPERIMENT\tx\tproduction_gate\tpassed\tcorr\n",
        kind,
        AdapterError::unsupported_schema,
        "nonnumeric schema must be unsupported");
}

void testExperimentErrors()
{
    const auto kind = ExternalExperimentKind::production_gate;

    expectError(
        "KIKO-EXPERIMENT\t1\tunknown\tpassed\tcorr\n",
        kind,
        AdapterError::unsupported_experiment,
        "unknown experiment must fail");

    expectError(
        "KIKO-EXPERIMENT\t1\tProduction_gate\tpassed\tcorr\n",
        kind,
        AdapterError::unsupported_experiment,
        "case-modified experiment must fail");
}

void testOutcomeErrors()
{
    const auto kind = ExternalExperimentKind::production_gate;

    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tunknown\tcorr\n",
        kind,
        AdapterError::invalid_outcome,
        "unknown outcome must fail");

    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tPassed\tcorr\n",
        kind,
        AdapterError::invalid_outcome,
        "case-modified outcome must fail");
}

void testCorrelationErrors()
{
    const auto kind = ExternalExperimentKind::production_gate;

    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\t\n",
        kind,
        AdapterError::invalid_correlation,
        "empty correlation must fail");

    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\tbad id\n",
        kind,
        AdapterError::invalid_correlation,
        "space in correlation must fail");

    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\tbad/id\n",
        kind,
        AdapterError::invalid_correlation,
        "slash in correlation must fail");

    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\tbad\\id\n",
        kind,
        AdapterError::invalid_correlation,
        "backslash in correlation must fail");

    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\tbad\tid\n",
        kind,
        AdapterError::malformed_record,
        "TAB creates an extra field and must fail structurally");
}

void testContextMatching()
{
    constexpr std::string_view record =
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\tcorr\n";

    expectError(
        record,
        ExternalExperimentKind::k3_authorization_disabled,
        AdapterError::context_mismatch,
        "kind mismatch must fail");

    const auto invalidExpectedKind =
        static_cast<ExternalExperimentKind>(999);

    expectError(
        record,
        invalidExpectedKind,
        AdapterError::context_mismatch,
        "invalid expected-kind representation must fail closed");
}

void testValidationPrecedence()
{
    // Schema error must win over later invalid fields.
    expectError(
        "KIKO-EXPERIMENT\t2\tunknown\tunknown\tbad id\n",
        ExternalExperimentKind::production_gate,
        AdapterError::unsupported_schema,
        "schema precedence must be stable");

    // Experiment error must win over outcome/correlation.
    expectError(
        "KIKO-EXPERIMENT\t1\tunknown\tunknown\tbad id\n",
        ExternalExperimentKind::production_gate,
        AdapterError::unsupported_experiment,
        "experiment precedence must be stable");

    // Outcome error must win over correlation.
    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tunknown\tbad id\n",
        ExternalExperimentKind::production_gate,
        AdapterError::invalid_outcome,
        "outcome precedence must be stable");

    // Correlation validation occurs before expected-context validation.
    expectError(
        "KIKO-EXPERIMENT\t1\tproduction_gate\tpassed\tbad id\n",
        ExternalExperimentKind::k3_authorization_disabled,
        AdapterError::invalid_correlation,
        "correlation must precede context mismatch");
}

} // namespace

int main()
{
    testValidProductionPassed();
    testValidOtherVocabulary();
    testCorrelationBoundaries();
    testDeterminism();

    testMalformedFraming();
    testSchemaErrors();
    testExperimentErrors();
    testOutcomeErrors();
    testCorrelationErrors();
    testContextMatching();
    testValidationPrecedence();

    std::cout << "K9 SOAM interchange tests passed\n";
    return EXIT_SUCCESS;
}
