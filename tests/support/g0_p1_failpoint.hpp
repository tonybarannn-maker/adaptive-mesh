#pragma once

#include <stdexcept>

namespace AdaptiveMesh::test_support {

enum class D7PreparationFailurePhase {
    none,
    persistence_lineage,
    d7_publication_lineage
};

inline thread_local D7PreparationFailurePhase d7PreparationFailurePhase =
    D7PreparationFailurePhase::none;

inline void armD7PreparationFailure(D7PreparationFailurePhase phase) noexcept {
    d7PreparationFailurePhase = phase;
}

inline void failIfArmed(D7PreparationFailurePhase phase) {
    if (d7PreparationFailurePhase == phase) {
        d7PreparationFailurePhase = D7PreparationFailurePhase::none;
        throw std::bad_alloc{};
    }
}

} // namespace AdaptiveMesh::test_support
