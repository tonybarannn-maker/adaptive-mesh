#pragma once

#include <optional>

namespace AdaptiveMesh {

class SpatialAdaptiveMesh;

namespace detail {

struct SimulationPhaseProfile final {
    double preValidationMicroseconds = 0.0;
    double workerPoolReadyMicroseconds = 0.0;
    double bufferPreparationMicroseconds = 0.0;
    double workerDispatchWaitMicroseconds = 0.0;
    double resultValidationMicroseconds = 0.0;
    double commitMicroseconds = 0.0;
};

class SimulationPhaseProfileAccessor final {
public:
    [[nodiscard]] static std::optional<SimulationPhaseProfile> current(
        const SpatialAdaptiveMesh& mesh);
};

} // namespace detail
} // namespace AdaptiveMesh
