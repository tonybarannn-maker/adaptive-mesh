#pragma once

#include <cmath>
#include <stdexcept>

namespace AdaptiveMesh {

class InteractionObservation {
public:
    explicit InteractionObservation(double compatibility)
        : compatibility_(compatibility)
    {
        if (!std::isfinite(compatibility)) {
            throw std::invalid_argument(
                "interaction observation compatibility must be finite");
        }
        if (compatibility < 0.0 || compatibility > 1.0) {
            throw std::invalid_argument(
                "interaction observation compatibility must be in [0, 1]");
        }
    }

    [[nodiscard]] double compatibility() const noexcept {
        return compatibility_;
    }

private:
    double compatibility_;
};

} // namespace AdaptiveMesh
