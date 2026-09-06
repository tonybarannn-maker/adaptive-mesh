#pragma once

#include <cmath>
#include <stdexcept>

namespace AdaptiveMesh {

class BridgeConfidence {
public:
    explicit BridgeConfidence(double value)
        : value_(value)
    {
        if (!std::isfinite(value)) {
            throw std::invalid_argument(
                "bridge confidence must be finite");
        }
        if (value < 0.0 || value > 1.0) {
            throw std::invalid_argument(
                "bridge confidence must be in [0, 1]");
        }
    }

    [[nodiscard]] double value() const noexcept {
        return value_;
    }

private:
    double value_;
};

} // namespace AdaptiveMesh
