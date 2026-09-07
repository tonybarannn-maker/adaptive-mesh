#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

namespace soam::experiment::kiko {

enum class ExternalExperimentKind {
    production_gate,
    k3_authorization_disabled
};

enum class ExternalExperimentalOutcome {
    blocked,
    passed
};

enum class AdapterError {
    malformed_record,
    unsupported_schema,
    unsupported_experiment,
    invalid_outcome,
    invalid_correlation,
    context_mismatch
};

namespace detail {
struct ObservationFactory;
}

class ValidatedExternalExperimentObservation {
public:
    [[nodiscard]] std::uint32_t schemaVersion() const noexcept {
        return schemaVersion_;
    }

    [[nodiscard]] ExternalExperimentKind experimentKind() const noexcept {
        return experimentKind_;
    }

    [[nodiscard]] ExternalExperimentalOutcome outcome() const noexcept {
        return outcome_;
    }

    [[nodiscard]] std::string_view correlationId() const noexcept {
        return correlationId_;
    }

private:
    ValidatedExternalExperimentObservation(
        std::uint32_t schemaVersion,
        ExternalExperimentKind experimentKind,
        ExternalExperimentalOutcome outcome,
        std::string correlationId)
        : schemaVersion_(schemaVersion),
          experimentKind_(experimentKind),
          outcome_(outcome),
          correlationId_(std::move(correlationId))
    {
    }

    friend struct detail::ObservationFactory;

    std::uint32_t schemaVersion_;
    ExternalExperimentKind experimentKind_;
    ExternalExperimentalOutcome outcome_;
    std::string correlationId_;
};

using ValidationResult =
    std::variant<
        ValidatedExternalExperimentObservation,
        AdapterError>;

[[nodiscard]]
ValidationResult validate(
    std::string_view encodedRecord,
    ExternalExperimentKind expectedKind);

} // namespace soam::experiment::kiko
