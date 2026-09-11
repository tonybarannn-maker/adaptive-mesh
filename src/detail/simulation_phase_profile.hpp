#pragma once

namespace AdaptiveMesh {

struct SimulationPhaseProfile final {
    double preValidationMicroseconds = 0.0;
    double workerPoolReadyMicroseconds = 0.0;
    double bufferPreparationMicroseconds = 0.0;
    double workerDispatchWaitMicroseconds = 0.0;
    double resultValidationMicroseconds = 0.0;
    double commitMicroseconds = 0.0;
    double postValidationMicroseconds = 0.0;
};

} // namespace AdaptiveMesh
