#include "k9_interchange.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace soam::experiment::kiko {

namespace {

constexpr std::string_view marker = "KIKO-EXPERIMENT";
constexpr std::string_view schemaV1 = "1";

[[nodiscard]]
bool validExpectedKind(ExternalExperimentKind kind) noexcept
{
    switch (kind) {
    case ExternalExperimentKind::production_gate:
    case ExternalExperimentKind::k3_authorization_disabled:
        return true;
    default:
        return false;
    }
}

[[nodiscard]]
std::optional<ExternalExperimentKind>
parseExperimentKind(std::string_view token) noexcept
{
    if (token == "production_gate") {
        return ExternalExperimentKind::production_gate;
    }

    if (token == "k3_authorization_disabled") {
        return ExternalExperimentKind::k3_authorization_disabled;
    }

    return std::nullopt;
}

[[nodiscard]]
std::optional<ExternalExperimentalOutcome>
parseOutcome(std::string_view token) noexcept
{
    if (token == "blocked") {
        return ExternalExperimentalOutcome::blocked;
    }

    if (token == "passed") {
        return ExternalExperimentalOutcome::passed;
    }

    return std::nullopt;
}

[[nodiscard]]
bool isCorrelationCharacter(char value) noexcept
{
    return
        (value >= 'A' && value <= 'Z') ||
        (value >= 'a' && value <= 'z') ||
        (value >= '0' && value <= '9') ||
        value == '.' ||
        value == '_' ||
        value == ':' ||
        value == '-';
}

[[nodiscard]]
bool validCorrelation(std::string_view correlation) noexcept
{
    if (correlation.empty() || correlation.size() > 128) {
        return false;
    }

    for (const char value : correlation) {
        if (!isCorrelationCharacter(value)) {
            return false;
        }
    }

    return true;
}

[[nodiscard]]
std::optional<std::array<std::string_view, 5>>
splitFields(std::string_view body) noexcept
{
    std::array<std::string_view, 5> fields{};

    std::size_t fieldIndex = 0;
    std::size_t fieldStart = 0;

    for (std::size_t i = 0; i < body.size(); ++i) {
        if (body[i] != '\t') {
            continue;
        }

        if (fieldIndex >= 4) {
            return std::nullopt;
        }

        fields[fieldIndex++] =
            body.substr(fieldStart, i - fieldStart);
        fieldStart = i + 1;
    }

    if (fieldIndex != 4) {
        return std::nullopt;
    }

    fields[4] = body.substr(fieldStart);
    return fields;
}

} // namespace

namespace detail {

struct ObservationFactory {
    [[nodiscard]]
    static ValidatedExternalExperimentObservation create(
        ExternalExperimentKind experimentKind,
        ExternalExperimentalOutcome outcome,
        std::string_view correlationId)
    {
        return ValidatedExternalExperimentObservation(
            1U,
            experimentKind,
            outcome,
            std::string(correlationId));
    }
};

} // namespace detail

ValidationResult validate(
    std::string_view encodedRecord,
    ExternalExperimentKind expectedKind)
{
    // Frozen framing contract:
    // - exactly one terminal LF
    // - no CR
    // - no embedded LF
    // - no bytes following the terminal LF
    if (encodedRecord.empty() || encodedRecord.back() != '\n') {
        return AdapterError::malformed_record;
    }

    for (std::size_t i = 0; i + 1 < encodedRecord.size(); ++i) {
        if (encodedRecord[i] == '\n' ||
            encodedRecord[i] == '\r') {
            return AdapterError::malformed_record;
        }
    }

    const std::string_view body =
        encodedRecord.substr(0, encodedRecord.size() - 1);

    const auto fields = splitFields(body);
    if (!fields.has_value()) {
        return AdapterError::malformed_record;
    }

    if ((*fields)[0] != marker) {
        return AdapterError::malformed_record;
    }

    // Frozen validation precedence:
    // framing/fields/marker
    // -> schema
    // -> experiment
    // -> outcome
    // -> correlation
    // -> expected context
    if ((*fields)[1] != schemaV1) {
        return AdapterError::unsupported_schema;
    }

    const auto experimentKind =
        parseExperimentKind((*fields)[2]);
    if (!experimentKind.has_value()) {
        return AdapterError::unsupported_experiment;
    }

    const auto outcome = parseOutcome((*fields)[3]);
    if (!outcome.has_value()) {
        return AdapterError::invalid_outcome;
    }

    if (!validCorrelation((*fields)[4])) {
        return AdapterError::invalid_correlation;
    }

    if (!validExpectedKind(expectedKind) ||
        *experimentKind != expectedKind) {
        return AdapterError::context_mismatch;
    }

    return detail::ObservationFactory::create(
        *experimentKind,
        *outcome,
        (*fields)[4]);
}

} // namespace soam::experiment::kiko
